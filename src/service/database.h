#ifndef DATABASE_H
#define DATABASE_H
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <QtSql/QSqlDatabase>
#include <QString>
#include "model.h"

class PicDatabase{
public:
    PicDatabase(QString databaseFile = "database.db");
    ~PicDatabase();
    PicInfo getPicInfo(uint64_t id) const;
    TweetInfo getTweetInfo(uint64_t tweetID) const;
    PixivInfo getPixivInfo(uint64_t pixivID) const;

    bool insertPicInfo(const PicInfo& picInfo); //transaction
    bool insertPicture(const PicInfo& picInfo);
    bool insertPictureFilePath(const PicInfo& picInfo);
    bool insertPictureTags(const PicInfo& picInfo);
    bool insertPicturePixivId(const PicInfo& picInfo);
    bool insertPictureTweetId(const PicInfo& picInfo);
    
    bool insertTweetInfo(const TweetInfo& tweetInfo); //transaction
    bool insertTweet(const TweetInfo& tweetInfo);
    bool insertTweetHashtags(const TweetInfo& tweetInfo);
    
    bool insertPixivInfo(const PixivInfo& pixivInfo); //transaction
    bool insertPixivArtwork(const PixivInfo& pixivInfo);
    bool insertPixivArtworkTags(const PixivInfo& pixivInfo);
private:
    QSqlDatabase database; //SQLite

    void initDatabase(QString databaseFile);
    bool createTables();
};

#endif