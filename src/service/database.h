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
    PixivAuthorID,
    PixivAuthorName,
    PixivTitle,
    PixivDescription,
    TweetID,
    TweetAuthorID,
    TweetAuthorName,
    TweetAuthorNick,
    TweetDescription
};

class PicDatabase{
public:
    PicDatabase(const QString& connectionName = QString(), const QString& databaseFile=QString("database.db"));
    ~PicDatabase();
    
    // getters  TODO: batch get and merge SQL queries
    PicInfo getPicInfo(uint64_t id, int64_t tweetID=0, int64_t pixivID=0) const;
    TweetInfo getTweetInfo(int64_t tweetID) const;
    PixivInfo getPixivInfo(int64_t pixivID) const;
    std::vector<std::pair<std::string, int>> getGeneralTags() const;
    std::vector<std::pair<std::string, int>> getCharacterTags() const;
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
    // insert pixiv
    bool insertPixivInfo(const PixivInfo& pixivInfo);
    bool insertPixivArtwork(const PixivInfo& pixivInfo);
    bool insertPixivArtworkTags(const PixivInfo& pixivInfo);
    // update pixiv
    bool updatePixivInfo(const PixivInfo& pixivInfo);
    bool updatePixivArtwork(const PixivInfo& pixivInfo);
    bool updatePixivArtworkTags(const PixivInfo& pixivInfo);
    
    // search functions TODO: merge SQL queries
    std::vector<uint64_t> tagSearch(const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> pixivTagSearch(const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags);
    std::vector<uint64_t> tweetHashtagSearch(const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags);
    std::unordered_map<uint64_t, int64_t> textSearch(const std::string& searchText, SearchField searchField);

    void processSingleFile(const std::filesystem::path& path, ParserType parserType=ParserType::None);
    void scanDirectory(const std::filesystem::path& directory, ParserType parserType=ParserType::None);// TODO: multithreading?
    void syncTables(); // call this after scanDirectory, sync x_restrict and ai_type from pixiv to pictures, count tags
private:
    QSqlDatabase database; //SQLite
    QString connectionName;
    std::unordered_map<std::string, int> tagToId;
    std::unordered_map<std::string, int> twitterHashtagToId;
    std::unordered_map<std::string, int> pixivTagToId;
    std::unordered_map<int, std::string> idToTag;
    std::unordered_map<int, std::string> idToTwitterHashtag;
    std::unordered_map<int, std::string> idToPixivTag;
    std::unordered_set<int64_t> newPixivIDs;
    
    void initDatabase(QString databaseFile);
    bool createTables(); // TODO: Perceptual hash((tag, xrestrict, ai_type)(picture, phash)), FTS5
    bool setupFTS5();
    void getTagMapping();
};

#endif