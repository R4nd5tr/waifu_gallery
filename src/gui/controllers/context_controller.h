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
#include "service/database.h"
#include "service/model.h"
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>

enum class SortBy { None, ID, DownloadDate, EditDate, Size, Filename, Width, Height, Ratio };

enum class SortOrder { Ascending, Descending };

struct FilterContext {
    bool showUnknowPlatform = true;
    bool showPixiv = true;
    bool showTwitter = true;

    bool showPNG = true;
    bool showJPG = true;
    bool showGIF = true;
    bool showWEBP = true;

    bool showUnknowRestrict = true;
    bool showAllAge = true;
    bool showSensitive = false;
    bool showQuestionable = false;
    bool showR18 = false;
    bool showR18G = false;

    bool showUnknowAI = true;
    bool showAI = true;
    bool showNonAI = true;

    uint32_t maxHeight = std::numeric_limits<uint32_t>::max();
    uint32_t minHeight = 0;
    uint32_t maxWidth = std::numeric_limits<uint32_t>::max();
    uint32_t minWidth = 0;
};
struct SortContext {
    SortBy sortBy = SortBy::None;
    SortOrder sortOrder = SortOrder::Ascending;
    float ratio = 1.0f;
};
struct SearchContext {
    PlatformType searchPlatform = PlatformType::Unknown;
    SearchField searchField = SearchField::None;
    std::string searchText;
    std::unordered_set<uint32_t> includedTags;
    std::unordered_set<uint32_t> excludedTags;
    std::unordered_set<uint32_t> includedPlatformTags;
    std::unordered_set<uint32_t> excludedPlatformTags;
};
