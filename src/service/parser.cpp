#include "parser.h"
#include "../utils/logger.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <rapidcsv.h>
#include <regex>
#define STB_IMAGE_IMPLEMENTATION
#include <chrono>
#include <stb_image.h>
#include <vector>
#include <webp/decode.h>
#include <xxhash.h>

static const std::unordered_map<std::string, ImageFormat> fileTypeMap = {
    {"JPG", ImageFormat::JPG},
    {"JPEG", ImageFormat::JPG},
    {"PNG", ImageFormat::PNG},
    {"GIF", ImageFormat::GIF},
    {"WEBP", ImageFormat::WebP},
};

// Utility functions
std::vector<uint8_t> readFileToBuffer(const std::filesystem::path& imagePath);
uint64_t calcFileHash(const std::vector<uint8_t>& buffer);
std::vector<std::string> splitAndTrim(const std::string& str);
std::tuple<int, int, ImageFormat> getImageResolutionOptimized(const std::vector<uint8_t>& buffer, ImageFormat fileType);
XRestrictType toXRestrictTypeEnum(const std::string& xRestrictStr);
AIType toAITypeEnum(const std::string& aiTypeStr);

PicInfo parsePicture(const std::filesystem::path& pictureFilePath, ParserType parserType) {
    std::vector<uint8_t> buffer = readFileToBuffer(pictureFilePath);

    std::string fileName = pictureFilePath.filename().string();
    std::string fileTypeStr = pictureFilePath.extension().string().substr(1);
    std::transform(fileTypeStr.begin(), fileTypeStr.end(), fileTypeStr.begin(), ::toupper);
    ImageFormat fileType = fileTypeMap.at(fileTypeStr);

    int width, height;
    std::tie(width, height, fileType) = getImageResolutionOptimized(buffer, fileType);

    PicInfo picInfo;
    picInfo.id = calcFileHash(buffer);
    picInfo.filePaths.insert(pictureFilePath);
    picInfo.width = width;
    picInfo.height = height;
    picInfo.size = static_cast<uint32_t>(buffer.size());
    picInfo.fileType = fileType;
    if (parserType == ParserType::Pixiv) {
        if (fileName.find("_p") == std::string::npos) {
            std::string stem = pictureFilePath.stem().string();
            if (std::all_of(stem.begin(), stem.end(), ::isdigit)) {
                uint32_t pixivId = static_cast<uint32_t>(std::stoul(stem));
                picInfo.pixivIdIndices[pixivId] = 0;
            }
        } else {
            std::regex pattern(R"(.*?(\d+)_p(\d+).*)");
            std::smatch match;
            if (std::regex_match(fileName, match, pattern) && match.size() == 3) {
                uint32_t pixivId = static_cast<uint32_t>(std::stoul(match[1].str()));
                int index = std::stoi(match[2].str());
                picInfo.pixivIdIndices[pixivId] = index;
            }
        }
    }
    if (parserType == ParserType::Twitter) {
        size_t firstUnderscore = fileName.find('_');
        size_t dot = fileName.find('.', firstUnderscore + 1);
        if (firstUnderscore != std::string::npos && dot != std::string::npos) {
            std::string tweetIdStr = fileName.substr(0, firstUnderscore);
            std::string indexStr = fileName.substr(firstUnderscore + 1, dot - firstUnderscore - 1);
            int64_t tweetId = std::stoll(tweetIdStr);
            int index = std::stoi(indexStr) - 1;
            picInfo.tweetIdIndices[tweetId] = index;
        }
    }
    return picInfo;
}
PixivInfo parsePixivMetadata(const std::filesystem::path& pixivMetadataFilePath) {
    std::ifstream file(pixivMetadataFilePath);
    if (!file.is_open()) {
        Error() << "Failed to open file:" << pixivMetadataFilePath.string();
        return PixivInfo{};
    }
    std::string line;
    PixivInfo info{};
    while (std::getline(file, line)) {
        if (line == "Id" || line == "ID") {
            std::getline(file, line);
            info.pixivID = static_cast<int64_t>(std::stoll(line));
        } else if (line == "xRestrict") {
            std::getline(file, line);
            info.xRestrict = toXRestrictTypeEnum(line);
        } else if (line == "AI") {
            std::getline(file, line);
            info.aiType = toAITypeEnum(line);
        } else if (line == "User") {
            std::getline(file, line);
            info.authorName = line;
        } else if (line == "UserID" || line == "UserId") {
            std::getline(file, line);
            info.authorID = static_cast<uint32_t>(std::stoi(line));
        } else if (line == "Title") {
            std::getline(file, line);
            info.title = line;
        } else if (line == "Description") {
            while (std::getline(file, line) && line != "Tags") {
                info.description += line + "\n";
            }
            if (line == "Tags") {              // This logic handles two formats of metadata files:
                std::getline(file, line);      // 1. The description section is at the end of the file.
                while (line != "") {           // 2. The description section is followed by the tags section.
                    info.tags.push_back(line); // This approach ensures all line breaks in the description are
                    std::getline(file, line);  // preserved and both formats are supported.
                }
            }
        } else if (line == "Tags") {
            std::getline(file, line);
            while (line != "") {
                info.tags.push_back(line);
                std::getline(file, line);
            }
        } else if (line == "Date") {
            std::getline(file, line);
            info.date = line;
        }
    }
    for (auto& tag : info.tags) {
        if (!tag.empty() && tag[0] == '#') {
            tag.erase(0, 1);
        }
    }
    return info;
}
std::vector<PixivInfo> parsePixivCsv(const std::filesystem::path& pixivCsvFilePath) {
    std::vector<PixivInfo> result;
    std::ifstream file(pixivCsvFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << pixivCsvFilePath << std::endl;
        return result;
    }
    rapidcsv::Document doc(file, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams(',', '"', false));
    size_t rowCount = doc.GetRowCount();

    auto colNames = doc.GetColumnNames();
    bool hasAI = std::find(colNames.begin(), colNames.end(), "AI") != colNames.end();

    for (size_t i = 0; i < rowCount; ++i) {
        PixivInfo info;
        info.pixivID = doc.GetCell<int64_t>("id", i);
        info.tags = splitAndTrim(doc.GetCell<std::string>("tags", i));
        info.tagsTransl = splitAndTrim(doc.GetCell<std::string>("tags_transl", i));
        info.authorName = doc.GetCell<std::string>("user", i);
        info.authorID = doc.GetCell<uint32_t>("userId", i);
        info.title = doc.GetCell<std::string>("title", i);
        info.description = doc.GetCell<std::string>("description", i);
        if (std::find(colNames.begin(), colNames.end(), "likeCount") != colNames.end())
            info.likeCount = doc.GetCell<uint32_t>("likeCount", i);
        if (std::find(colNames.begin(), colNames.end(), "viewCount") != colNames.end())
            info.viewCount = doc.GetCell<uint32_t>("viewCount", i);
        info.xRestrict = toXRestrictTypeEnum(doc.GetCell<std::string>("xRestrict", i));
        if (hasAI) info.aiType = toAITypeEnum(doc.GetCell<std::string>("AI", i));
        info.date = doc.GetCell<std::string>("date", i);
        result.push_back(info);
    }
    return result;
}
std::vector<PixivInfo> parsePixivJson(const std::filesystem::path& pixivJsonFilePath) {
    std::vector<PixivInfo> result;
    std::vector<uint8_t> data = readFileToBuffer(pixivJsonFilePath);
    auto json = nlohmann::json::parse(data, nullptr, false);
    if (json.is_discarded() || !json.is_array()) {
        Error() << "Failed to parse JSON or JSON is not an array.";
        return result;
    }
    for (const auto& obj : json) {
        PixivInfo info;
        info.pixivID = obj.value("idNum", 0LL);
        info.title = obj.value("title", "");
        info.description = obj.value("description", "");
        info.authorName = obj.value("user", "");
        info.authorID = std::stoul(obj.value("userId", ""));
        info.likeCount = obj.value("likeCount", 0);
        info.viewCount = obj.value("viewCount", 0);
        info.xRestrict = static_cast<XRestrictType>(obj.value("xRestrict", 0) + 1);
        info.aiType = static_cast<AIType>(obj.value("aiType", 0));
        info.date = obj.value("date", "");

        // tags
        if (obj.contains("tags") && obj["tags"].is_array()) {
            for (const auto& tag : obj["tags"]) {
                info.tags.push_back(tag.get<std::string>());
            }
        }
        // tagsWithTransl
        if (obj.contains("tagsWithTransl") && obj["tagsWithTransl"].is_array()) {
            for (const auto& tag : obj["tagsWithTransl"]) {
                info.tagsTransl.push_back(tag.get<std::string>());
            }
        }
        result.push_back(info);
    }
    return result;
}
TweetInfo parseTweetJson(const std::filesystem::path& tweetJsonFilePath) {
    TweetInfo info;
    std::vector<uint8_t> data = readFileToBuffer(tweetJsonFilePath);
    auto json = nlohmann::json::parse(data.begin(), data.end());
    if (!json.is_object()) {
        Error() << "Failed to parse JSON: not an object";
        return info;
    }

    info.tweetID = json.value("tweet_id", 0LL);
    info.date = json.value("date", "");
    info.description = json.value("content", "");
    info.favoriteCount = json.value("favorite_count", 0);
    info.quoteCount = json.value("quote_count", 0);
    info.replyCount = json.value("reply_count", 0);
    info.retweetCount = json.value("retweet_count", 0);
    info.bookmarkCount = json.value("bookmark_count", 0);
    info.viewCount = json.value("view_count", 0);

    // 解析 author 对象
    if (json.contains("author") && json["author"].is_object()) {
        const auto& authorObj = json["author"];
        info.authorID = authorObj.value("id", 0LL);
        info.authorName = authorObj.value("name", "");
        info.authorNick = authorObj.value("nick", "");
        info.authorDescription = authorObj.value("description", "");
        info.authorProfileImage = authorObj.value("profile_image", "");
        info.authorProfileBanner = authorObj.value("profile_banner", "");
    }

    // 解析 hashtags 数组
    if (json.contains("hashtags") && json["hashtags"].is_array()) {
        for (const auto& tag : json["hashtags"]) {
            info.hashtags.insert(tag.get<std::string>());
        }
    }
    return info;
}

