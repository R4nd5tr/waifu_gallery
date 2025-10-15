#pragma once
#include "model.h"
#include "parser.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using ProgressCallback = std::function<void(size_t processed, size_t total)>;

enum class SearchField {
    None,
    PicID,
    PixivID,
    PixivAuthorID,
    PixivAuthorName,
    PixivTitle,
    TweetID,
    TweetAuthorID,
    TweetAuthorName,
    TweetAuthorNick
};

class PicDatabase {
public:
    PicDatabase(const std::string& databaseFile = "database.db", bool importOnly = false);
    ~PicDatabase();

    void enableForeignKeyRestriction() const;
    void disableForeignKeyRestriction() const;
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // getters  TODO: batch get and merge SQL queries
    PicInfo getPicInfo(uint64_t id, int64_t tweetID = 0, int64_t pixivID = 0) const;
    TweetInfo getTweetInfo(int64_t tweetID) const;
    PixivInfo getPixivInfo(int64_t pixivID) const;
    std::vector<std::tuple<std::string, int, bool>> getTags() const; // (tag, count, isCharacter)
    std::vector<std::pair<std::string, int>> getTwitterHashtags() const;
    std::vector<std::pair<std::string, int>> getPixivTags() const;
    // insert picture
    bool insertPicInfo(const PicInfo& picInfo);
    bool insertPicture(const PicInfo& picInfo);
    bool insertPictureFilePath(const PicInfo& picInfo);
    bool insertPictureTags(const PicInfo& picInfo);
    bool insertPicturePixivId(const PicInfo& picInfo);
    bool insertPictureTweetId(const PicInfo& picInfo);
    // insert tweet
    bool insertTweetInfo(const TweetInfo& tweetInfo);
    bool insertTweet(const TweetInfo& tweetInfo);
    bool insertTweetHashtags(const TweetInfo& tweetInfo);
    // insert pixiv (for text metadata files)
    bool insertPixivInfo(const PixivInfo& pixivInfo);
    bool insertPixivArtwork(const PixivInfo& pixivInfo);
    bool insertPixivArtworkTags(const PixivInfo& pixivInfo);
    // update pixiv (for csv/json files)
    bool updatePixivInfo(const PixivInfo& pixivInfo);
    bool updatePixivArtwork(const PixivInfo& pixivInfo);
    bool updatePixivArtworkTags(const PixivInfo& pixivInfo);

    // search functions TODO: merge SQL queries
    std::vector<uint64_t> tagSearch(const std::unordered_set<std::string>& includedTags,
                                    const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> pixivTagSearch(const std::unordered_set<std::string>& includedTags,
                                         const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> tweetHashtagSearch(const std::unordered_set<std::string>& includedTags,
                                             const std::unordered_set<std::string>& excludedTags);
    std::unordered_map<uint64_t, int64_t> textSearch(const std::string& searchText, SearchField searchField);

    // import functions
    void importFilesFromDirectory(const std::filesystem::path& directory,
                                  ParserType parserType = ParserType::None,
                                  ProgressCallback progressCallback = nullptr);
    void processAndImportSingleFile(const std::filesystem::path& path, ParserType parserType = ParserType::None);

    // call this after scanDirectory, sync x_restrict and ai_type from pixiv to pictures, count tags
    void syncTables(std::unordered_set<int64_t> newPixivIDs = {});
    std::unordered_set<int64_t> newPixivIDs; // to be used between import and syncTables

private:
    sqlite3* db = nullptr;
    std::unordered_map<std::string, int> tagToId;
    std::unordered_map<std::string, int> twitterHashtagToId;
    std::unordered_map<std::string, int> pixivTagToId;
    std::unordered_map<int, std::string> idToTag;
    std::unordered_map<int, std::string> idToTwitterHashtag;
    std::unordered_map<int, std::string> idToPixivTag;

    void initDatabase(const std::string& databaseFile);
    bool createTables(); // TODO: Perceptual hash((tag, xrestrict, ai_type)(picture, phash))
    void getTagMapping();

    bool execute(const std::string& sql) const {
        char* errorMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "Error executing SQL: " << errorMsg << std::endl;
            sqlite3_free(errorMsg);
            return false;
        }
        return true;
    }
    sqlite3_stmt* prepare(const std::string& sql) const {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << std::endl;
            return nullptr;
        }
        return stmt;
    }
};

class MultiThreadedImporter {
public:
    MultiThreadedImporter(const std::filesystem::path& directory,
                          size_t threadCount = std::thread::hardware_concurrency(),
                          ProgressCallback progressCallback = nullptr,
                          ParserType parserType = ParserType::None,
                          std::string dbFile = "database.db");
    ~MultiThreadedImporter();

    bool finish();
    void forceStop();

private:
    bool finished = false;

    ProgressCallback progressCallback; // this will be called in insert thread, make sure it's thread safe
    ParserType parserType;
    std::string dbFile;

    std::vector<std::thread> workers;
    std::vector<std::filesystem::path> files;
    std::atomic<size_t> nextFileIndex = 0;

    std::thread insertThread;
    std::queue<PicInfo> picQueue;
    std::mutex picMutex;
    std::queue<TweetInfo> tweetQueue;
    std::mutex tweetMutex;
    std::queue<PixivInfo> pixivQueue;
    std::mutex pixivMutex;
    std::queue<std::vector<PixivInfo>> pixivVecQueue;
    std::mutex pixivVecMutex;

    std::atomic<bool> stopFlag = false;
    std::condition_variable cv;

    void workerThreadFunc();
    void insertThreadFunc();
};
