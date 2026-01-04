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

#include "database.h"
#include "model.h"
#include "parser.h"
#include "utils/utils.h"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>

// utility functions

int64_t uint64_to_int64(uint64_t u) {
    int64_t i;
    std::memcpy(&i, &u, sizeof(u));
    return i;
}
uint64_t int64_to_uint64(int64_t i) {
    uint64_t u;
    std::memcpy(&u, &i, sizeof(i));
    return u;
}

// PicDatabase class implementation

PicDatabase::PicDatabase(const std::string& databaseFile, DbMode mode) {
    initDatabase(databaseFile);
    setMode(mode);
    if (mode != DbMode::Query) initTagMapping();     // query mode does not need tag mapping
    if (mode == DbMode::Import) initImportedFiles(); // only needed in import mode, to avoid parsing duplicate files
}
PicDatabase::~PicDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

// init functions

void PicDatabase::initDatabase(const std::string& databaseFile) {
    if (sqlite3_open(databaseFile.c_str(), &db) != SQLITE_OK) {
        Error() << "Error opening database: " << sqlite3_errmsg(db);
        return;
    }
    if (!createTables()) {
        Error() << "Failed to create tables";
        return;
    }
    Info() << "Database initialized";
}
bool PicDatabase::createTables() {
    const std::string metadataTable = R"(
        CREATE TABLE IF NOT EXISTS metadata (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )";
    const std::string picturesTable = R"(
        CREATE TABLE IF NOT EXISTS pictures (
            id INTEGER PRIMARY KEY NOT NULL,
            width INTEGER,
            height INTEGER,
            size INTEGER,
            file_type INTEGER,

            edit_time TEXT DEFAULT NULL,
            download_time TEXT DEFAULT NULL,

            feature_hash BLOB DEFAULT NULL,

            restrict_type INTEGER DEFAULT 0,
            ai_type INTEGER DEFAULT 0
        )
    )";
    const std::string pictureTagsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tags (
            id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            probability REAL DEFAULT 0.0,

            PRIMARY KEY (id, tag_id),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
        )
    )";
    const std::string pictureFilesTable = R"(
        CREATE TABLE IF NOT EXISTS picture_file_paths (
            id INTEGER NOT NULL,
            file_path TEXT NOT NULL UNIQUE,

            PRIMARY KEY (id, file_path),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const std::string pictureSourceTable = R"(
        CREATE TABLE IF NOT EXISTS picture_source (
            id INTEGER NOT NULL,
            platform INTEGER NOT NULL,
            platform_id INTEGER NOT NULL,
            image_index INTEGER NOT NULL,

            PRIMARY KEY (platform, platform_id, image_index),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const std::string tagsTable = R"(
        CREATE TABLE IF NOT EXISTS tags (
            tag_id INTEGER PRIMARY KEY,
            tag TEXT NOT NULL UNIQUE,
            is_character BOOLEAN,
            count INTEGER DEFAULT 0
        )
    )";
    const std::string picMetadataTable = R"(
        CREATE TABLE IF NOT EXISTS picture_metadata (
            platform INTEGER NOT NULL,
            platform_id INTEGER NOT NULL,
            date DATE NOT NULL,

            author_id INTEGER NOT NULL,
            author_name TEXT NOT NULL,
            author_nick TEXT,
            author_description TEXT,

            title TEXT,
            description TEXT,

            view_count INTEGER DEFAULT 0,
            like_count INTEGER DEFAULT 0,
            bookmark_count INTEGER DEFAULT 0,
            reply_count INTEGER DEFAULT 0,
            forward_count INTEGER DEFAULT 0,
            quote_count INTEGER DEFAULT 0,

            restrict_type INTEGER DEFAULT 0,
            ai_type INTEGER DEFAULT 0,

            PRIMARY KEY (platform, platform_id)
        )
    )";
    const std::string picMetadataTagsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_metadata_tags (
            platform INTEGER NOT NULL,
            platform_id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,

            PRIMARY KEY (platform, platform_id, tag_id),

            FOREIGN KEY (platform, platform_id) REFERENCES picture_metadata(platform, platform_id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES platform_tags(tag_id) ON DELETE CASCADE
        )
    )";
    const std::string platformTagsTable = R"(
        CREATE TABLE IF NOT EXISTS platform_tags (
            tag_id INTEGER NOT NULL PRIMARY KEY,
            platform INTEGER NOT NULL,
            tag TEXT NOT NULL,
            translated_tag TEXT,
            count INTEGER DEFAULT 0,

            UNIQUE (platform, tag)
        )
    )";
    const std::string importedDirectoriesTable = R"(
        CREATE TABLE IF NOT EXISTS imported_directories (
            dir_id INTEGER PRIMARY KEY AUTOINCREMENT,
            dir_path TEXT NOT NULL UNIQUE
        )
    )";
    const std::string importedFilesTable = R"(
        CREATE TABLE IF NOT EXISTS imported_files (
            dir_id INTEGER NOT NULL,
            filename TEXT NOT NULL,
            
            PRIMARY KEY (dir_id, filename),

            FOREIGN KEY (dir_id) REFERENCES imported_directories(dir_id) ON DELETE CASCADE
        )
    )";
    const std::vector<std::string> tables = {metadataTable,
                                             // pictures
                                             picturesTable,
                                             pictureTagsTable,
                                             pictureFilesTable,
                                             pictureSourceTable,
                                             tagsTable,
                                             // picture metadata
                                             picMetadataTable,
                                             picMetadataTagsTable,
                                             platformTagsTable,
                                             // imported files tracking
                                             importedDirectoriesTable,
                                             importedFilesTable};
    const std::vector<std::string> indexes = {
        // foreign key indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_id ON picture_tags(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_tag_id ON picture_tags(tag_id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_file_paths_id ON picture_file_paths(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_source_id ON picture_source(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_metadata_tags_platform_id ON picture_metadata_tags(platform, platform_id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_metadata_tags_tag_id ON picture_metadata_tags(tag_id)",
        // tags indexes
        "CREATE INDEX IF NOT EXISTS idx_tags_tag ON tags(tag)",
        "CREATE INDEX IF NOT EXISTS idx_platform_tags_platform_tag ON platform_tags(platform, tag)",
        // author indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_metadata_author_id ON picture_metadata(author_id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_metadata_author_name ON picture_metadata(author_name)",
        // metadata indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_metadata_title ON picture_metadata(title)",
        // tag search indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_id ON picture_tags(tag_id, id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_metadata_tags_tag_id ON picture_metadata_tags(tag_id, platform, platform_id)",
        // imported files tracking indexes
        "CREATE INDEX IF NOT EXISTS idx_imported_directories_dir_path ON imported_directories(dir_path)"};
    beginTransaction();
    for (const auto& tableSql : tables) {
        if (!execute(tableSql)) {
            Error() << "Failed to create table:" << sqlite3_errmsg(db);
            rollbackTransaction();
            return false;
        }
    }
    for (const auto& indexSql : indexes) {
        if (!execute(indexSql)) {
            Error() << "Failed to create index:" << sqlite3_errmsg(db);
            rollbackTransaction();
            return false;
        }
    }
    commitTransaction();
    return true;
}
void PicDatabase::initTagMapping() {
    tagToId.clear();
    platformTagToId.clear();
    tags.clear();
    platformTags.clear();

    SQLiteStatement stmt;

    stmt = prepare("SELECT tag, is_character FROM tags ORDER BY tag_id ASC");
    if (!stmt.get()) {
        Error() << "Failed to prepare statement for fetching tags.";
        return;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        bool isCharacterTag = sqlite3_column_int(stmt.get(), 1) != 0;
        tags.emplace_back(StringTag{tag, isCharacterTag});
        // only need tagToId mapping in import mode, for inserting new tags
        if (currentMode == DbMode::Import) tagToId[tag] = static_cast<uint32_t>(tags.size() - 1);
    }

    stmt = prepare("SELECT tag, platform FROM platform_tags ORDER BY tag_id ASC");
    if (!stmt.get()) {
        Error() << "Failed to prepare statement for fetching twitter hashtags.";
        return;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        PlatformType platform = static_cast<PlatformType>(sqlite3_column_int(stmt.get(), 1));
        platformTags.emplace_back(PlatformStringTag{platform, tag});
        if (currentMode == DbMode::Import)
            platformTagToId[PlatformStringTag{platform, tag}] = static_cast<uint32_t>(platformTags.size() - 1); // same as above
    }

    Info() << "Tag mappings loaded. Tags:" << tags.size() << "Platform Tags:" << platformTags.size();
}
void PicDatabase::initImportedFiles() {
    importedFiles.clear();
    SQLiteStatement stmt;
    stmt = prepare(R"(
        SELECT id.dir_path, f.filename
        FROM imported_files f
        JOIN imported_directories id ON f.dir_id = id.dir_id
    )");
    if (!stmt.get()) {
        Error() << "Failed to prepare statement for fetching imported files.";
        return;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* dirPathCStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        const char* fileNameCStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        if (dirPathCStr && fileNameCStr) {
            std::string dirPath(dirPathCStr);
            std::string fileName(fileNameCStr);
            importedFiles[dirPath].insert(fileName);
        }
    }
    Info() << "Imported files tracking initialized. Directories:" << importedFiles.size();
}

