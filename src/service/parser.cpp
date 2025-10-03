#include "parser.h"
#include <QDebug>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <rapidcsv.h>
#include <regex>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vector>
#include <webp/decode.h>
#include <xxhash.h>

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
std::pair<int, int> getImageResolution(const std::vector<uint8_t>& buffer, const std::string& fileType) {
    int width = 0, height = 0, channels = 0;
    if (fileType == "webp") {
        if (WebPGetInfo(buffer.data(), buffer.size(), &width, &height)) {
            return {width, height};
        }
    } else {
        unsigned char* imgData =
            stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()), &width, &height, &channels, 0);
        if (imgData) {
            stbi_image_free(imgData);
            return {width, height};
        }
    }
    return {0, 0};
}
std::vector<uint8_t> readFileToBuffer(const std::filesystem::path& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        qCritical() << "Failed to open file:" << imagePath.string();
        return {};
    }
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    if (size == 0) {
        qCritical() << "File is empty:" << imagePath.string();
        return {};
    }
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        qCritical() << "Failed to read file:" << imagePath.string();
        return {};
    }
    return buffer;
}
uint64_t calcFileHash(const std::vector<uint8_t>& buffer) {
    return XXH64(buffer.data(), static_cast<size_t>(buffer.size()), 0);
}
PicInfo parsePicture(const std::filesystem::path& pictureFilePath, ParserType parserType) {
    std::vector<uint8_t> buffer = readFileToBuffer(pictureFilePath);

    std::string fileName = pictureFilePath.filename().string();
    std::string fileType = pictureFilePath.extension().string().substr(1);
    std::transform(fileType.begin(), fileType.end(), fileType.begin(), ::tolower);
    if (fileType == "jpeg") fileType = "jpg";

    int width, height;
    std::tie(width, height) = getImageResolution(buffer, fileType);

    PicInfo picInfo;
    picInfo.id = calcFileHash(buffer);
    picInfo.filePaths.insert(pictureFilePath);
    picInfo.width = width;
    picInfo.height = height;
    picInfo.size = static_cast<uint32_t>(buffer.size());
    picInfo.fileType = fileType;
    if (parserType == ParserType::Pixiv) {
        std::regex pattern(R"(.*?(\d+)_p(\d+).*)");
        std::smatch match;
        if (std::regex_match(fileName, match, pattern) && match.size() == 3) {
            uint32_t pixivId = static_cast<uint32_t>(std::stoul(match[1].str()));
            int index = std::stoi(match[2].str());
            picInfo.pixivIdIndices[pixivId] = index;
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
        qCritical() << "Failed to open file:" << pixivMetadataFilePath.string();
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
        qCritical() << "Failed to parse JSON or JSON is not an array.";
        return result;
    }
    for (const auto& obj : json) {
        PixivInfo info;
        info.pixivID = obj.value("idNum", 0LL);
        info.title = obj.value("title", "");
        info.description = obj.value("description", "");
        info.authorName = obj.value("user", "");
        info.authorID = obj.value("userId", 0U);
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
        qCritical() << "Failed to parse JSON: not an object";
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
        info.authorID = authorObj.value("id", 0);
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