// ----------------- Utility Functions ----------------

std::vector<std::string> splitAndTrim(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                   item.end());
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result;
}
XRestrictType toXRestrictTypeEnum(const std::string& xRestrictStr) {
    if (xRestrictStr == "AllAges") {
        return XRestrictType::AllAges;
    } else if (xRestrictStr == "R-18") {
        return XRestrictType::R18;
    } else if (xRestrictStr == "R-18G") {
        return XRestrictType::R18G;
    }
    return XRestrictType::Unknown;
}
AIType toAITypeEnum(const std::string& aiTypeStr) {
    if (aiTypeStr == "No") {
        return AIType::NotAI;
    } else if (aiTypeStr == "Unknown") {
        return AIType::Unknown;
    } else if (aiTypeStr == "Yes") {
        return AIType::AI;
    }
    return AIType::Unknown;
}
std::vector<uint8_t> readFileToBuffer(const std::filesystem::path& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        Error() << "Failed to open file:" << imagePath.string();
        return {};
    }
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    if (size == 0) {
        Error() << "File is empty:" << imagePath.string();
        return {};
    }
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        Error() << "Failed to read file:" << imagePath.string();
        return {};
    }
    return buffer;
}
uint64_t calcFileHash(const std::vector<uint8_t>& buffer) {
    return XXH64(buffer.data(), static_cast<size_t>(buffer.size()), 0);
}
std::tuple<int, int, ImageFormat> getImageResolution(const std::vector<uint8_t>& buffer, ImageFormat fileType) {
    int width = 0, height = 0, channels = 0;
    if (fileType == ImageFormat::WebP) {
        if (WebPGetInfo(buffer.data(), buffer.size(), &width, &height)) {
            return {width, height, fileType};
        }
    } else {
        unsigned char* imgData =
            stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()), &width, &height, &channels, 0);
        if (imgData) {
            stbi_image_free(imgData);
            return {width, height, fileType};
        }
    }
    return {0, 0, fileType};
}

