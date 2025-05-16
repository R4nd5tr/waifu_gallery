#ifndef DATABASE_H
#define DATABASE_H
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <QtSql/QSqlDatabase>
#include <QString>

class PicDatabase{
public:
    PicDatabase(QString databaseFile = "database.db");
    ~PicDatabase();
    const PicInfo getPicInfo(uint64_t id);
    const TweetInfo getTweetInfo(uint64_t tweetID);
    const PixivInfo getPixivInfo(uint64_t pixivID);

    bool insertPicInfo(const PicInfo& picInfo);
    bool insertPicture(const PicInfo& picInfo);
    bool insertPictureFilePath(const PicInfo& picInfo);
    bool insertPictureTags(const PicInfo& picInfo);

    bool insertTweetInfo(const TweetInfo& tweetInfo);
    bool insertTweet(const TweetInfo& tweetInfo);
    bool insertTweetHashtags(const TweetInfo& tweetInfo);
    
    bool insertPixivInfo(const PixivInfo& pixivInfo);
    bool insertPixivArtwork(const PixivInfo& pixivInfo);
    bool insertPixivArtworkTags(const PixivInfo& pixivInfo);
private:
    QSqlDatabase database;  //SQLite

    void initDatabase(QString databaseFile);
    bool createTables();
};

struct PicInfo{
    uint64_t id;     //xxhash64
    uint64_t tweetID;//TODO: duplicate id
    uint tweetNum;
    uint32_t pixivID;//TODO: duplicate id
    uint pixivNum;
    std::unordered_map<std::string, char> tags; //{tag, source}
    std::unordered_set<std::string> filePaths;
    uint width;
    uint height;
    uint size;
    char xRestrict;

    const std::string getFileType();
    const float getRatio();
};

struct TweetInfo{
    uint64_t tweetID;
    std::string date;
    uint32_t authorID;
    std::string authorName;
    std::string authorNick;
    std::string authorDescription;
    uint favoriteCount;
    uint quoteCount;
    uint replyCount;
    uint retweetCount;
    uint bookmarkCount;
    std::unordered_set<std::string> hashtags;
    std::string description;
};

struct PixivInfo{
    uint32_t pixivID;
    std::string date;
    std::vector<std::string> tags;
    std::vector<std::string> tagsTransl;
    std::string authorName;
    uint32_t authorID;
    std::string title;
    std::string description;
    uint likeCount;
    uint viewCount;
    char xRestrict;
};

#endif