// insert functions

bool PicDatabase::insertPicture(const ParsedPicture& picInfo) {
    SQLiteStatement stmt;
    // insert into pictures table
    stmt = prepare(R"(
        INSERT OR IGNORE INTO pictures(
            id, width, height, size, file_type, edit_time, download_time, restrict_type
        ) VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(picInfo.id));
    sqlite3_bind_int(stmt.get(), 2, picInfo.width);
    sqlite3_bind_int(stmt.get(), 3, picInfo.height);
    sqlite3_bind_int64(stmt.get(), 4, picInfo.size);
    sqlite3_bind_int(stmt.get(), 5, static_cast<int>(picInfo.fileType));
    sqlite3_bind_text(stmt.get(), 6, picInfo.editTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 7, picInfo.downloadTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 8, static_cast<int>(picInfo.restrictType));
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert picture: " << sqlite3_errmsg(db);
        return false;
    }
    // insert into picture_file_paths table
    stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_file_paths(
                id, file_path
            ) VALUES (?, ?)
        )");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(picInfo.id));
    sqlite3_bind_text(stmt.get(), 2, picInfo.filePath.generic_u8string().c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert picture_file_path: " << sqlite3_errmsg(db);
        return false;
    }
    // insert into picture_source table
    stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_source(
                id, platform, platform_id, image_index
            ) VALUES (
                ?, ?, ?, ?
            )
        )");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(picInfo.id));
    sqlite3_bind_int(stmt.get(), 2, static_cast<int>(picInfo.identifier.platform));
    sqlite3_bind_int64(stmt.get(), 3, picInfo.identifier.platformID);
    sqlite3_bind_int(stmt.get(), 4, picInfo.identifier.imageIndex);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert picture_source: " << sqlite3_errmsg(db);
        return false;
    }

    return true;
}
bool PicDatabase::insertPictureTags(uint64_t id, int tagId, float probability) {
    SQLiteStatement stmt;
    stmt = prepare(R"(
        INSERT OR IGNORE INTO picture_tags(
            id, tag_id, probability
        ) VALUES (?, ?, ?)
    )");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(id));
    sqlite3_bind_int(stmt.get(), 2, tagId);
    sqlite3_bind_double(stmt.get(), 3, probability);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert picture_tag: " << sqlite3_errmsg(db);
        return false;
    }
    return true;
}
bool PicDatabase::insertMetadata(const ParsedMetadata& metadataInfo) {
    if (metadataInfo.updateIfExists) {
        Warn() << "insertMetadata called with updateIfExists=true, redirecting to updateMetadata.";
        return updateMetadata(metadataInfo);
    }
    SQLiteStatement stmt;
    // insert into picture_metadata table
    stmt = prepare(R"(
        INSERT OR IGNORE INTO picture_metadata(
            platform, platform_id, date, author_id, author_name, author_nick, 
            author_description, title, description, view_count, like_count, bookmark_count,
            reply_count, forward_count, quote_count, restrict_type, ai_type
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    sqlite3_bind_int(stmt.get(), 1, static_cast<int>(metadataInfo.platformType));
    sqlite3_bind_int64(stmt.get(), 2, metadataInfo.id);
    sqlite3_bind_text(stmt.get(), 3, metadataInfo.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 4, metadataInfo.authorID);
    sqlite3_bind_text(stmt.get(), 5, metadataInfo.authorName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, metadataInfo.authorNick.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 7, metadataInfo.authorDescription.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 8, metadataInfo.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, metadataInfo.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, metadataInfo.viewCount);
    sqlite3_bind_int(stmt.get(), 11, metadataInfo.likeCount);
    sqlite3_bind_int(stmt.get(), 12, metadataInfo.bookmarkCount);
    sqlite3_bind_int(stmt.get(), 13, metadataInfo.replyCount);
    sqlite3_bind_int(stmt.get(), 14, metadataInfo.forwardCount);
    sqlite3_bind_int(stmt.get(), 15, metadataInfo.quoteCount);
    sqlite3_bind_int(stmt.get(), 16, static_cast<int>(metadataInfo.restrictType));
    sqlite3_bind_int(stmt.get(), 17, static_cast<int>(metadataInfo.aiType));
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert picture_metadata: " << sqlite3_errmsg(db);
        return false;
    }
    // insert into picture_metadata_tags table
    for (const auto& tag : metadataInfo.tags) {
        PlatformStringTag stringTag{metadataInfo.platformType, tag};
        if (platformTagToId.find(stringTag) == platformTagToId.end()) {
            // insert new platform tag
            platformTags.emplace_back(stringTag);
            int newId = static_cast<int>(platformTags.size() - 1);
            platformTagToId[stringTag] = newId;
            stmt = prepare(R"(
                INSERT OR IGNORE INTO platform_tags(tag_id, platform, tag) VALUES (?, ?, ?)
            )");
            sqlite3_bind_int(stmt.get(), 1, newId);
            sqlite3_bind_int(stmt.get(), 2, static_cast<int>(metadataInfo.platformType));
            sqlite3_bind_text(stmt.get(), 3, tag.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
                Error() << "Failed to insert new platform_tag: " << sqlite3_errmsg(db);
                return false;
            }
        }
        stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_metadata_tags(
                platform, platform_id, tag_id
            ) VALUES (?, ?, ?)
        )");
        sqlite3_bind_int(stmt.get(), 1, static_cast<int>(metadataInfo.platformType));
        sqlite3_bind_int64(stmt.get(), 2, metadataInfo.id);
        sqlite3_bind_int(stmt.get(), 3, platformTagToId.at(stringTag));
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Error() << "Failed to insert picture_metadata_tag: " << sqlite3_errmsg(db);
            return false;
        }
    }
    if (metadataInfo.tags.size() == metadataInfo.tagsTransl.size()) {
        for (size_t i = 0; i < metadataInfo.tags.size(); ++i) {
            PlatformStringTag stringTag{metadataInfo.platformType, metadataInfo.tags[i]};
            int tagId = platformTagToId.at(stringTag);
            stmt = prepare(R"(
                UPDATE platform_tags SET translated_tag = ? WHERE tag_id = ?
            )");
            sqlite3_bind_text(stmt.get(), 1, metadataInfo.tagsTransl[i].c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt.get(), 2, tagId);
            if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
                Error() << "Failed to update platform_tag translated_tag: " << sqlite3_errmsg(db);
                return false;
            }
        }
    }
    newMetadataIds.insert(PlatformID{metadataInfo.platformType, metadataInfo.id});
    return true;
}
bool PicDatabase::updateMetadata(const ParsedMetadata& metadataInfo) {
    SQLiteStatement stmt;
    // update picture_metadata table
    stmt = prepare(R"(
        INSERT INTO picture_metadata(
            platform, platform_id, date, author_id, author_name, author_nick, 
            author_description, title, description, view_count, like_count, bookmark_count,
            reply_count, forward_count, quote_count, restrict_type, ai_type
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(platform, platform_id) DO UPDATE SET
            date=excluded.date,
            author_id=excluded.author_id,
            author_name=excluded.author_name,
            author_nick=excluded.author_nick,
            author_description=excluded.author_description,
            title=excluded.title,
            description=excluded.description,
            view_count=excluded.view_count,
            like_count=excluded.like_count,
            bookmark_count=excluded.bookmark_count,
            reply_count=excluded.reply_count,
            forward_count=excluded.forward_count,
            quote_count=excluded.quote_count,
            -- only update restrict_type when excluded.restrict_type is not NULL and greater than existing
            restrict_type = CASE
                WHEN excluded.restrict_type IS NOT NULL
                     AND (picture_metadata.restrict_type IS NULL OR excluded.restrict_type > picture_metadata.restrict_type)
                THEN excluded.restrict_type
                ELSE picture_metadata.restrict_type
            END,
            -- only update ai_type when excluded.ai_type is not NULL and greater than existing
            ai_type = CASE
                WHEN excluded.ai_type IS NOT NULL
                     AND (picture_metadata.ai_type IS NULL OR excluded.ai_type > picture_metadata.ai_type)
                THEN excluded.ai_type
                ELSE picture_metadata.ai_type
            END
    )");
    sqlite3_bind_int(stmt.get(), 1, static_cast<int>(metadataInfo.platformType));
    sqlite3_bind_int64(stmt.get(), 2, metadataInfo.id);
    sqlite3_bind_text(stmt.get(), 3, metadataInfo.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 4, metadataInfo.authorID);
    sqlite3_bind_text(stmt.get(), 5, metadataInfo.authorName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 6, metadataInfo.authorNick.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 7, metadataInfo.authorDescription.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 8, metadataInfo.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 9, metadataInfo.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 10, metadataInfo.viewCount);
    sqlite3_bind_int(stmt.get(), 11, metadataInfo.likeCount);
    sqlite3_bind_int(stmt.get(), 12, metadataInfo.bookmarkCount);
    sqlite3_bind_int(stmt.get(), 13, metadataInfo.replyCount);
    sqlite3_bind_int(stmt.get(), 14, metadataInfo.forwardCount);
    sqlite3_bind_int(stmt.get(), 15, metadataInfo.quoteCount);
    sqlite3_bind_int(stmt.get(), 16, static_cast<int>(metadataInfo.restrictType));
    sqlite3_bind_int(stmt.get(), 17, static_cast<int>(metadataInfo.aiType));
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to update picture_metadata: " << sqlite3_errmsg(db);
        return false;
    }
    // update picture_metadata_tags table
    for (const auto& tag : metadataInfo.tags) {
        PlatformStringTag stringTag{metadataInfo.platformType, tag};
        if (platformTagToId.find(stringTag) == platformTagToId.end()) {
            // insert new platform tag
            platformTags.emplace_back(stringTag);
            int newId = static_cast<int>(platformTags.size() - 1);
            platformTagToId[stringTag] = newId;
            stmt = prepare(R"(
                INSERT OR IGNORE INTO platform_tags(tag_id, platform, tag) VALUES (?, ?, ?)
            )");
            sqlite3_bind_int(stmt.get(), 1, newId);
            sqlite3_bind_int(stmt.get(), 2, static_cast<int>(metadataInfo.platformType));
            sqlite3_bind_text(stmt.get(), 3, tag.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
                Error() << "Failed to insert new platform_tag: " << sqlite3_errmsg(db);
                return false;
            }
        }
        stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_metadata_tags(
                platform, platform_id, tag_id
            ) VALUES (?, ?, ?)
        )");
        sqlite3_bind_int(stmt.get(), 1, static_cast<int>(metadataInfo.platformType));
        sqlite3_bind_int64(stmt.get(), 2, metadataInfo.id);
        sqlite3_bind_int(stmt.get(), 3, platformTagToId.at(stringTag));
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Error() << "Failed to insert picture_metadata_tag: " << sqlite3_errmsg(db);
            return false;
        }
    }
    if (metadataInfo.tags.size() == metadataInfo.tagsTransl.size()) {
        for (size_t i = 0; i < metadataInfo.tags.size(); ++i) {
            PlatformStringTag stringTag{metadataInfo.platformType, metadataInfo.tags[i]};
            int tagId = platformTagToId.at(stringTag);
            stmt = prepare(R"(
                UPDATE platform_tags SET translated_tag = ? WHERE tag_id = ?
            )");
            sqlite3_bind_text(stmt.get(), 1, metadataInfo.tagsTransl[i].c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt.get(), 2, tagId);
            if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
                Error() << "Failed to update platform_tag translated_tag: " << sqlite3_errmsg(db);
                return false;
            }
        }
    }
    return true;
}

