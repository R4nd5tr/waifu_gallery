#ifndef MODEL_H
#define MODEL_H

#include <cstdint>
#include <set>
#include <vector>
#include <string>

enum class XRestrictType {
    Unknown,
    AllAges,
    R18,
    R18G
};
enum class AIType {
    Unknown,
    NotAI,
    AI
};

struct PicInfo{
    uint64_t id;     //xxhash64
    std::set<std::pair<uint64_t, int>> tweetIdIndices; //(tweetID, index)
    std::set<std::pair<uint64_t, int>> pixivIdIndices; //(pixivID, index)
    std::set<std::pair<std::string, uint8_t>> tags; //(tag, source)
    std::set<std::string> filePaths;
    uint32_t width;
    uint32_t height;
    uint32_t size;
    XRestrictType xRestrict = XRestrictType::Unknown;

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
    std::string authorProfileImage;
    std::string authorProfileBanner;
    uint32_t favoriteCount = 0;
    uint32_t quoteCount = 0;
    uint32_t replyCount = 0;
    uint32_t retweetCount = 0;
    uint32_t bookmarkCount = 0;
    uint32_t viewCount = 0;
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
    uint32_t likeCount = 0;
    uint32_t viewCount = 0;
    XRestrictType xRestrict = XRestrictType::Unknown;
    AIType aiType = AIType::Unknown;
};

#endif