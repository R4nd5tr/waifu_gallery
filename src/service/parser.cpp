#include "parser.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <QImageReader>
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <xxhash.h>
#include <rapidcsv.h>

std::vector<std::string> splitAndTrim(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), item.end());
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result;
}
XRestrictType toXRestrictTypeEnum (const std::string& xRestrictStr) {
    if (xRestrictStr == "AllAges") {
        return XRestrictType::AllAges;
    } else if (xRestrictStr == "R-18") {
        return XRestrictType::R18;
    } else if (xRestrictStr == "R-18G") {
        return XRestrictType::R18G;
    }
    return XRestrictType::Unknown;
}
AIType toAITypeEnum (const std::string& aiTypeStr) {
    if (aiTypeStr == "No") {
        return AIType::NotAI;
    } else if (aiTypeStr == "Unknown") {
        return AIType::Unknown;
    } else if (aiTypeStr == "Yes") {
        return AIType::AI;
    }
    return AIType::Unknown;
}

PicInfo parsePicture(const std::filesystem::path& pictureFilePath) {
    QString qStrPath = QString::fromStdString(pictureFilePath.string());
    QString qStrDirectory = QString::fromStdString(pictureFilePath.parent_path().string());
    QImageReader reader(qStrPath);
    QSize size = reader.size();
    if (size.isNull()) {
        qCritical() << "failed to load image: " << pictureFilePath.string();
    }
    QFileInfo fileInfo(qStrPath);
    QString name = fileInfo.baseName();

    PicInfo picInfo;
    picInfo.id = calcFileHash(pictureFilePath);
    picInfo.filePaths.insert(pictureFilePath);
    picInfo.height = size.height();
    picInfo.width = size.width();
    picInfo.size = fileInfo.size();
    picInfo.fileType = reader.format().toStdString();
    if (qStrDirectory.contains("pixiv", Qt::CaseInsensitive)) {      // parse pid only when the file is in a directory with "pixiv"
        QStringList parts = name.split("_p");                        //        ^
        uint64_t pid = static_cast<uint64_t>(parts[0].toLongLong()); //        |
        int index = (parts.size() > 1) ? parts[1].toInt() : 0;       // I do these is because it is hard to distinguish between different types of files just by their names
        picInfo.pixivIdIndices.insert({pid, index});                 //        |
    }                                                                //        v
    if (qStrDirectory.contains("twitter", Qt::CaseInsensitive)) {    // parse tweetID only when the file is in a directory with "twitter"
        QStringList parts = name.split("_");
        uint64_t tweetId = static_cast<uint64_t>(parts[0].toLongLong());
        int index = parts[1].toInt() - 1;
        picInfo.tweetIdIndices.insert({tweetId, index});
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
            if (line == "Tags") {                // This logic handles two formats of metadata files:
                std::getline(file, line);        // 1. The description section is at the end of the file.
                while (line != "") {             // 2. The description section is followed by the tags section.
                    info.tags.push_back(line);   // This approach ensures all line breaks in the description are 
                    std::getline(file, line);    // preserved and both formats are supported.
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
        if (std::find(colNames.begin(), colNames.end(), "likeCount") != colNames.end()) info.likeCount = doc.GetCell<uint32_t>("likeCount", i);
        if (std::find(colNames.begin(), colNames.end(), "viewCount") != colNames.end()) info.viewCount = doc.GetCell<uint32_t>("viewCount", i);
        info.xRestrict = toXRestrictTypeEnum(doc.GetCell<std::string>("xRestrict", i));
        if (hasAI) info.aiType = toAITypeEnum(doc.GetCell<std::string>("AI", i));
        info.date = doc.GetCell<std::string>("date", i);
        result.push_back(info);
    }
    return result;
}
QByteArray readJsonFile (const std::filesystem::path& jsonFilePath) {
    QFile file(QString::fromStdString(jsonFilePath.string()));
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file:" << jsonFilePath.string();
        return QByteArray();
    }
    QByteArray data = file.readAll();
    file.close();
    return data;
}
JsonType detectJsonType(const QByteArray& data) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return JsonType::Unknown;
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        if (!arr.isEmpty() && arr[0].isObject()) {
            QJsonObject obj = arr[0].toObject();
            if (obj.contains("idNum")) return JsonType::Pixiv;
        }
    } else if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("tweet_id")) return JsonType::Tweet;
    }
    return JsonType::Unknown;
}
PixivInfo parsePixivJson(const QByteArray& data) {
    PixivInfo info{};
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        qCritical() << "Failed to parse JSON:" << err.errorString();
        return info;
    }
    QJsonArray arr = doc.array();
    if (arr.isEmpty() || !arr[0].isObject()) {
        qCritical() << "JSON array is empty or first element is not an object";
        return info;
    }
    QJsonObject obj = arr[0].toObject();
    info.pixivID = obj.value("idNum").toVariant().toLongLong();
    info.title = obj.value("title").toString().toStdString();
    info.description = obj.value("description").toString().toStdString();
    info.authorName = obj.value("user").toString().toStdString();
    info.authorID = obj.value("userId").toString().toUInt();
    info.likeCount = obj.value("likeCount").toInt();
    info.viewCount = obj.value("viewCount").toInt();
    info.xRestrict = static_cast<XRestrictType>(obj.value("xRestrict").toInt());
    info.aiType = static_cast<AIType>(obj.value("aiType").toInt());
    info.date = obj.value("date").toString().toStdString();

    // tags
    if (obj.contains("tags") && obj.value("tags").isArray()) {
        QJsonArray tagsArr = obj.value("tags").toArray();
        for (const QJsonValue& tag : tagsArr) {
            info.tags.push_back(tag.toString().toStdString());
        }
    }
    // tagsTransl
    if (obj.contains("tagsWithTransl") && obj.value("tagsWithTransl").isArray()) {
        QJsonArray tagsTranslArr = obj.value("tagsWithTransl").toArray();
        for (const QJsonValue& tag : tagsTranslArr) {
            info.tagsTransl.push_back(tag.toString().toStdString());
        }
    }
    return info;
}
TweetInfo parseTweetJson(const QByteArray& data) {
    TweetInfo info;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qCritical() << "Failed to parse JSON:" << err.errorString();
        return info;
    }
    QJsonObject obj = doc.object();

    info.tweetID = obj.value("tweet_id").toVariant().toLongLong();
    info.date = obj.value("date").toString().toStdString();
    info.description = obj.value("content").toString().toStdString();
    info.favoriteCount = obj.value("favorite_count").toInt();
    info.quoteCount = obj.value("quote_count").toInt();
    info.replyCount = obj.value("reply_count").toInt();
    info.retweetCount = obj.value("retweet_count").toInt();
    info.bookmarkCount = obj.value("bookmark_count").toInt();
    info.viewCount = obj.value("view_count").toInt();
    // 解析 author 对象
    if (obj.contains("author") && obj.value("author").isObject()) {
        QJsonObject authorObj = obj.value("author").toObject();
        info.authorID = authorObj.value("id").toVariant().toInt();
        info.authorName = authorObj.value("name").toString().toStdString();
        info.authorNick = authorObj.value("nick").toString().toStdString();
        info.authorDescription = authorObj.value("description").toString().toStdString();
        info.authorProfileImage = authorObj.value("profile_image").toString().toStdString();
        info.authorProfileBanner = authorObj.value("profile_banner").toString().toStdString();
    }
    // 解析 hashtags 数组
    if (obj.contains("hashtags") && obj.value("hashtags").isArray()) {
        QJsonArray hashtagsArr = obj.value("hashtags").toArray();
        for (const QJsonValue& tag : hashtagsArr) {
            info.hashtags.insert(tag.toString().toStdString());
        }
    }
    return info;
}
uint64_t calcFileHash(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        qWarning() << "Failed to open file:" << filePath.c_str();
        return 0;
    }
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize > 100 * 1024 * 1024) {  // 100MB Max
        qWarning() << "File too large:" << filePath.c_str() << "size:" << fileSize;
        return 0;
    }
    if (fileSize == 0) {
        qWarning() << "Empty file:" << filePath.c_str();
        return 0;
    }
    std::vector<char> buffer(static_cast<size_t>(fileSize));
    if (!file.read(buffer.data(), fileSize)) {
        qWarning() << "Failed to read file:" << filePath.c_str();
        return 0;
    }
    return XXH64(buffer.data(), static_cast<size_t>(fileSize), 0);
}