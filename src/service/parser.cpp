#include "parser.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <QImageReader>
#include <QString>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <xxhash.h>
#include <fast-cpp-csv-parser/csv.h>

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
PicInfo parsePicture(const std::string& pictureFilePath, ParserType parser) {
    QString qStrPath = QString::fromStdString(pictureFilePath);
    QImageReader reader(qStrPath);
    QSize size = reader.size();
    if (size.isNull()) {
        qCritical() << "failed to load image: " << pictureFilePath;
    }
    QFileInfo fileInfo(qStrPath);
    QString name = fileInfo.baseName();

    PicInfo picInfo;
    picInfo.id = calcFileHash(pictureFilePath);
    picInfo.filePaths.insert(pictureFilePath);
    picInfo.height = size.height();
    picInfo.width = size.width();
    picInfo.size = fileInfo.size();
    if (parser == ParserType::Pixiv) {
        QStringList parts = name.split("_p");
        uint64_t pid = static_cast<uint64_t>(parts[0].toLongLong());
        int index = (parts.size() > 1) ? parts[1].toInt() : 0;
        picInfo.pixivIdIndices.insert({pid, index});
    }
    if (parser == ParserType::Twitter) {
        QStringList parts = name.split("_");
        uint64_t tweetId = static_cast<uint64_t>(parts[0].toLongLong());
        int index = (parts.size() > 1) ? parts[1].toInt() : 0;
        picInfo.tweetIdIndices.insert({tweetId, index});
    }
    return picInfo;
};
PixivInfo parsePixivMetadata(const std::string& pixivMetadataFilePath) {
    std::ifstream file(pixivMetadataFilePath);
    if (!file.is_open()) {
        qCritical() << "Failed to open file:" << pixivMetadataFilePath;
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
    return info;
}
std::vector<PixivInfo> parsePixivCsv(const std::string& pixivCsvFilePath) {
    std::ifstream file(pixivCsvFilePath);
    if (!file.is_open()) {
        qCritical() << "Failed to open file:" << pixivCsvFilePath;
        return {};
    }
    std::string headerLine;
    size_t headerCount = 0;
    std::vector<PixivInfo> result;
    //get header count
    if (std::getline(file, headerLine)) {
        std::stringstream ss(headerLine);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            ++headerCount;
        }
    }
    int64_t pixivID;
    std::string tagsStr;
    std::string tagsTranslStr;
    std::string authorName;
    uint32_t authorID;
    std::string title;
    std::string description;
    uint32_t likeCount;
    uint32_t viewCount;
    std::string xRestrictStr;
    std::string aiTypeStr;
    std::string date;

    if (headerCount == 21) {
        io::CSVReader<11> in(pixivCsvFilePath);
        in.read_header(io::ignore_extra_column, "id", "tags", "tags_transl", "user", "userId", 
            "title", "description", "likeCount", "viewCount", "xRestrict", "date"
        );
        while (in.read_row(pixivID, tagsStr, tagsTranslStr, authorName, authorID, 
            title, description, likeCount, viewCount, xRestrictStr, date
        )) {
            PixivInfo info;
            info.pixivID = pixivID;
            info.tags = splitAndTrim(tagsStr);
            info.tagsTransl = splitAndTrim(tagsTranslStr);
            info.authorName = authorName;
            info.authorID = authorID;
            info.title = title;
            info.description = description;
            info.likeCount = likeCount;
            info.viewCount = viewCount;
            info.xRestrict = toXRestrictTypeEnum(xRestrictStr);
            info.date = date;
            result.push_back(info);
        }
    } else if (headerCount == 22) {
        io::CSVReader<12> in(pixivCsvFilePath);
        in.read_header(io::ignore_extra_column, "id", "tags", "tags_transl", "user", "userId", 
            "title", "description", "likeCount", "viewCount", "xRestrict", "AI", "date"
        );
        while (in.read_row(pixivID, tagsStr, tagsTranslStr, authorName, authorID, 
            title, description, likeCount, viewCount, xRestrictStr, aiTypeStr, date
        )) {
            PixivInfo info;
            info.pixivID = pixivID;
            info.tags = splitAndTrim(tagsStr);
            info.tagsTransl = splitAndTrim(tagsTranslStr);
            info.authorName = authorName;
            info.authorID = authorID;
            info.title = title;
            info.description = description;
            info.likeCount = likeCount;
            info.viewCount = viewCount;
            info.xRestrict = toXRestrictTypeEnum(xRestrictStr);
            info.aiType = toAITypeEnum(aiTypeStr);
            info.date = date;
            result.push_back(info);
        }
    } else {
        qCritical() << "incompatible csv file header count: " << pixivCsvFilePath;
        return result;
    }
    return result;
}
PixivInfo parsePixivJson(const std::string& pixivJsonFilePath) {
    PixivInfo info{};
    QFile file(QString::fromStdString(pixivJsonFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file:" << QString::fromStdString(pixivJsonFilePath);
        return info;
    }
    QByteArray data = file.readAll();
    file.close();

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
TweetInfo parseTweetJson(const std::string& tweetJsonFilePath) {
    TweetInfo info;
    QFile file(QString::fromStdString(tweetJsonFilePath));
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file:" << QString::fromStdString(tweetJsonFilePath);
        return info;
    }
    QByteArray data = file.readAll();
    file.close();

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
uint64_t calcFileHash(const std::string& filePath) {
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