struct ImageHeaderInfo {
    ImageFormat format = ImageFormat::Unknown;
    int width = 0;
    int height = 0;
    bool isValid = false;
    std::string formatName;
};

// 检查签名的辅助函数
inline bool checkSignature(const std::vector<uint8_t>& buffer, const char* signature, size_t len) {
    return buffer.size() >= len && memcmp(buffer.data(), signature, len) == 0;
}
inline bool checkSignature(const uint8_t* data, const char* signature, size_t len) {
    return memcmp(data, signature, len) == 0;
}

// 各种格式的解析函数
void parseJPEGResolution(const std::vector<uint8_t>& buffer, ImageHeaderInfo& info) {
    static constexpr char JPEG_SIGNATURE[] = "\xFF\xD8\xFF";

    if (!checkSignature(buffer, JPEG_SIGNATURE, 3)) return;

    size_t pos = 2; // 跳过FF D8
    while (pos + 9 < buffer.size()) {
        if (buffer[pos] != 0xFF) break;

        uint8_t marker = buffer[pos + 1];
        uint16_t segmentLength = (buffer[pos + 2] << 8) | buffer[pos + 3];

        // SOF标记 (Start of Frame)
        if ((marker >= 0xC0 && marker <= 0xC3) || (marker >= 0xC5 && marker <= 0xC7) || (marker >= 0xC9 && marker <= 0xCB) ||
            (marker >= 0xCD && marker <= 0xCF)) {

            if (pos + 7 < buffer.size()) {
                info.height = (buffer[pos + 5] << 8) | buffer[pos + 6];
                info.width = (buffer[pos + 7] << 8) | buffer[pos + 8];
                info.isValid = (info.width > 0 && info.height > 0);
                return;
            }
        }

        if (segmentLength < 2) break;
        pos += segmentLength + 2;
    }
}

