#ifndef MODEL_H
#define MODEL_H

#include <cstdint>
#include <set>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_set>

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

struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        std::size_t h1 = std::hash<T1>{}(p.first);
        std::size_t h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

struct TweetInfo{
    int64_t tweetID;
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
    std::unordered_set<std::string> hashtags;
    std::string description;
};

struct PixivInfo{
    int64_t pixivID;
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

struct PicInfo{
    uint64_t id;     //xxhash64
    std::unordered_set<std::pair<int64_t, int>, PairHash> tweetIdIndices; //(tweetID, index)
    std::unordered_set<std::pair<int64_t, int>, PairHash> pixivIdIndices; //(pixivID, index)
    std::unordered_set<std::string> tags;
    std::unordered_set<std::filesystem::path> filePaths; // identical file can appear in multiple locations
    std::vector<TweetInfo> tweetInfo;
    std::vector<PixivInfo> pixivInfo;
    uint32_t width;
    uint32_t height;
    uint32_t size;
    std::string fileType;
    XRestrictType xRestrict = XRestrictType::Unknown;

    float getRatio() const;
};
#endif