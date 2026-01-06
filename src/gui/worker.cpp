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

#include "worker.h"
#include "service/database.h"
#include <QImageReader>

// utility function to get PicInfo from Metadata
std::vector<PicInfo> getPicInfoFromMetadata(const Metadata& metadata) {
    std::vector<PicInfo> pics;
    Metadata associatedMetadata = metadata;
    associatedMetadata.associatedPics.clear(); // avoid circular reference
    for (const auto& associatePic : metadata.associatedPics) {
        PicInfo pic = associatePic;
        pic.associatedMetadata.push_back(associatedMetadata);
        pics.push_back(pic);
    }
    return pics;
}

DatabaseWorker::DatabaseWorker(QObject* parent) : QObject(parent), database{DbMode::Query} { // search worker
}
DatabaseWorker::~DatabaseWorker() {}
void DatabaseWorker::searchPics(const std::unordered_set<uint32_t>& includedTags,
                                const std::unordered_set<uint32_t>& excludedTags,
                                const std::unordered_set<uint32_t>& includedPlatformTags,
                                const std::unordered_set<uint32_t>& excludedPlatformTags,
                                PlatformType platform,
                                SearchField searchField,
                                const std::string& searchText,
                                size_t requestId) { // TODO: support display metadata instead of pics directly in the future

    if (includedTags != lastIncludedTags || excludedTags != lastExcludedTags) {
        lastTagSearchResult = database.tagSearch(includedTags, excludedTags);
    }
    if (includedPlatformTags != lastIncludedPlatformTags || excludedPlatformTags != lastExcludedPlatformTags) {
        lastPlatformTagSearchResult = database.platformTagSearch(includedPlatformTags, excludedPlatformTags);
    }
    if (platform != lastPlatformType || searchField != lastSearchField || searchText != lastSearchText) {
        lastTextSearchResult = database.textSearch(searchText, platform, searchField);
    }
    std::unordered_map<uint64_t, PicInfo> platformTagSearchResultPics;
    for (const auto& platformID : lastPlatformTagSearchResult) {
        Metadata metadata = database.getMetadata(platformID, true);
        std::vector<PicInfo> pics = getPicInfoFromMetadata(metadata);
        for (const auto& pic : pics) {
            platformTagSearchResultPics[pic.id] = pic;
        }
    }
    std::unordered_map<uint64_t, PicInfo> textSearchResultPics;
    for (const auto& platformID : lastTextSearchResult) {
        Metadata metadata = database.getMetadata(platformID, true);
        std::vector<PicInfo> pics = getPicInfoFromMetadata(metadata);
        for (const auto& pic : pics) {
            textSearchResultPics[pic.id] = pic;
        }
    }

    // intersect all search results
    std::vector<PicInfo> resultPics;
    if (!lastTagSearchResult.empty()) {
        for (const auto& id : lastTagSearchResult) {
            bool inTextSearch = textSearchResultPics.find(id) != textSearchResultPics.end();
            bool inPlatformTagSearch = platformTagSearchResultPics.find(id) != platformTagSearchResultPics.end();

            if ((!textSearchResultPics.empty() && !inTextSearch) ||
                (!platformTagSearchResultPics.empty() && !inPlatformTagSearch)) {
                continue;
            }

            if (inPlatformTagSearch) { // use existing PicInfo to avoid duplicate database queries
                resultPics.push_back(platformTagSearchResultPics[id]);
            } else if (inTextSearch) {
                resultPics.push_back(textSearchResultPics[id]);
            } else {
                resultPics.push_back(database.getPicInfo(id));
            }
        }
    } else if (!platformTagSearchResultPics.empty()) {
        for (const auto& [id, pic] : platformTagSearchResultPics) {
            if (!textSearchResultPics.empty() && textSearchResultPics.find(id) == textSearchResultPics.end()) {
                continue;
            }
            resultPics.push_back(pic);
        }
    } else if (!textSearchResultPics.empty()) {
        for (const auto& [id, pic] : textSearchResultPics) {
            resultPics.push_back(pic);
        }
    } else {
        // no search criteria, return empty result
        emit searchComplete(resultPics, {}, {}, requestId);
        return;
    }

    // gather available tags from resultPics
    std::unordered_map<uint32_t, int> tagCount;
    std::unordered_map<uint32_t, int> platformTagCount;
    std::unordered_set<int64_t> countedMetadata; // to avoid double counting metadata tags
    for (const auto& pic : resultPics) {
        for (const auto& picTag : pic.tags) {
            tagCount[picTag.tagId]++;
        }
        for (const auto& metadata : pic.associatedMetadata) {
            if (countedMetadata.find(metadata.id) != countedMetadata.end()) continue;
            countedMetadata.insert(metadata.id);
            for (const auto& tagId : metadata.tagIds) {
                platformTagCount[tagId]++;
            }
        }
    }

    std::vector<TagCount> availableTags;
    std::vector<PlatformTagCount> availablePlatformTags;
    for (const auto& [tagId, count] : tagCount) {
        if (std::find(includedTags.begin(), includedTags.end(), tagId) != includedTags.end() ||
            std::find(excludedTags.begin(), excludedTags.end(), tagId) != excludedTags.end()) {
            continue; // skip tags already in filter
        }
        TagCount tagCountEntry;
        tagCountEntry.tag = database.getStringTag(tagId);
        tagCountEntry.tagId = tagId;
        tagCountEntry.count = count;
        availableTags.push_back(tagCountEntry);
    }
    for (const auto& [tagId, count] : platformTagCount) {
        if (std::find(includedPlatformTags.begin(), includedPlatformTags.end(), tagId) != includedPlatformTags.end() ||
            std::find(excludedPlatformTags.begin(), excludedPlatformTags.end(), tagId) != excludedPlatformTags.end()) {
            continue; // skip tags already in filter
        }
        PlatformTagCount tagCountEntry;
        tagCountEntry.tag = database.getPlatformStringTag(tagId);
        tagCountEntry.tagId = tagId;
        tagCountEntry.count = count;
        availablePlatformTags.push_back(tagCountEntry);
    }
    std::sort(availableTags.begin(), availableTags.end(), [](const TagCount& a, const TagCount& b) { return b.count < a.count; });
    std::sort(availablePlatformTags.begin(),
              availablePlatformTags.end(),
              [](const PlatformTagCount& a, const PlatformTagCount& b) { return b.count < a.count; });

    emit searchComplete(resultPics, availableTags, availablePlatformTags, requestId);
}
