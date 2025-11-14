/*
 * Waifu Gallery - A Qt-based anime illustration gallery application.
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

enum class ParserType { None, Pixiv, Twitter };

PicInfo parsePicture(const std::filesystem::path& pictureFilePath, ParserType parserType = ParserType::None);

PixivInfo parsePixivMetadata(const std::filesystem::path& pixivMetadataFilePath);
std::vector<PixivInfo> parsePixivCsv(const std::filesystem::path& pixivCsvFilePath);
std::vector<PixivInfo> parsePixivJson(const std::filesystem::path& pixivJsonFilePath);

TweetInfo parseTweetJson(const std::filesystem::path& tweetJsonFilePath);
