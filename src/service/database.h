#ifndef DATABASE_H
#define DATABASE_H
#include<unordered_set>
#include<cstdint>
#include<QtSql/QSqlDatabase>
#include<QString>

class PicDatabase{
public:
    PicDatabase(QString databaseFile = "database.db");
    ~PicDatabase();
    const PicInfo getPicInfo(uint64_t id);
    const TweetInfo getTweetInfo(uint64_t tweetID);
    const PixivInfo getPixivInfo(uint64_t pixivID);
    bool insertPicInfo(PicInfo picInfo);
    bool insertTweetInfo(TweetInfo tweetInfo);
    bool insertPixivInfo(PixivInfo pixivInfo);
private:
    QSqlDatabase database;  //SQLite

    void initDatabase(QString databaseFile);
    bool createTables();

};

struct PicInfo{
    uint64_t id;     //xxhash64
    uint64_t tweetID;
    uint tweetNum;
    uint32_t pixivID;
    uint pixivNum;
    std::unordered_set<std::string> tags;
    std::string directory;
    std::string fileName;
    std::string fileType;
    uint width;
    uint height;
    uint size;
    char xRestrict;

    const std::string getFilePath();
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
    std::unordered_set<std::string> tags;
    std::unordered_set<std::string> tagsTransl;
    std::string authorName;
    uint32_t authorID;
    std::string title;
    std::string description;
    uint likeCount;
    uint viewCount;
    char xRestrict;
};

#endif