// query functions

PicInfo PicDatabase::getPicInfo(uint64_t id, bool loadAssociatedMetadata) const {
    PicInfo info{};
    SQLiteStatement stmt;

    // query main picture info
    stmt = prepare("SELECT width, height, size, file_type, edit_time, download_time, feature_hash, restrict_type, ai_type FROM "
                   "pictures WHERE id = ?");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(id));
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        info.id = id;
        info.width = sqlite3_column_int(stmt.get(), 0);
        info.height = sqlite3_column_int(stmt.get(), 1);
        info.size = sqlite3_column_int(stmt.get(), 2);
        info.fileType = static_cast<ImageFormat>(sqlite3_column_int(stmt.get(), 3));
        info.editTime = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4)));
        info.downloadTime = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5)));
        const void* featureHashBlob = sqlite3_column_blob(stmt.get(), 6);
        int featureHashSize = sqlite3_column_bytes(stmt.get(), 6);
        if (featureHashBlob && featureHashSize == sizeof(info.featureHash)) {
            std::memcpy(info.featureHash.data(), featureHashBlob, sizeof(info.featureHash));
        } else {
            info.featureHash.fill(0);
        }
        info.restrictType = static_cast<RestrictType>(sqlite3_column_int(stmt.get(), 7));
        info.aiType = static_cast<AIType>(sqlite3_column_int(stmt.get(), 8));
    } else {
        return info; // id 不存在，返回空对象
    }

    // query file paths
    stmt = prepare("SELECT file_path FROM picture_file_paths WHERE id = ?");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(id));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        info.filePaths.emplace_back(std::filesystem::path(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0))));
    }

    // query tags
    stmt = prepare("SELECT tag_id, probability FROM picture_tags WHERE id = ?");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(id));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        PicTag picTag;
        picTag.tagId = sqlite3_column_int(stmt.get(), 0);
        picTag.probability = static_cast<float>(sqlite3_column_double(stmt.get(), 1));
        info.tags.push_back(picTag);
    }

    // query source identifiers
    stmt = prepare("SELECT platform, platform_id, image_index FROM picture_source WHERE id = ?");
    sqlite3_bind_int64(stmt.get(), 1, uint64_to_int64(id));
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        ImageSource identifier;
        identifier.platform = static_cast<PlatformType>(sqlite3_column_int(stmt.get(), 0));
        identifier.platformID = sqlite3_column_int64(stmt.get(), 1);
        identifier.imageIndex = sqlite3_column_int(stmt.get(), 2);
        info.sourceIdentifiers.push_back(identifier);
    }

    // query associated metadata
    if (!loadAssociatedMetadata) return info;
    for (const auto& identifier : info.sourceIdentifiers) {
        info.associatedMetadata.emplace_back(getMetadata(identifier));
    }

    return info;
}
// 查询没有任何元数据关联的图片和有元数据关联但对应元数据缺失的图片
std::vector<PicInfo> PicDatabase::getNoMetadataPics() const {
    std::vector<PicInfo> pics;
    SQLiteStatement stmt;

    stmt = prepare(R"(
        SELECT p.id
        FROM pictures p
        WHERE NOT EXISTS (
            SELECT 1
            FROM picture_source ps
            INNER JOIN picture_metadata pm 
                ON ps.platform = pm.platform 
                AND ps.platform_id = pm.platform_id
            WHERE ps.id = p.id
            )
    )");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        uint64_t picID = static_cast<uint64_t>(sqlite3_column_int64(stmt.get(), 0));
        PicInfo picInfo = getPicInfo(picID);
        pics.push_back(picInfo);
    }
    return pics;
}
Metadata PicDatabase::getMetadata(PlatformType platform, int64_t platformID, bool loadAssociatedPics) const {
    Metadata info{};
    SQLiteStatement stmt;

    // query main metadata
    stmt = prepare(R"(
        SELECT date, author_id, author_name, author_nick, author_description,
               title, description, view_count, like_count, bookmark_count,
               reply_count, forward_count, quote_count, restrict_type, ai_type
        FROM picture_metadata
        WHERE platform = ? AND platform_id = ?
        )");
    sqlite3_bind_int(stmt.get(), 1, static_cast<int>(platform));
    sqlite3_bind_int64(stmt.get(), 2, platformID);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        info.platformType = platform;
        info.id = platformID;
        info.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        info.authorID = sqlite3_column_int64(stmt.get(), 1);
        info.authorName = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        info.authorNick = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        info.authorDescription = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4));
        info.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5));
        info.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 6));
        info.viewCount = sqlite3_column_int(stmt.get(), 7);
        info.likeCount = sqlite3_column_int(stmt.get(), 8);
        info.bookmarkCount = sqlite3_column_int(stmt.get(), 9);
        info.replyCount = sqlite3_column_int(stmt.get(), 10);
        info.forwardCount = sqlite3_column_int(stmt.get(), 11);
        info.quoteCount = sqlite3_column_int(stmt.get(), 12);
        info.restrictType = static_cast<RestrictType>(sqlite3_column_int(stmt.get(), 13));
        info.aiType = static_cast<AIType>(sqlite3_column_int(stmt.get(), 14));
    } else {
        return info;
    }

    // query tags
    stmt = prepare(R"(
        SELECT tag_id WHERE platform = ? AND platform_id = ?
        )");
    sqlite3_bind_int(stmt.get(), 1, static_cast<int>(platform));
    sqlite3_bind_int64(stmt.get(), 2, platformID);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        uint32_t tagId = static_cast<uint32_t>(sqlite3_column_int(stmt.get(), 0));
        info.tagIds.push_back(tagId);
    }

    // query associated pictures
    if (loadAssociatedPics) {
        stmt = prepare(R"(
            SELECT ps.id
            FROM picture_source ps
            WHERE ps.platform = ? AND ps.platform_id = ?
            ORDER BY ps.image_index ASC
        )");
        sqlite3_bind_int(stmt.get(), 1, static_cast<int>(platform));
        sqlite3_bind_int64(stmt.get(), 2, platformID);
        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            uint64_t picID = static_cast<uint64_t>(sqlite3_column_int64(stmt.get(), 0));
            PicInfo picInfo = getPicInfo(picID);
            info.associatedPics.push_back(picInfo);
        }
    }

    return info;
}

