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

#include "model.h"
#include <cstdint>
#include <string>
#include <vector>

enum class ParserType { None, PowerfulPixivDownloader, GallerydlTwitter };

struct ParsedMetadata {
    PlatformType platformType = PlatformType::Unknown;
    int64_t id = 0;
    std::string date;

    int64_t authorID;
    std::string authorName;
    std::string authorNick;
    std::string authorDescription;

    std::string title;
    std::string description;
    std::vector<std::string> tags;
    std::vector<std::string> tagsTransl;

    uint32_t viewCount = 0;
    uint32_t likeCount = 0;
    uint32_t bookmarkCount = 0;
    uint32_t replyCount = 0;
    uint32_t forwardCount = 0;
    uint32_t quoteCount = 0;

    RestrictType restrictType = RestrictType::Unknown;
    AIType aiType = AIType::Unknown;

    bool updateIfExists = false;
};

struct ParsedPicture {
    uint64_t id = 0;

    uint32_t width;
    uint32_t height;
    uint32_t size;

    ImageFormat fileType = ImageFormat::Unknown;
    std::filesystem::path filePath;

    std::string editTime;
    std::string downloadTime;

    ImageSource identifier;

    RestrictType restrictType = RestrictType::Unknown;
};

ParsedPicture parsePicture(const std::filesystem::path& pictureFilePath, ParserType parserType = ParserType::None);

std::vector<ParsedMetadata> powerfulPixivDownloaderMetadataParser(const std::filesystem::path& metadataFilePath);

ParsedMetadata gallerydlTwitterMetadataParser(const std::filesystem::path& metadataFilePath);
