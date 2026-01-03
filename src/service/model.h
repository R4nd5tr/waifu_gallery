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

#include <array>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class PlatformType { Unknown, Pixiv, Twitter };
enum class RestrictType { // keep in sync with autotagger module
    Unknown,
    AllAges, // General
    Sensitive,
    Questionable,
    R18, // Explicit
    R18G
};
enum class AIType { Unknown, NotAI, AI };
enum class ImageFormat { Unknown, JPG, PNG, GIF, WebP };

struct PicTag {
    uint32_t tagId;
    float probability; // confidence score for the tag
};
struct ImageSource {
    PlatformType platform;
    int64_t platformID;
    uint32_t imageIndex;
};
struct PlatformID {
    PlatformType platform;
    int64_t platformID;
};
struct StringTag {
    std::string tag;
    bool isCharacter;
};
struct PlatformStringTag {
    PlatformType platform;
    std::string tag;
};

struct PicInfo;

struct Metadata { // represents one social media post(one tweet, one pixiv artwork, etc.)
    PlatformType platformType = PlatformType::Unknown;
    int64_t id = 0;
    std::string date;

    int64_t authorID;
    std::string authorName;
    std::string authorNick;
    std::string authorDescription;

    std::string title;
    std::string description;
    std::vector<uint32_t> tagIds;

    uint32_t viewCount = 0;
    uint32_t likeCount = 0;
    uint32_t bookmarkCount = 0;
    uint32_t replyCount = 0;
    uint32_t forwardCount = 0;
    uint32_t quoteCount = 0;

    RestrictType restrictType = RestrictType::Unknown;
    AIType aiType = AIType::Unknown;

    std::vector<PicInfo> associatedPics; // optional pictures associated with this metadata
};

struct PicInfo {     // represents one image file
    uint64_t id = 0; // xxhash64
    uint32_t width;
    uint32_t height;
    uint32_t size;
    ImageFormat fileType = ImageFormat::Unknown;
    std::vector<std::filesystem::path> filePaths; // identical file can appear in multiple locations

    std::string editTime;     // last modified time of the file
    std::string downloadTime; // ISO 8601 format time

    std::vector<PicTag> tags;            // tag IDs from the database
    std::array<uint8_t, 64> featureHash; // perceptual hash for image similarity search

    std::vector<ImageSource> sourceIdentifiers; // source platform identifiers
    RestrictType restrictType = RestrictType::Unknown;
    AIType aiType = AIType::Unknown;

    std::vector<Metadata> associatedMetadata; // optional metadata associated with this picture

    float getRatio() const {
        if (height > 0) {
            return static_cast<float>(width) / static_cast<float>(height);
        }
        return 0.0f;
    };
};
