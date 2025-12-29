/*
 * Waifu Gallery - A anime illustration gallery application.
 * Copyright (C) 2025 R4nd5tr <r4nd5tr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// refactor the data model in the future?
// use one struct to represent social media info (tweet, pixiv, etc.) and use feature hash for primary key?

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

struct PicTag {
    std::string tag;
    bool isCharacter;
    float probability; // confidence score for the tag
};

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
    std::vector<PicInfo> pics; // optional information about pictures in this tweet
};

struct PixivInfo { // represents one pixiv artwork
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
    std::vector<PicInfo> pics; // optional information about pictures in this artwork
};

struct PicInfo {     // represents one image file
    uint64_t id = 0; // xxhash64
    uint32_t width;
    uint32_t height;
    uint32_t size;
    ImageFormat fileType = ImageFormat::Unknown;
    std::string editTime;                            // last modified time of the file
    std::string downloadTime;                        // ISO 8601 format time
    std::string phash;                               // perceptual hash for image similarity search
    std::unordered_map<int64_t, int> tweetIdIndices; //(tweetID, index)
    std::unordered_map<int64_t, int> pixivIdIndices; //(pixivID, index)
    std::vector<PicTag> tags;
    std::unordered_set<std::filesystem::path> filePaths; // identical file can appear in multiple locations
    std::vector<TweetInfo> tweetInfo;                    // optional information about tweets containing this picture
    std::vector<PixivInfo> pixivInfo;                    // optional information about pixiv artworks containing this picture
    RestrictType xRestrict = RestrictType::Unknown;
    AIType aiType = AIType::Unknown;

    float getRatio() const {
        if (height > 0) {
            return static_cast<float>(width) / static_cast<float>(height);
        }
        return 0.0f;
    };
};