// import functions

void PicDatabase::processAndImportSingleFile(const std::filesystem::path& filePath, ParserType parserType) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif" || ext == ".webp") {
            insertPicture(parsePicture(filePath, parserType));
        } else {
            switch (parserType) {
            case ParserType::PowerfulPixivDownloader: {
                std::vector<ParsedMetadata> parsedMetadata = powerfulPixivDownloaderMetadataParser(filePath);
                for (const auto& metadata : parsedMetadata) {
                    insertMetadata(metadata);
                }
                break;
            }
            case ParserType::GallerydlTwitter: {
                insertMetadata(gallerydlTwitterMetadataParser(filePath));
                break;
            }
            default:
                break;
            }
        }
    } catch (const std::exception& e) {
        Warn() << "Error processing file:" << filePath.c_str() << "Error:" << e.what();
    }
}
void PicDatabase::importFilesFromDirectory(const std::filesystem::path& directory,
                                           ParserType parserType,
                                           ImportProgressCallback progressCallback) {
    auto collectedFiles = collectFiles(directory);
    std::vector<std::filesystem::path> files;
    for (const auto& filePath : collectedFiles) {
        if (!isFileImported(filePath)) {
            files.push_back(filePath);
        }
    }
    disableForeignKeyRestriction();

    const size_t BATCH_SIZE = 1000;
    size_t processed = 0;
    for (size_t i = 0; i < files.size(); i += BATCH_SIZE) {
        beginTransaction();
        auto batch_end = std::min(i + BATCH_SIZE, files.size());
        auto start_time = std::chrono::high_resolution_clock::now();
        for (size_t j = i; j < batch_end; j++) {
            processAndImportSingleFile(files[j], parserType);
            addImportedFile(files[j]);
            processed++;
        }
        if (!commitTransaction()) {
            Warn() << "Batch commit failed, rolling back";
            rollbackTransaction();
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        double speed = (batch_end - i) / elapsed.count();
        double eta = (files.size() - processed) / speed;
        int eta_minutes = static_cast<int>(eta) / 60;
        int eta_seconds = static_cast<int>(eta) % 60;
        if (progressCallback) {
            progressCallback(processed, files.size());
        }
        // Info() << "Processed files: " << processed << " | Speed: " << speed << " files/sec | ETA: " << eta_minutes << "m "
        //        << eta_seconds << "s";
    }
    syncTables();
    enableForeignKeyRestriction();
    // Info() << "Import completed. Total files processed:" << processed;
}
void PicDatabase::syncTables(std::unordered_set<PlatformID> newMetadataIds) { // post-import operations
    SQLiteStatement stmt;
    // count tags
    if (!execute(R"(
        UPDATE tags SET count = (
            SELECT COUNT(*) FROM picture_tags WHERE tag_id = tags.tag_id
        )
    )")) {
        Warn() << "Failed to count tags:" << sqlite3_errmsg(db);
    }
    if (!execute(R"(
        UPDATE platform_tags SET count = (
            SELECT COUNT(*) FROM picture_metadata_tags WHERE tag_id = platform_tags.tag_id
        )
    )")) {
        Warn() << "Failed to count platform tags:" << sqlite3_errmsg(db);
    }
    // sync restrict_type and ai_type in pictures table
    if (newMetadataIds.empty()) {
        newMetadataIds = this->newMetadataIds;
    }
    for (const auto& metadataId : newMetadataIds) {
        stmt = prepare(R"(
            UPDATE pictures
            SET restrict_type = CASE
                WHEN restrict_type IS NULL OR restrict_type < (
                    SELECT restrict_type FROM picture_metadata WHERE platform = ? AND platform_id = ?
                )
                THEN (SELECT restrict_type FROM picture_metadata WHERE platform = ? AND platform_id = ?)
                ELSE restrict_type
            END,
            ai_type = CASE
                WHEN ai_type IS NULL OR ai_type < (
                    SELECT ai_type FROM picture_metadata WHERE platform = ? AND platform_id = ?
                )
                THEN (SELECT ai_type FROM picture_metadata WHERE platform = ? AND platform_id = ?)
                ELSE ai_type
            END
            WHERE id IN (
                SELECT id FROM picture_source WHERE platform = ? AND platform_id = ?
            )
        )");
        sqlite3_bind_int(stmt.get(), 1, static_cast<int>(metadataId.platform));
        sqlite3_bind_int64(stmt.get(), 2, metadataId.platformID);
        sqlite3_bind_int(stmt.get(), 3, static_cast<int>(metadataId.platform));
        sqlite3_bind_int64(stmt.get(), 4, metadataId.platformID);
        sqlite3_bind_int(stmt.get(), 5, static_cast<int>(metadataId.platform));
        sqlite3_bind_int64(stmt.get(), 6, metadataId.platformID);
        sqlite3_bind_int(stmt.get(), 7, static_cast<int>(metadataId.platform));
        sqlite3_bind_int64(stmt.get(), 8, metadataId.platformID);
        sqlite3_bind_int(stmt.get(), 9, static_cast<int>(metadataId.platform));
        sqlite3_bind_int64(stmt.get(), 10, metadataId.platformID);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Warn() << "Failed to sync restrict_type and ai_type for platform:" << static_cast<int>(metadataId.platform)
                   << "platform_id:" << metadataId.platformID << "Error:" << sqlite3_errmsg(db);
        }
    }
    enableForeignKeyRestriction();
}
void PicDatabase::importTagSet(const std::string& modelName, const std::vector<std::pair<std::string, bool>>& tagSet) {
    SQLiteStatement stmt;
    stmt = prepare(R"(
        INSERT OR UPDATE INTO metadata(key, value) VALUES ('tagset_model', ?)
    )");
    sqlite3_bind_text(stmt.get(), 1, modelName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert/update tagset_model: " << sqlite3_errmsg(db);
    }

    for (int tagId = 0; tagId < tagSet.size(); tagId++) {
        stmt = prepare(R"(
            INSERT INTO tags(id, tag, is_character) VALUES (?, ?, ?)
        )");
        const auto& [tag, isCharacter] = tagSet[tagId];
        sqlite3_bind_int(stmt.get(), 1, tagId);
        sqlite3_bind_text(stmt.get(), 2, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt.get(), 3, isCharacter ? 1 : 0);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Error() << "Failed to insert tag: " << sqlite3_errmsg(db);
        }
    }
}
bool PicDatabase::isFileImported(const std::filesystem::path& filePath) const {
    std::string dir = filePath.parent_path().string();
    std::string filename = filePath.filename().string();
    if (importedFiles.find(dir) != importedFiles.end()) {
        if (importedFiles.at(dir).find(filename) != importedFiles.at(dir).end()) {
            return true;
        }
    }
    return false;
}
void PicDatabase::addImportedFile(const std::filesystem::path& filePath) {
    if (isFileImported(filePath)) return;
    SQLiteStatement stmt;

    std::string dir = filePath.parent_path().string();
    std::string filename = filePath.filename().string();
    if (importedFiles.find(dir) == importedFiles.end()) {
        importedFiles[dir] = std::unordered_set<std::string>();
    }
    importedFiles[dir].insert(filename);

    // Step 1: Insert or ignore the directory
    stmt = prepare(R"(
        INSERT INTO imported_directories (dir_path)
        VALUES (?)
        ON CONFLICT(dir_path) DO NOTHING
    )");
    sqlite3_bind_text(stmt.get(), 1, dir.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert directory: " << sqlite3_errmsg(db);

        return;
    }

    // Step 2: Get the dir_id
    int64_t dirId = -1;
    stmt = prepare(R"(
        SELECT dir_id FROM imported_directories WHERE dir_path = ?
    )");
    sqlite3_bind_text(stmt.get(), 1, dir.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        dirId = sqlite3_column_int64(stmt.get(), 0);
    } else {
        Error() << "Failed to fetch dir_id: " << sqlite3_errmsg(db);
        return;
    }

    // Step 3: Insert the file
    stmt = prepare(R"(
        INSERT INTO imported_files (dir_id, filename)
        VALUES (?, ?)
        ON CONFLICT(dir_id, filename) DO NOTHING
    )");
    sqlite3_bind_int64(stmt.get(), 1, dirId);
    sqlite3_bind_text(stmt.get(), 2, filename.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        Error() << "Failed to insert imported file: " << sqlite3_errmsg(db);
    }
}

