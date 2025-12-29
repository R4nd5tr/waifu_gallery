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

    void searchPics(const std::unordered_set<std::string>& includedTags,
                    const std::unordered_set<std::string>& excludedTags,
                    const std::unordered_set<std::string>& includedPixivTags,
                    const std::unordered_set<std::string>& excludedPixivTags,
                    const std::unordered_set<std::string>& includedTweetTags,
                    const std::unordered_set<std::string>& excludedTweetTags,
                    const std::string& searchText,
                    SearchField searchField,
                    bool selectedTagChanged,
                    bool selectedPixivTagChanged,
                    bool selectedTweetTagChanged,
                    bool searchTextChanged,
                    size_t requestId);
    void reloadDatabase() { database.reloadDatabase(); };
signals:
    void searchComplete(const std::vector<PicInfo>& resultPics,
                        std::vector<std::tuple<std::string, int, bool>> availableTags,
                        std::vector<std::pair<std::string, int>> availablePixivTags,
                        std::vector<std::pair<std::string, int>> availableTwitterHashtags,
                        size_t requestId);

private:
    PicDatabase database;

    std::vector<uint64_t> lastTagSearchResult;
    std::vector<uint64_t> lastPixivTagSearchResult;
    std::vector<uint64_t> lastTweetTagSearchResult;
    std::vector<uint64_t> lastTextSearchResult;
};
