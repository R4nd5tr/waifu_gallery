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

DatabaseWorker::DatabaseWorker(QObject* parent) : QObject(parent), database{DbMode::Query} { // search worker
}
DatabaseWorker::~DatabaseWorker() {}
void DatabaseWorker::searchPics(const SearchContext& searchCtx, size_t requestId) {

    const std::unordered_set<uint32_t>& includedTags = searchCtx.includedTags;
    const std::unordered_set<uint32_t>& excludedTags = searchCtx.excludedTags;
    const std::unordered_set<uint32_t>& includedPlatformTags = searchCtx.includedPlatformTags;
    const std::unordered_set<uint32_t>& excludedPlatformTags = searchCtx.excludedPlatformTags;
    PlatformType platform = searchCtx.searchPlatform;
    SearchField searchField = searchCtx.searchField;
    const std::string& searchText = searchCtx.searchText;

    bool tagSearchApplied = !includedTags.empty() || !excludedTags.empty();
    bool platformTagSearchApplied = !includedPlatformTags.empty() || !excludedPlatformTags.empty();
    bool textSearchApplied = !searchText.empty() && searchField != SearchField::None;

    // determine display metadata or pics
    DisplayItemType displayType;
    if ((textSearchApplied || platformTagSearchApplied) && !tagSearchApplied) {
        displayType = DisplayItemType::Metadata;
    } else {
        displayType = DisplayItemType::Pic;
    }

    // skip search if criteria unchanged
    // the search feature includes three parts: tag search, platform tag search, text search
    // each part can be cached separately, no need to redo the part if criteria unchanged
    if (includedTags != lastIncludedTags || excludedTags != lastExcludedTags) {
        lastIncludedTags = includedTags;
        lastExcludedTags = excludedTags;
        lastTagSearchResult = database.tagSearch(includedTags, excludedTags);
    }
    if (includedPlatformTags != lastIncludedPlatformTags || excludedPlatformTags != lastExcludedPlatformTags) {
        lastIncludedPlatformTags = includedPlatformTags;
        lastExcludedPlatformTags = excludedPlatformTags;
        lastPlatformTagSearchResult = database.platformTagSearch(includedPlatformTags, excludedPlatformTags);
    }
    if (platform != lastPlatformType || searchField != lastSearchField || searchText != lastSearchText) {
        lastPlatformType = platform;
        lastSearchField = searchField;
        lastSearchText = searchText;
        lastTextSearchResult = database.textSearch(searchText, platform, searchField);
    }

    // intersect all search results
    DisplayItems* displayItems = new DisplayItems();
    if (displayType == DisplayItemType::Metadata) {
        std::vector<PlatformID> intersectedResult;
        if (textSearchApplied && platformTagSearchApplied) {
            const auto& small = lastTextSearchResult.size() < lastPlatformTagSearchResult.size() ? lastTextSearchResult
                                                                                                 : lastPlatformTagSearchResult;
            const auto& large = lastTextSearchResult.size() < lastPlatformTagSearchResult.size() ? lastPlatformTagSearchResult
                                                                                                 : lastTextSearchResult;
            for (const auto& id : small) {
                if (large.find(id) != large.end()) {
                    intersectedResult.push_back(id);
                }
            }
        } else if (textSearchApplied) {
            for (const auto& id : lastTextSearchResult) {
                intersectedResult.push_back(id);
            }
        } else if (platformTagSearchApplied) {
            for (const auto& id : lastPlatformTagSearchResult) {
                intersectedResult.push_back(id);
            }
        }

        displayItems->type = DisplayItemType::Metadata;
        displayItems->metadataItems.reserve(intersectedResult.size());
        displayItems->picItems.reserve(intersectedResult.size());
        size_t metadataIdx = 0;
        size_t picIdx = 0;
        for (const auto& platformID : intersectedResult) {
            auto picIds = database.getMetadataPicIds(platformID);
            displayItems->metadataItems.emplace_back(MetadataItem{database.getMetadata(platformID), picIdx, picIds.size()});
            for (const auto& picId : picIds) {
                displayItems->picItems.emplace_back(PicItem{database.getPicInfo(picId), metadataIdx, 1});
                picIdx++;
            }
            metadataIdx++;
        }
    } else if (displayType == DisplayItemType::Pic) { // tag search is always applied
        std::vector<uint64_t> intersectedResult;
        std::unordered_set<uint64_t> platformTagSearchIntersectedResult;
        std::unordered_set<uint64_t> textSearchIntersectedResult;

        // first intersect tag search result with platform tag and text search result separately
        if (platformTagSearchApplied) {
            std::unordered_set<uint64_t> platformTagSearchResultPics;
            for (const auto& platformID : lastPlatformTagSearchResult) {
                auto picIds = database.getMetadataPicIds(platformID);
                platformTagSearchResultPics.insert(picIds.begin(), picIds.end());
            }
            const auto& small = lastTagSearchResult.size() < platformTagSearchResultPics.size() ? lastTagSearchResult
                                                                                                : platformTagSearchResultPics;
            const auto& large = lastTagSearchResult.size() < platformTagSearchResultPics.size() ? platformTagSearchResultPics
                                                                                                : lastTagSearchResult;
            for (const auto& id : small) {
                if (large.find(id) != large.end()) {
                    platformTagSearchIntersectedResult.insert(id);
                }
            }
        }
        if (textSearchApplied) {
            std::unordered_set<uint64_t> textSearchResultPics;
            for (const auto& platformID : lastTextSearchResult) {
                auto picIds = database.getMetadataPicIds(platformID);
                textSearchResultPics.insert(picIds.begin(), picIds.end());
            }
            const auto& small =
                lastTagSearchResult.size() < textSearchResultPics.size() ? lastTagSearchResult : textSearchResultPics;
            const auto& large =
                lastTagSearchResult.size() < textSearchResultPics.size() ? textSearchResultPics : lastTagSearchResult;
            for (const auto& id : small) {
                if (large.find(id) != large.end()) {
                    textSearchIntersectedResult.insert(id);
                }
            }
        }

        // then intersect with tag search result if tag search applied, otherwise use platform tag or text search result directly
        if (platformTagSearchApplied && textSearchApplied) {
            const auto& small = platformTagSearchIntersectedResult.size() < textSearchIntersectedResult.size()
                                    ? platformTagSearchIntersectedResult
                                    : textSearchIntersectedResult;
            const auto& large = platformTagSearchIntersectedResult.size() < textSearchIntersectedResult.size()
                                    ? textSearchIntersectedResult
                                    : platformTagSearchIntersectedResult;
            for (const auto& id : small) {
                if (large.find(id) != large.end()) {
                    intersectedResult.push_back(id);
                }
            }
        } else if (platformTagSearchApplied) {
            for (const auto& id : platformTagSearchIntersectedResult) {
                intersectedResult.push_back(id);
            }
        } else if (textSearchApplied) {
            for (const auto& id : textSearchIntersectedResult) {
                intersectedResult.push_back(id);
            }
        } else {
            for (const auto& id : lastTagSearchResult) {
                intersectedResult.push_back(id);
            }
        }
        displayItems->type = DisplayItemType::Pic;
        displayItems->picItems.reserve(intersectedResult.size());
        displayItems->metadataItems.reserve(intersectedResult.size());
        size_t picIdx = 0;
        size_t metadataIdx = 0;
        for (const auto& id : intersectedResult) {
            PicInfo picInfo = database.getPicInfo(id);
            displayItems->picItems.emplace_back(PicItem{picInfo, metadataIdx, picInfo.sourceIdentifiers.size()});
            for (const auto& identifier : picInfo.sourceIdentifiers) {
                displayItems->metadataItems.emplace_back(MetadataItem{database.getMetadata(identifier), picIdx, 1});
                metadataIdx++;
            }
            picIdx++;
        }
    }

    // gather available tags from resultItems
    std::unordered_map<uint32_t, int> tagCount;
    std::unordered_map<uint32_t, int> platformTagCount;
    std::unordered_set<PlatformID> countedMetadata; // to avoid double counting metadata tags
    std::unordered_set<uint64_t> countedPics;       // to avoid double counting pic tags
    for (const auto& item : displayItems->picItems) {
        const auto& pic = item.info;
        if (countedPics.find(pic.id) != countedPics.end()) continue;
        countedPics.insert(pic.id);
        for (const auto& picTag : pic.tags) {
            tagCount[picTag.tagId]++;
        }
    }
    for (const auto& metadataItem : displayItems->metadataItems) {
        const auto& metadata = metadataItem.metadata;
        if (countedMetadata.find(metadata.getPlatformID()) != countedMetadata.end()) continue;
        countedMetadata.insert(metadata.getPlatformID());
        for (const auto& tagId : metadata.tagIds) {
            platformTagCount[tagId]++;
        }
    }

    // prepare available tags
    std::vector<TagCount> availableTags;
    std::vector<PlatformTagCount> availablePlatformTags;
    for (const auto& [tagId, count] : tagCount) {
        if (includedTags.find(tagId) != includedTags.end() || excludedTags.find(tagId) != excludedTags.end()) {
            continue; // skip tags already in filter
        }
        TagCount tagCountEntry;
        tagCountEntry.tag = database.getStringTag(tagId);
        tagCountEntry.tagId = tagId;
        tagCountEntry.count = count;
        availableTags.push_back(tagCountEntry);
    }
    for (const auto& [tagId, count] : platformTagCount) {
        if (includedPlatformTags.find(tagId) != includedPlatformTags.end() ||
            excludedPlatformTags.find(tagId) != excludedPlatformTags.end()) {
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

    emit searchComplete(displayItems, availableTags, availablePlatformTags, requestId);
}
