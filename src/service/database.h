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
#include "parser.h"
#include "utils/logger.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <sqlite3.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using ImportProgressCallback = std::function<void(size_t processed, size_t total)>;

enum class SearchField { None, PlatformID, AuthorID, AuthorName, AuthorNick, Title };

const std::string DEFAULT_DATABASE_FILE = "database.db";

enum class DbMode { None, Normal, Import, Query };

class SQLiteStatement { // RAII wrapper for sqlite3_stmt
public:
    SQLiteStatement() : stmt_(nullptr) {}
    SQLiteStatement(sqlite3* db, const std::string& sql) : stmt_(nullptr) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
            Error() << "Failed to prepare statement:" << sql << "Error:" << sqlite3_errmsg(db);
            stmt_ = nullptr;
        }
    }
    ~SQLiteStatement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }
    sqlite3_stmt* get() const { return stmt_; }

    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;
    SQLiteStatement(SQLiteStatement&& other) noexcept : stmt_(other.stmt_) { other.stmt_ = nullptr; }
    SQLiteStatement& operator=(SQLiteStatement&& other) noexcept {
        if (this != &other) {
            if (stmt_) {
                sqlite3_finalize(stmt_);
            }
            stmt_ = other.stmt_;
            other.stmt_ = nullptr;
        }
        return *this;
    }

private:
    sqlite3_stmt* stmt_;
};

class DbCache { // singleton class for caching database mappings
public:
    static DbCache& getInstance() {
        static DbCache instance;
        return instance;
    }
    bool tagMappingLoaded() const {
        return !tagToId.empty() && !platformTagToId.empty() && !tags.empty() && !platformTags.empty();
    }
    bool featureHashCacheLoaded() const { return !picFeatureHashes.empty(); }
    bool importedFileLoaded() const { return !importedFiles.empty(); }

    void loadTagMapping(std::unordered_map<std::string, uint32_t>&& tagToIdMap,
                        std::unordered_map<PlatformTagStr, uint32_t>&& platformTagToIdMap,
                        std::vector<TagStr>&& tagList,
                        std::vector<PlatformTagStr>&& platformTagList) {
        std::lock_guard<std::mutex> lock(writeMutex);
        tagToId = tagToIdMap;
        platformTagToId = platformTagToIdMap;
        tags = tagList;
        platformTags = platformTagList;
    }
    void loadPicFeatureHashes(std::vector<std::pair<uint64_t, std::array<uint8_t, 64>>>&& featureHashes) {
        std::lock_guard<std::mutex> lock(writeMutex);
        picFeatureHashes = featureHashes;
    }
    void loadImportedFiles(std::unordered_map<std::string, std::unordered_set<std::string>>&& files) {
        std::lock_guard<std::mutex> lock(writeMutex);
        importedFiles = files;
    }

    TagStr getStringTag(uint32_t tagId) const {
        if (tagId < tags.size()) {
            return tags[tagId];
        }
        return TagStr{};
    }
    PlatformTagStr getPlatformStringTag(uint32_t tagId) const {
        if (tagId < platformTags.size()) {
            return platformTags[tagId];
        }
        return PlatformTagStr{};
    }
    bool isFileImported(const std::filesystem::path& filePath) const {
        std::string dir = filePath.parent_path().string();
        std::string filename = filePath.filename().string();
        if (importedFiles.find(dir) != importedFiles.end()) {
            if (importedFiles.at(dir).find(filename) != importedFiles.at(dir).end()) {
                return true;
            }
        }
        return false;
    }
    void addImportedFile(const std::filesystem::path& filePath) {
        if (isFileImported(filePath)) return;
        std::lock_guard<std::mutex> lock(writeMutex);

        std::string dir = filePath.parent_path().string();
        std::string filename = filePath.filename().string();
        if (importedFiles.find(dir) == importedFiles.end()) {
            importedFiles[dir] = std::unordered_set<std::string>();
        }
        importedFiles[dir].insert(filename);
    }

    bool platformTagExists(const PlatformTagStr& tag) const { return platformTagToId.find(tag) != platformTagToId.end(); }
    uint32_t addPlatformTag(const PlatformTagStr& tag) {
        if (platformTagToId.find(tag) != platformTagToId.end()) {
            return platformTagToId.at(tag);
        }
        std::lock_guard<std::mutex> lock(writeMutex);
        platformTags.emplace_back(tag);
        uint32_t newId = static_cast<uint32_t>(platformTags.size());
        platformTagToId[tag] = newId;
        return newId;
    }
    uint32_t getPlatformTagId(const PlatformTagStr& tag) const { return platformTagToId.at(tag); }

    void clearTagMapping() {
        std::lock_guard<std::mutex> lock(writeMutex);
        tagToId.clear();
        platformTagToId.clear();
        tags.clear();
        platformTags.clear();
    }

private:
    DbCache() = default;
    ~DbCache() = default;
    DbCache(const DbCache&) = delete;
    DbCache& operator=(const DbCache&) = delete;
    DbCache(DbCache&&) = delete;
    DbCache& operator=(DbCache&&) = delete;

    std::mutex writeMutex;

    // in-memory tag mapping
    std::unordered_map<std::string, uint32_t> tagToId;
    std::unordered_map<PlatformTagStr, uint32_t> platformTagToId;
    std::vector<TagStr> tags;                 // index is tag ID
    std::vector<PlatformTagStr> platformTags; // index is platform tag ID

    // feature hash cache for similarity search
    std::vector<std::pair<uint64_t, std::array<uint8_t, 64>>> picFeatureHashes; // (picID, featureHash)

    // imported files cache
    std::unordered_map<std::string, std::unordered_set<std::string>> importedFiles; // directory -> set of imported file names
};

