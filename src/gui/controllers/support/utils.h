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
#include "service/model.h"
#include "service/parser.h"
#include <QString>

static const std::vector<QString> PLATFORM_TYPE_STRS = {"Unknown", "pixiv", "twitter"};
static const std::vector<QString> PARSER_TYPE_STRS = {"None", "Powerful Pixiv Downloader", "gallery-dl Twitter"};

inline QString platformTypeToString(PlatformType platform) {
    return PLATFORM_TYPE_STRS[static_cast<int>(platform)];
}

inline QString parserTypeToString(ParserType parser) {
    return PARSER_TYPE_STRS[static_cast<int>(parser)];
}
