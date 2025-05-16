#include <cstdint>
#include <set>
#include <vector>
#include <string>

struct PicInfo{
    uint64_t id;     //xxhash64
    std::set<std::pair<uint64_t, int>> tweetIdIndices; //(tweetID, index)
    std::set<std::pair<uint64_t, int>> pixivIdIndices; //(pixivID, index)
    std::set<std::pair<std::string, uint8_t>> tags; //(tag, source)
    std::set<std::string> filePaths;
    uint32_t width;
    uint32_t height;
    uint32_t size;
    uint8_t xRestrict;

    std::string getFileType() const;
    float getRatio() const;
};

struct TweetInfo{
    uint64_t tweetID;
    std::string date;
    uint32_t authorID;
    std::string authorName;
    std::string authorNick;
    std::string authorDescription;
    uint32_t favoriteCount;
    uint32_t quoteCount;
    uint32_t replyCount;
    uint32_t retweetCount;
    uint32_t bookmarkCount;
    std::set<std::string> hashtags;
    std::string description;
};

struct PixivInfo{
    uint64_t pixivID;
    std::string date;
    std::vector<std::string> tags;
    std::vector<std::string> tagsTransl;
    std::string authorName;
    uint32_t authorID;
    std::string title;
    std::string description;
    uint32_t likeCount;
    uint32_t viewCount;
    uint8_t xRestrict;
};