// Search functions

std::vector<uint64_t> PicDatabase::tagSearch(const std::unordered_set<uint32_t>& includedTagIds,
                                             const std::unordered_set<uint32_t>& excludedTagIds) const {
    std::vector<uint64_t> results;
    if (includedTagIds.empty() && excludedTagIds.empty()) return results;

    std::string includedTagIdStr;
    std::string excludedTagIdStr;
    if (!includedTagIds.empty()) {
        int idx = 0;
        for (const auto& tagId : includedTagIds) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += std::to_string(tagId);
        }
    }
    if (!excludedTagIds.empty()) {
        int idx = 0;
        for (const auto& tagId : excludedTagIds) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += std::to_string(tagId);
        }
    }
    std::string sql = "SELECT id FROM picture_tags WHERE 1=1 AND tag_id IN (" + includedTagIdStr +
                      ") AND id NOT IN (SELECT id FROM picture_tags WHERE tag_id IN (" + excludedTagIdStr +
                      ")) GROUP BY id HAVING COUNT(DISTINCT tag_id) = " + std::to_string(includedTagIds.size());

    SQLiteStatement stmt;
    stmt = prepare(sql.c_str());
    if (!stmt.get()) {
        Error() << "Failed to prepare tag search statement:" << sqlite3_errmsg(db);
        return results;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        results.push_back(int64_to_uint64(sqlite3_column_int64(stmt.get(), 0)));
    }
    return results;
}
std::vector<PlatformID> PicDatabase::platformTagSearch(const std::unordered_set<uint32_t>& includedTagIds,
                                                       const std::unordered_set<uint32_t>& excludedTagIds) const {
    std::vector<PlatformID> results;
    if (includedTagIds.empty() && excludedTagIds.empty()) return results;

    std::string includedTagIdStr;
    std::string excludedTagIdStr;
    if (!includedTagIds.empty()) {
        int idx = 0;
        for (const auto& tagId : includedTagIds) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += std::to_string(tagId);
        }
    }
    if (!excludedTagIds.empty()) {
        int idx = 0;
        for (const auto& tagId : excludedTagIds) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += std::to_string(tagId);
        }
    }
    std::string sql = "SELECT platform, platform_id FROM picture_metadata_tags WHERE tag_id IN (" + includedTagIdStr +
                      ")"
                      " AND NOT EXISTS ( SELECT 1 FROM picture_metadata_tags AS excluded WHERE excluded.tag_id IN (" +
                      excludedTagIdStr +
                      ")"
                      " AND excluded.platform = picture_metadata_tags.platform"
                      " AND excluded.platform_id = picture_metadata_tags.platform_id)"
                      " GROUP BY platform, platform_id HAVING COUNT(DISTINCT tag_id) = " +
                      std::to_string(includedTagIds.size());

    SQLiteStatement stmt = prepare(sql);
    if (!stmt.get()) {
        Error() << "Failed to prepare platform tag search statement:" << sqlite3_errmsg(db);
        return results;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        PlatformID platformID;
        platformID.platform = static_cast<PlatformType>(sqlite3_column_int(stmt.get(), 0));
        platformID.platformID = sqlite3_column_int64(stmt.get(), 1);
        results.push_back(platformID);
    }
    return results;
}
std::unordered_set<PlatformID>
PicDatabase::textSearch(const std::string& searchText, PlatformType platformType, SearchField searchField) const {
    std::unordered_set<PlatformID> results;
    if (searchText.empty() || searchField == SearchField::None) return results;
    SQLiteStatement stmt;
    bool isNumeric =
        !searchText.empty() && std::all_of(searchText.begin(), searchText.end(), [](char c) { return std::isdigit(c); });

    std::string searchFieldStr;
    std::string likePattern;
    switch (searchField) {
    case SearchField::PlatformID:
        searchFieldStr = "platform_id";
        break;
    case SearchField::AuthorID:
        searchFieldStr = "author_id";
        break;
    case SearchField::AuthorName:
        searchFieldStr = "author_name";
        break;
    case SearchField::AuthorNick:
        searchFieldStr = "author_nick";
        break;
    case SearchField::Title:
        searchFieldStr = "title";
        break;
    default:
        return results;
    }

    switch (searchField) {
    case SearchField::PlatformID:
    case SearchField::AuthorID:
        if (isNumeric) {
            if (platformType != PlatformType::Unknown) {
                searchFieldStr = "platform = " + std::to_string(static_cast<int>(platformType)) + " AND " + searchFieldStr;
            }
            stmt = prepare("SELECT platform, platform_id FROM picture_metadata WHERE " + searchFieldStr + " = ?");
            sqlite3_bind_int64(stmt.get(), 1, std::stoll(searchText));
        } else {
            return results;
        }
        break;
    case SearchField::AuthorName:
    case SearchField::AuthorNick:
    case SearchField::Title:
        if (platformType != PlatformType::Unknown) {
            searchFieldStr = "platform = " + std::to_string(static_cast<int>(platformType)) + " AND " + searchFieldStr;
        }
        stmt = prepare(" SELECT platform, platform_id FROM picture_metadata WHERE " + searchFieldStr + " LIKE ?");
        likePattern = searchText + "%";
        sqlite3_bind_text(stmt.get(), 1, likePattern.c_str(), -1, SQLITE_TRANSIENT);
        break;
    default:
        return results;
    }

    if (!stmt.get()) {
        Error() << "Failed to prepare text search statement:" << sqlite3_errmsg(db);
        return results;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        PlatformID platformID;
        platformID.platform = static_cast<PlatformType>(sqlite3_column_int(stmt.get(), 0));
        platformID.platformID = sqlite3_column_int64(stmt.get(), 1);
        results.insert(platformID);
    }
    return results;
}