class PicDatabase { // sqlite database wrapper
public:
    PicDatabase(const std::string& databaseFile = DEFAULT_DATABASE_FILE, DbMode mode = DbMode::Normal);
    explicit PicDatabase(DbMode mode) : PicDatabase(DEFAULT_DATABASE_FILE, mode) {}
    ~PicDatabase();

    // transaction and mode management
    void enableForeignKeyRestriction() const {
        if (!execute("PRAGMA foreign_keys = ON;")) {
            Error() << "Failed to enable foreign key restriction:" << sqlite3_errmsg(db);
        }
    }
    void disableForeignKeyRestriction() const {
        if (!execute("PRAGMA foreign_keys = OFF;")) {
            Error() << "Failed to disable foreign key restriction:" << sqlite3_errmsg(db);
        }
    }
    bool beginTransaction() { return execute("BEGIN TRANSACTION;"); }
    bool commitTransaction() { return execute("COMMIT;"); }
    bool rollbackTransaction() { return execute("ROLLBACK;"); }
    void setMode(DbMode mode) {
        if (currentMode == mode) return;
        execute("PRAGMA journal_mode = WAL");
        switch (mode) {
        case DbMode::Normal: // use for ui thread
            execute("PRAGMA cache_size = -32000");
            execute("PRAGMA synchronous = NORMAL");
            execute("PRAGMA foreign_keys = ON");
            break;

        case DbMode::Import: // use for batch import
            execute("PRAGMA cache_size = -128000");
            execute("PRAGMA temp_store = memory");
            execute("PRAGMA foreign_keys = OFF");
            execute("PRAGMA synchronous = NORMAL");
            break;

        case DbMode::Query: // use for read-only query
            execute("PRAGMA cache_size = -64000");
            execute("PRAGMA mmap_size = 268435456");
            execute("PRAGMA foreign_keys = ON");
            execute("PRAGMA query_only = ON");
            break;
        }
        currentMode = mode;
    }

    // getters
    PicInfo getPicInfo(uint64_t id, bool loadAssociatedMetadata = false) const;
    Metadata getMetadata(PlatformType platform, int64_t platformID, bool loadAssociatedPics = false) const;
    Metadata getMetadata(ImageSource identifier) const { return getMetadata(identifier.platform, identifier.platformID); }
    Metadata getMetadata(const PlatformID& platformID, bool loadAssociatedPics = false) const {
        return getMetadata(platformID.platform, platformID.platformID, loadAssociatedPics);
    }
    std::vector<PicInfo> getNoMetadataPics() const;
    std::vector<TagCount> getTagCounts() const; // for gui tag selection panel display
    std::vector<PlatformTagCount> getPlatformTagCounts() const;
    TagStr getStringTag(uint32_t tagId) const { return cache.getStringTag(tagId); }
    PlatformTagStr getPlatformStringTag(uint32_t tagId) const { return cache.getPlatformStringTag(tagId); }

    // insert functions
    bool insertPicture(const ParsedPicture& picInfo);
    bool insertMetadata(const ParsedMetadata& metadataInfo);
    bool updateMetadata(const ParsedMetadata& metadataInfo);

    // search functions
    std::vector<uint64_t> tagSearch(const std::unordered_set<uint32_t>& includedTagIds,
                                    const std::unordered_set<uint32_t>& excludedTagIds) const;
    std::vector<PlatformID> platformTagSearch(const std::unordered_set<uint32_t>& includedTagIds,
                                              const std::unordered_set<uint32_t>& excludedTagIds) const;
    std::vector<PlatformID> textSearch(const std::string& searchText, PlatformType platformType, SearchField searchField) const;

    // import functions
    void importFilesFromDirectory(
        const std::filesystem::path& directory,   // single-threaded import function, only for debug use
        ParserType parserType = ParserType::None, // use as a base line comparison to multi-threaded Importer class
        ImportProgressCallback progressCallback = nullptr);
    void processAndImportSingleFile(const std::filesystem::path& path, ParserType parserType = ParserType::None);
    void syncMetadataAndPicTables(std::unordered_set<PlatformID> newMetadataIds = {}); // post-import operations
    bool isFileImported(const std::filesystem::path& filePath) const { return cache.isFileImported(filePath); }
    void addImportedFile(const std::filesystem::path& filePath);
    void updatePlatformTagCounts(); // update platform tag counts after bulk import
    void updateTagCounts();         // update tag counts after bulk import

    // tagger functions
    std::string getModelName() const;
    void importTagSet(const std::string& modelName, const std::vector<std::pair<std::string, bool>>& tags); // (tag, isCharacter)
    std::vector<std::pair<uint64_t, std::vector<std::filesystem::path>>> getUntaggedPics();                 // (picID, filePath)
    void updatePicTags(uint64_t picID,
                       const std::vector<PicTag>& picTags,
                       RestrictType restrictType,
                       std::vector<uint8_t> featureHash);

private:
    sqlite3* db = nullptr;
    DbMode currentMode = DbMode::None;
    DbCache& cache = DbCache::getInstance();

    std::unordered_set<PlatformID> newMetadataIds; // for syncMetadataAndPicTables use

    void initDatabase(const std::string& databaseFile);
    bool createTables();
    void initTagMapping();
    void initImportedFiles();

    bool execute(const std::string& sql) const {
        char* errorMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMsg);
        if (rc != SQLITE_OK) {
            Error() << "Error executing SQL: " << errorMsg;
            sqlite3_free(errorMsg);
            return false;
        }
        return true;
    }
    SQLiteStatement prepare(const std::string& sql) const { return SQLiteStatement(db, sql); }
};
