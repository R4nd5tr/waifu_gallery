#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class RestrictType { // keep in sync with autotagger module
    Unknown,
    AllAges, // rating: General
    Sensitive,
    Questionable,
    R18, // rating: Explicit
    R18G
};
enum class AIType { Unknown, NotAI, AI };
enum class ImageFormat { Unknown, JPG, PNG, GIF, WebP };

struct PicInfo;

struct TweetInfo { // represents one tweet
    int64_t tweetID = 0;
    std::string date;
    int64_t authorID;
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
    std::vector<PicInfo> pics;
};

struct PixivInfo { // represents one pixiv illustration
    int64_t pixivID = 0;
    std::string date;
    std::vector<std::string> tags;
    std::vector<std::string> tagsTransl;
    std::string authorName;
    int64_t authorID;
    std::string title;
    std::string description;
    uint32_t likeCount = 0;
    uint32_t viewCount = 0;
    RestrictType xRestrict = RestrictType::Unknown;
    AIType aiType = AIType::Unknown;
    std::vector<PicInfo> pics;
};

struct PicInfo {                                         // represents one image file
    uint64_t id = 0;                                     // xxhash64
    std::unordered_map<int64_t, int> tweetIdIndices;     //(tweetID, index)
    std::unordered_map<int64_t, int> pixivIdIndices;     //(pixivID, index)
    std::unordered_map<std::string, bool> tags;          // tag -> isCharacter
    std::unordered_set<std::filesystem::path> filePaths; // identical file can appear in multiple locations
    std::vector<TweetInfo> tweetInfo;
    std::vector<PixivInfo> pixivInfo;
    uint32_t width;
    uint32_t height;
    uint32_t size;
    ImageFormat fileType = ImageFormat::Unknown;
    RestrictType xRestrict = RestrictType::Unknown;
    AIType aiType = AIType::Unknown;

    float getRatio() const;
};
