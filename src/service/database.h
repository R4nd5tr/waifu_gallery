#ifndef DATABASE_H
#define DATABASE_H
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <QtSql/QSqlDatabase>
#include <QString>
#include "model.h"
#include "parser.h"

enum class SearchField {
    None,
    PicID,
    PixivID,
    PixivUserID,
    PixivUserName,
    PixivTitle,
    PixivDescription,
    TweetID,
    TweetUserID,
    TweetUserName,
    TweetUserNick,
    TweetDescription
};

class PicDatabase{
public:
    PicDatabase(const QString& connectionName = QString(), const QString& databaseFile="database.db");
    ~PicDatabase();
    PicInfo getPicInfo(uint64_t id) const;
    TweetInfo getTweetInfo(int64_t tweetID) const;
    PixivInfo getPixivInfo(int64_t pixivID) const;
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
    // insert pixiv
    bool insertPixivInfo(const PixivInfo& pixivInfo);
    bool insertPixivArtwork(const PixivInfo& pixivInfo);
    bool insertPixivArtworkTags(const PixivInfo& pixivInfo);
    // update pixiv
    bool updatePixivInfo(const PixivInfo& pixivInfo);
    bool updatePixivArtwork(const PixivInfo& pixivInfo);
    bool updatePixivArtworkTags(const PixivInfo& pixivInfo);

    // search functions
    std::vector<uint64_t> tagSearch(const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> pixivTagSearch(const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> tweetHashtagSearch(const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> textSearch(const std::string& searchText, SearchField searchField);// TODO: FTS5

    void processSingleFile(const std::filesystem::path& path);
    void scanDirectory(const std::filesystem::path& directory);// TODO: multithreading?
private:
    QSqlDatabase database; //SQLite
    QString connectionName;

    void initDatabase(QString databaseFile);
    bool createTables(); // TODO: Perceptual hash((tag, xrestrict, ai_type)(picture, phash)), FTS5, tag table
    void syncTables(); // call this after scanDirectory TODO: sync x_restrict and ai_type from pixiv to pictures, count tags
};

#endif