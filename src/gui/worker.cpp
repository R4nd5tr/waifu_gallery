#include "../service/database.h"
#include "worker.h"
#include <QImageReader>

DatabaseWorker::DatabaseWorker(const QString &connectionName, QObject *parent):
    QObject(parent), database(connectionName) {}
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
    std::unordered_map<std::string, int> tagCount;
    std::unordered_map<std::string, int> pixivTagCount;
    std::unordered_map<std::string, int> tweetTagCount;
    for (const auto& pic : resultPics) {
        for (const auto& tag : pic.tags) {
            tagCount[tag]++;
        }
        for (const auto& pixivInfo : pic.pixivInfo) {
            for (const auto& tag : pixivInfo.tags) {
                pixivTagCount[tag]++;
            }
        }
        for (const auto& tweetInfo : pic.tweetInfo) {
            for (const auto& tag : tweetInfo.hashtags) {
                tweetTagCount[tag]++;
            }
        }
    }
    std::vector<std::pair<std::string, int>> availableTags(tagCount.begin(), tagCount.end());
    std::vector<std::pair<std::string, int>> availablePixivTags(pixivTagCount.begin(), pixivTagCount.end());
    std::vector<std::pair<std::string, int>> availableTwitterHashtags(tweetTagCount.begin(), tweetTagCount.end());
    std::sort(availableTags.begin(), availableTags.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    std::sort(availablePixivTags.begin(), availablePixivTags.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    std::sort(availableTwitterHashtags.begin(), availableTwitterHashtags.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    emit searchComplete(std::move(resultPics), 
                        availableTags, availablePixivTags, availableTwitterHashtags,
                        requestId);
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