void parsePNGResolution(const std::vector<uint8_t>& buffer, ImageHeaderInfo& info) {
    static constexpr char PNG_SIGNATURE[] = "\x89PNG\r\n\x1A\n";
    static constexpr char IHDR_CHUNK[] = "IHDR";

    if (!checkSignature(buffer, PNG_SIGNATURE, 8)) return;

    // IHDR块在文件头后8字节开始
    if (buffer.size() >= 24) {
        // 检查IHDR块标识
        if (memcmp(&buffer[12], IHDR_CHUNK, 4) == 0) {
            info.width = (buffer[16] << 24) | (buffer[17] << 16) | (buffer[18] << 8) | buffer[19];
            info.height = (buffer[20] << 24) | (buffer[21] << 16) | (buffer[22] << 8) | buffer[23];
            info.isValid = (info.width > 0 && info.height > 0);
        }
    }
}

void parseGIFResolution(const std::vector<uint8_t>& buffer, ImageHeaderInfo& info) {
    static constexpr char GIF87A_SIGNATURE[] = "GIF87a";
    static constexpr char GIF89A_SIGNATURE[] = "GIF89a";

    if (!checkSignature(buffer, GIF87A_SIGNATURE, 6) && !checkSignature(buffer, GIF89A_SIGNATURE, 6)) {
        return;
    }

    if (buffer.size() >= 10) {
        info.width = buffer[6] | (buffer[7] << 8);
        info.height = buffer[8] | (buffer[9] << 8);
        info.isValid = (info.width > 0 && info.height > 0);
    }
}

