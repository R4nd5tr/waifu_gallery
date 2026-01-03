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
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <thread>
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
            stmt_ = nullptr;
        }
    }
    sqlite3_stmt* get() const { return stmt_; }

private:
    sqlite3_stmt* stmt_;
};

class PicDatabase {
public:
    PicDatabase(const std::string& databaseFile = DEFAULT_DATABASE_FILE, DbMode mode = DbMode::Normal);
    explicit PicDatabase(DbMode mode) : PicDatabase(DEFAULT_DATABASE_FILE, mode) {}
    ~PicDatabase();

    void reloadDatabase() { initTagMapping(); };

    // transaction and mode management
    void enableForeignKeyRestriction() const {
        if (!execute("PRAGMA foreign_keys = ON;")) {
            Warn() << "Failed to enable foreign key restriction:" << sqlite3_errmsg(db);
        }
    }
    void disableForeignKeyRestriction() const {
        if (!execute("PRAGMA foreign_keys = OFF;")) {
            Warn() << "Failed to disable foreign key restriction:" << sqlite3_errmsg(db);
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
    std::vector<std::pair<StringTag, uint32_t>> getTagCounts() const; // for gui tag selection panel display
    std::vector<std::pair<PlatformStringTag, uint32_t>> getPlatformTagCounts() const;
    StringTag getStringTag(uint32_t tagId) const {
        if (currentMode == DbMode::Query) {
            Error() << "Cannot get tag by ID in Query mode.";
        }
        if (tagId < tags.size()) {
            return tags[tagId];
        }
        return StringTag{};
    }
    PlatformStringTag getPlatformStringTag(uint32_t tagId) const {
        if (currentMode == DbMode::Query) {
            Error() << "Cannot get platform tag by ID in Query mode.";
        }
        if (tagId < platformTags.size()) {
            return platformTags[tagId];
        }
        return PlatformStringTag{};
    }

    // insert functions
    bool insertPicture(const ParsedPicture& picInfo);
    bool insertMetadata(const ParsedMetadata& metadataInfo);
    bool updateMetadata(const ParsedMetadata& metadataInfo);

    // search functions
    std::vector<uint64_t> tagSearch(const std::unordered_set<uint32_t>& includedTagIds,
                                    const std::unordered_set<uint32_t>& excludedTagIds) const;
    std::vector<PlatformID> platformTagSearch(const std::unordered_set<uint32_t>& includedTagIds,
                                              const std::unordered_set<uint32_t>& excludedTagIds) const;
    std::unordered_set<PlatformID>
    textSearch(const std::string& searchText, PlatformType platformType, SearchField searchField) const;

    // import functions
    void importFilesFromDirectory(const std::filesystem::path& directory,
                                  ParserType parserType = ParserType::None,
                                  ImportProgressCallback progressCallback = nullptr);
    void processAndImportSingleFile(const std::filesystem::path& path, ParserType parserType = ParserType::None);
    // call this after scanDirectory, sync x_restrict and ai_type from pixiv to pictures, count tags
    void syncTables(std::unordered_set<PlatformID> newMetadataIds = {});
    bool isFileImported(const std::filesystem::path& filePath) const;
    void addImportedFile(const std::filesystem::path& filePath);

    // tagger functions
    void getModelName() const;
    void importTagSet(const std::string& modelName, const std::vector<std::pair<std::string, bool>>& tags); // (tag, isCharacter)
    std::vector<std::pair<uint64_t, std::filesystem::path>> getUntaggedPics();                              // (picID, filePath)
    void updatePicTags(uint64_t picID, const std::vector<int>& tagIds, const std::vector<float>& probabilities);
    bool insertPictureTags(uint64_t id, int tagId, float probability);

private:
    DbMode currentMode = DbMode::None;
    sqlite3* db = nullptr;

    // in-memory tag mapping
    std::unordered_map<std::string, uint32_t> tagToId;
    std::unordered_map<PlatformStringTag, uint32_t> platformTagToId;
    std::vector<StringTag> tags;                 // index is tag ID
    std::vector<PlatformStringTag> platformTags; // index is platform tag ID

    // feature hash cache for similarity search
    std::vector<std::pair<uint64_t, std::array<uint8_t, 64>>> picFeatureHashes; // (picID, featureHash)

    // import related
    std::unordered_set<PlatformID> newMetadataIds;                                  // for syncTables use
    std::unordered_map<std::string, std::unordered_set<std::string>> importedFiles; // directory -> set of imported file names

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

class MultiThreadedImporter {
public:
    MultiThreadedImporter(const std::filesystem::path& directory,
                          ImportProgressCallback progressCallback = nullptr,
                          ParserType prserType = ParserType::None,
                          std::string dbFile = DEFAULT_DATABASE_FILE,
                          size_t threadCount = std::thread::hardware_concurrency());
    ~MultiThreadedImporter();

    bool finish();
    void forceStop();
    std::pair<std::filesystem::path, ParserType> getImportingDir() const { return {importDirectory, parserType}; }

private:
    bool finished = false;

    ImportProgressCallback progressCallback; // this will be called in insert thread, make sure it's thread safe
    ParserType parserType;
    std::string dbFile;
    std::filesystem::path importDirectory;

    std::vector<std::thread> workers;
    std::vector<std::filesystem::path> files;
    std::atomic<bool> readyFlag = false;
    std::condition_variable cvReady;
    std::atomic<size_t> nextFileIndex = 0;
    size_t importedCount = 0;
    std::atomic<size_t> supportedFileCount = 0; // to prevent importer from never stopping
                                                // when some unsupported files are in the directory
    // single insert thread
    std::thread insertThread;
    std::queue<ParsedPicture> parsedPictureQueue; // one ParsedPicture represents one image file
    std::mutex parsedPicQueueMutex;
    std::queue<std::vector<ParsedMetadata>> metadataVecQueue; // one vector represents one metadata file
    std::mutex parsedMetadataQueueMutex;

    std::atomic<bool> stopFlag = false;
    std::condition_variable cv;

    void workerThreadFunc();
    void insertThreadFunc(); // TODO:use only one transaction and commit at the end for cancel operation?
};