// Get tag lists

std::vector<std::pair<StringTag, uint32_t>> PicDatabase::getTagCounts() const {
    std::vector<std::pair<StringTag, uint32_t>> tagCounts;
    SQLiteStatement stmt = prepare("SELECT tag, count, is_character FROM tags ORDER BY count DESC");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        uint32_t count = sqlite3_column_int(stmt.get(), 1);
        bool isCharacter = sqlite3_column_int(stmt.get(), 2) != 0;
        tagCounts.emplace_back(StringTag{tag, isCharacter}, count);
    }
    return tagCounts;
}
std::vector<std::pair<PlatformStringTag, uint32_t>> PicDatabase::getPlatformTagCounts() const {
    std::vector<std::pair<PlatformStringTag, uint32_t>> tagCounts;
    SQLiteStatement stmt = prepare("SELECT platform, tag, count FROM platform_tags ORDER BY count DESC");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        PlatformType platform = static_cast<PlatformType>(sqlite3_column_int(stmt.get(), 0));
        std::string tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        int count = sqlite3_column_int(stmt.get(), 2);
        tagCounts.emplace_back(PlatformStringTag{platform, tag}, count);
    }
    return tagCounts;
}

// Multi-threaded importer implementation

MultiThreadedImporter::MultiThreadedImporter(const std::filesystem::path& directory,
                                             ImportProgressCallback progressCallback,
                                             ParserType parserType,
                                             std::string dbFile,
                                             size_t threadCount)
    : parserType(parserType), progressCallback(progressCallback), dbFile(dbFile) {
    importDirectory = directory;
    insertThread = std::thread(&MultiThreadedImporter::insertThreadFunc, this);
    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back(&MultiThreadedImporter::workerThreadFunc, this);
    }
}
MultiThreadedImporter::~MultiThreadedImporter() {
    forceStop();
    finish();
}
void MultiThreadedImporter::forceStop() {
    if (finished) return;
    stopFlag.store(true);
    finish();
}
bool MultiThreadedImporter::finish() {
    if (finished) return true;
    if (importedCount < supportedFileCount.load(std::memory_order_relaxed) && !stopFlag.load()) {
        return false; // 还有文件未处理完
    }
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers.clear();
    cv.notify_all();
    if (insertThread.joinable()) {
        insertThread.join();
    }
    finished = true;
    return true;
}
bool processSingleFile(const std::filesystem::path& filePath,
                       ParserType parserType,
                       std::vector<ParsedPicture>& parsedPictures,
                       std::vector<std::vector<ParsedMetadata>>& parsedMetadataVecs) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif" || ext == ".webp") {
            parsedPictures.emplace_back(parsePicture(filePath, parserType));
            return true;
        } else {
            switch (parserType) {
            case ParserType::PowerfulPixivDownloader: {
                std::vector<ParsedMetadata> parsedMetadata = powerfulPixivDownloaderMetadataParser(filePath);
                if (!parsedMetadata.empty()) {
                    parsedMetadataVecs.push_back(std::move(parsedMetadata));
                    return true;
                }
                break;
            }
            case ParserType::GallerydlTwitter: {
                ParsedMetadata parsedMetadata = gallerydlTwitterMetadataParser(filePath);
                if (parsedMetadata.platformType != PlatformType::Unknown) {
                    parsedMetadataVecs.push_back({std::move(parsedMetadata)});
                    return true;
                }
                break;
            }
            default:
                break;
            }
        }
    } catch (const std::exception& e) {
        Error() << "Error processing file:" << filePath << "Error:" << e.what();
        return false;
    }
    return false;
}
void MultiThreadedImporter::workerThreadFunc() {
    std::vector<ParsedPicture> parsedPictures;
    parsedPictures.reserve(500);
    std::vector<std::vector<ParsedMetadata>> ParsedMetadataVecs;
    ParsedMetadataVecs.reserve(500);
    size_t index = 0;

    // Wait until files are collected
    while (!readyFlag.load(std::memory_order_acquire) && !stopFlag.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (stopFlag.load()) return;

    while ((index = nextFileIndex.fetch_add(1, std::memory_order_relaxed)) < files.size()) {
        if (parsedPictures.size() >= 100) {
            if (parsedPicQueueMutex.try_lock()) {
                while (!parsedPictures.empty()) {
                    parsedPictureQueue.push(std::move(parsedPictures.back()));
                    parsedPictures.pop_back();
                }
                parsedPicQueueMutex.unlock();
                parsedPictures.clear();
                cv.notify_one();
            }
        }
        if (ParsedMetadataVecs.size() >= 100) {
            if (parsedMetadataQueueMutex.try_lock()) {
                while (!ParsedMetadataVecs.empty()) {
                    metadataVecQueue.push(std::move(ParsedMetadataVecs.back()));
                    ParsedMetadataVecs.pop_back();
                }
                parsedMetadataQueueMutex.unlock();
                ParsedMetadataVecs.clear();
                cv.notify_one();
            }
        }
        if (stopFlag.load()) return;
        const auto& filePath = files[index];
        if (!processSingleFile(filePath, parserType, parsedPictures, ParsedMetadataVecs))
            supportedFileCount.fetch_sub(1, std::memory_order_relaxed);
    }
    if (!parsedPictures.empty()) {
        std::lock_guard<std::mutex> lock(parsedPicQueueMutex);
        while (!parsedPictures.empty()) {
            parsedPictureQueue.push(std::move(parsedPictures.back()));
            parsedPictures.pop_back();
        }
        parsedPictures.clear();
        cv.notify_one();
    }
    if (!ParsedMetadataVecs.empty()) {
        std::lock_guard<std::mutex> lock(parsedPicQueueMutex);
        while (!ParsedMetadataVecs.empty()) {
            metadataVecQueue.push(std::move(ParsedMetadataVecs.back()));
            ParsedMetadataVecs.pop_back();
        }
        ParsedMetadataVecs.clear();
        cv.notify_one();
    }
}
void MultiThreadedImporter::insertThreadFunc() {
    PicDatabase threadDb(dbFile, DbMode::Import);
    std::mutex conditionMutex;
    std::vector<ParsedPicture> picsToInsert;
    picsToInsert.reserve(1000);
    std::vector<std::vector<ParsedMetadata>> metadataVecsToInsert;
    metadataVecsToInsert.reserve(1000);

    // Collect files to import
    for (const auto& entry : std::filesystem::recursive_directory_iterator(importDirectory)) {
        if (!entry.is_regular_file() || threadDb.isFileImported(entry.path())) {
            continue;
        }
        files.push_back(entry.path());
    }
    supportedFileCount = files.size();
    Info() << "Total files to import: " << supportedFileCount.load();
    readyFlag.store(true, std::memory_order_release);

    threadDb.beginTransaction();
    while (!stopFlag.load() && importedCount < supportedFileCount.load(std::memory_order_relaxed)) {
        if (progressCallback) progressCallback(importedCount, supportedFileCount.load(std::memory_order_relaxed));
        std::unique_lock<std::mutex> lock(conditionMutex);
        cv.wait(lock, [this]() { return !parsedPictureQueue.empty() || !metadataVecQueue.empty() || stopFlag.load(); });
        if (stopFlag.load()) break;
        if (!parsedPictureQueue.empty()) {
            if (parsedPicQueueMutex.try_lock()) {
                while (!parsedPictureQueue.empty()) {
                    picsToInsert.push_back(std::move(parsedPictureQueue.front()));
                    parsedPictureQueue.pop();
                }
                parsedPicQueueMutex.unlock();
                for (const auto& picInfo : picsToInsert) {
                    threadDb.insertPicture(picInfo);
                    importedCount++;
                }
                picsToInsert.clear();
            }
        }
        if (!metadataVecQueue.empty()) {
            if (parsedMetadataQueueMutex.try_lock()) {
                while (!metadataVecQueue.empty()) {
                    metadataVecsToInsert.push_back(std::move(metadataVecQueue.front()));
                    metadataVecQueue.pop();
                }
                parsedMetadataQueueMutex.unlock();
                for (const auto& metadataVec : metadataVecsToInsert) {
                    for (const auto& metadataInfo : metadataVec) {
                        if (metadataInfo.updateIfExists) {
                            threadDb.updateMetadata(metadataInfo);
                        } else {
                            threadDb.insertMetadata(metadataInfo);
                        }
                    }
                    importedCount++;
                }
                metadataVecsToInsert.clear();
            }
        }
    }
    Info() << "Finalizing import";
    if (stopFlag.load()) {
        Info() << "Import stopped by user, rolling back. " << "Directory: " << importDirectory;
        threadDb.rollbackTransaction();
        return;
    }
    threadDb.syncTables();
    for (const auto& filePath : files) {
        threadDb.addImportedFile(filePath);
    }
    if (!threadDb.commitTransaction()) {
        Error() << "Import commit failed, rolling back. " << "Directory: " << importDirectory;
        threadDb.rollbackTransaction();
    }
    Info() << "Import completed. Directory: " << importDirectory;
    if (progressCallback) progressCallback(importedCount, supportedFileCount.load(std::memory_order_seq_cst));
}
