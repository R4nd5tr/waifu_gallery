#pragma once
#include "model.h"
#include "parser.h"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    PicDatabase(const std::string& databaseFile = "database.db");
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

    void processAndImportSingleFile(const std::filesystem::path& path, ParserType parserType = ParserType::None);
    // import functions (only used in testing)
    void importFilesFromDirectory(const std::filesystem::path& directory, ParserType parserType = ParserType::None);

    void syncTables(); // call this after scanDirectory, sync x_restrict and ai_type from pixiv to pictures, count tags
private:
    sqlite3* db = nullptr;
    std::unordered_map<std::string, int> tagToId;
    std::unordered_map<std::string, int> twitterHashtagToId;
    std::unordered_map<std::string, int> pixivTagToId;
    std::unordered_map<int, std::string> idToTag;
    std::unordered_map<int, std::string> idToTwitterHashtag;
    std::unordered_map<int, std::string> idToPixivTag;
    std::unordered_set<int64_t> newPixivIDs;

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
