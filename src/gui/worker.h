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
#include "widgets/picture_frame.h"
#include <QObject>
#include <QPixmap>
#include <filesystem>

class DatabaseWorker : public QObject { // database search worker in another thread
    Q_OBJECT
public:
    explicit DatabaseWorker(QObject* parent = nullptr);
    ~DatabaseWorker();

    void searchPics(const std::unordered_set<uint32_t>& includedTags,
                    const std::unordered_set<uint32_t>& excludedTags,
                    const std::unordered_set<uint32_t>& includedPlatformTags,
                    const std::unordered_set<uint32_t>& excludedPlatformTags,
                    PlatformType platform,
                    SearchField searchField,
                    const std::string& searchText,
                    size_t requestId);
    void reloadDatabase() { database.reloadDatabase(); };

signals:
    void searchComplete(const std::vector<PicInfo>& pics,
                        std::vector<TagCount> availableTags,
                        std::vector<PlatformTagCount> availablePlatformTags,
                        size_t requestId);

private:
    PicDatabase database;

    std::unordered_set<uint32_t> lastIncludedTags;
    std::unordered_set<uint32_t> lastExcludedTags;
    std::unordered_set<uint32_t> lastIncludedPlatformTags;
    std::unordered_set<uint32_t> lastExcludedPlatformTags;
    PlatformType lastPlatformType = PlatformType::Unknown;
    SearchField lastSearchField = SearchField::None;
    std::string lastSearchText;

    std::vector<uint64_t> lastTagSearchResult;
    std::vector<PlatformID> lastPlatformTagSearchResult;
    std::vector<PlatformID> lastTextSearchResult;
};
