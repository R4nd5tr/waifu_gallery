#include "../service/database.h"
#include "worker.h"
#include <QImageReader>

DatabaseWorker::DatabaseWorker(const QString &connectionName, QObject *parent) {
    database = PicDatabase(connectionName);
}
DatabaseWorker::~DatabaseWorker() {}
void DatabaseWorker::scanDirectory(std::filesystem::path directory) {
    database.scanDirectory(directory);
    emit scanComplete();
}
void DatabaseWorker::searchPics(
    const std::unordered_set<std::string>& includedTags,
    const std::unordered_set<std::string>& excludedTags,
    const std::unordered_set<std::string>& includedPixivTags,
    const std::unordered_set<std::string>& excludedPixivTags,
    const std::unordered_set<std::string>& includedTweetTags,
    const std::unordered_set<std::string>& excludedTweetTags,
    const std::string& searchText,
    SearchField searchField,
    bool selectedTagChanged,
    bool selectedPixivTagChanged,
    bool selectedTweetTagChanged,
    bool searchTextChanged,
    size_t requestId
) {
    std::unordered_map<uint64_t, int64_t> textSearchResult;
    bool disableTextSearch = (searchText.empty() || searchField == SearchField::None);
    if (disableTextSearch) {
        lastTextSearchResult.clear();
        searchTextChanged = false;
    }
    
    if (selectedTagChanged) {
        lastTagSearchResult = database.tagSearch(includedTags, excludedTags);
    }
    if (selectedPixivTagChanged) {
        lastPixivTagSearchResult = database.pixivTagSearch(includedPixivTags, excludedPixivTags);
    }
    if (selectedTweetTagChanged) {
        lastTweetTagSearchResult = database.tweetHashtagSearch(includedTweetTags, excludedTweetTags);
    }
    if (searchTextChanged) {
        textSearchResult = database.textSearch(searchText, searchField); // I do this because I want to get the pic id and the matched pixiv or tweet id at the same time
        lastTextSearchResult.clear();
        for (const auto& [id, _] : textSearchResult) {
            lastTextSearchResult.push_back(id);
        }
    }
    // intersect all search results
    std::unordered_map<uint64_t, int> idCount;
    auto addToCount = [&idCount](const std::vector<uint64_t>& pics) {
        for (const auto& pic : pics) {
            idCount[pic]++;
        }
    };
    size_t totalConditions = 0;
    if (!includedTags.empty() || !excludedTags.empty()) {
        addToCount(lastTagSearchResult);
        totalConditions++;
    }
    if (!includedPixivTags.empty() || !excludedPixivTags.empty()) {
        addToCount(lastPixivTagSearchResult);
        totalConditions++;
    }
    if (!includedTweetTags.empty() || !excludedTweetTags.empty()) {
        addToCount(lastTweetTagSearchResult);
        totalConditions++;
    }
    if (!disableTextSearch) {
        addToCount(lastTextSearchResult);
        totalConditions++;
    }

    std::vector<PicInfo> resultPics;
    for (const auto& [id, count] : idCount) {
        if (count != totalConditions) {
            continue;
        }
        if (!disableTextSearch) {
            switch (searchField) {
                case SearchField::PixivID:
                case SearchField::PixivAuthorID:
                case SearchField::PixivAuthorName:
                case SearchField::PixivTitle:
                case SearchField::PixivDescription:
                    resultPics.push_back(database.getPicInfo(id, 0, textSearchResult[id])); // make sure to get the matched pixivID then display correct search result
                    break;
                case SearchField::TweetID:
                case SearchField::TweetAuthorID:
                case SearchField::TweetAuthorName:
                case SearchField::TweetAuthorNick:
                case SearchField::TweetDescription:
                    resultPics.push_back(database.getPicInfo(id, textSearchResult[id], 0)); // make sure to get the matched tweetID then display correct search result
                    break;
                default:
                    resultPics.push_back(database.getPicInfo(id));
                    break;
            }
        } else {
            resultPics.push_back(database.getPicInfo(id));
        }
    }
    emit searchComplete(std::move(resultPics), requestId);
}

LoaderWorker::LoaderWorker(QObject* parent) : QObject(parent) {}
LoaderWorker::~LoaderWorker() {}
void LoaderWorker::loadImage(uint64_t id, const std::unordered_set<std::filesystem::path>& filePaths) {
    QString filePathStr;
    for (const auto& filePath : filePaths) {
        filePathStr = QString::fromUtf8(filePath.u8string().c_str());
        if (!std::filesystem::exists(filePath)) {
            qWarning() << "File does not exist:" << filePathStr;
            continue;
        }
        break;
    }
    QImageReader reader(filePathStr);
    if (!reader.canRead()) {
        qWarning() << "Cannot read image format:" << filePathStr;
        return;
    }
    reader.setAutoTransform(true);

    QImage img;
    QSize originalSize = reader.size();
    if (!originalSize.isValid()) originalSize = QSize(1, 1);

    int maxWidth = 232;
    int maxHeight = 218;

    QSize scaledSize = originalSize;
    scaledSize.scale(maxWidth, maxHeight, Qt::KeepAspectRatio);

    reader.setScaledSize(scaledSize);
    img = reader.read();

    if (!img.isNull()) {
        emit loadComplete(id, std::move(QPixmap::fromImage(img)));
    } else {
        qWarning() << "Failed to load image from" << filePathStr;
    }
}