void parseWebPResolution(const std::vector<uint8_t>& buffer, ImageHeaderInfo& info) {
    static constexpr char WEBP_RIFF_SIGNATURE[] = "RIFF";
    static constexpr char WEBP_WEBP_SIGNATURE[] = "WEBP";

    // 检查RIFF和WEBP签名
    if (!checkSignature(buffer, WEBP_RIFF_SIGNATURE, 4) || buffer.size() < 16 ||
        !checkSignature(buffer.data() + 8, WEBP_WEBP_SIGNATURE, 4)) {
        return;
    }

    // VP8 (lossy)
    if (checkSignature(buffer.data() + 12, "VP8 ", 4)) {
        if (buffer.size() >= 30) {
            info.width = (buffer[26] | (buffer[27] << 8)) & 0x3FFF;
            info.height = (buffer[28] | (buffer[29] << 8)) & 0x3FFF;
            info.isValid = (info.width > 0 && info.height > 0);
        }
    }
    // VP8L (lossless)
    else if (checkSignature(buffer.data() + 12, "VP8L", 4)) {
        if (buffer.size() >= 25) {
            uint32_t bits = buffer[21] | (buffer[22] << 8) | (buffer[23] << 16) | (buffer[24] << 24);
            info.width = (bits & 0x3FFF) + 1;
            info.height = ((bits >> 14) & 0x3FFF) + 1;
            info.isValid = (info.width > 0 && info.height > 0);
        }
    }
    // VP8X (extended)
    else if (checkSignature(buffer.data() + 12, "VP8X", 4)) {
        if (buffer.size() >= 30) {
            info.width = (buffer[24] | (buffer[25] << 8) | (buffer[26] << 16)) + 1;
            info.height = (buffer[27] | (buffer[28] << 8) | (buffer[29] << 16)) + 1;
            info.isValid = (info.width > 0 && info.height > 0);
        }
    }
}

// 统一的解析函数
ImageHeaderInfo parseImageHeader(const std::vector<uint8_t>& buffer) {
    ImageHeaderInfo info;

    if (buffer.size() < 12) {
        return info;
    }

    // 定义格式签名表
    struct FormatSignature {
        const char* signature;
        size_t length;
        ImageFormat format;
        const char* name;
        void (*parser)(const std::vector<uint8_t>&, ImageHeaderInfo&);
    };

    static constexpr FormatSignature SIGNATURES[] = {
        {"\xFF\xD8\xFF", 3, ImageFormat::JPG, "JPG", parseJPEGResolution},
        {"\x89PNG\r\n\x1A\n", 8, ImageFormat::PNG, "PNG", parsePNGResolution},
        {"GIF87a", 6, ImageFormat::GIF, "GIF", parseGIFResolution},
        {"GIF89a", 6, ImageFormat::GIF, "GIF", parseGIFResolution},
        {"RIFF", 4, ImageFormat::WebP, "WebP", parseWebPResolution},
    };

    // 检查所有已知格式
    for (const auto& sig : SIGNATURES) {
        if (checkSignature(buffer, sig.signature, sig.length)) {
            info.format = sig.format;
            info.formatName = sig.name;

            // 调用对应的解析函数
            sig.parser(buffer, info);
            break;
        }
    }

    return info;
}

std::tuple<int, int, ImageFormat> getImageResolutionOptimized(const std::vector<uint8_t>& buffer, ImageFormat fileType) {
    ImageHeaderInfo headerInfo = parseImageHeader(buffer);

    if (headerInfo.isValid) {
        fileType = headerInfo.format;
        return {headerInfo.width, headerInfo.height, fileType};
    }

    // 回退到完整解码
    return getImageResolution(buffer, fileType);
}
