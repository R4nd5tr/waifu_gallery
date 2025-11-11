/*
 * Waifu Gallery - A Qt-based image gallery application.
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

PicDatabase::PicDatabase(const std::string& databaseFile, DbMode mode) {
    initDatabase(databaseFile);
    setMode(mode);
    if (mode == DbMode::Import) return;
    getTagMapping();
}
PicDatabase::~PicDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}
void PicDatabase::initDatabase(const std::string& databaseFile) {
    if (sqlite3_open(databaseFile.c_str(), &db) != SQLITE_OK) {
        Error() << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    if (!createTables()) {
        Error() << "Failed to create tables";
        return;
    }
    Info() << "Database initialized";
}
bool PicDatabase::createTables() {
    const std::string picturesTable = R"(
        CREATE TABLE IF NOT EXISTS pictures (
            id INTEGER PRIMARY KEY NOT NULL,
            width INTEGER,
            height INTEGER,
            size INTEGER,
            file_type INTEGER,
            edit_time TEXT DEFAULT NULL,
            download_time TEXT DEFAULT NULL,
            x_restrict INTEGER DEFAULT 0,
            ai_type INTEGER DEFAULT 0,
            phash TEXT DEFAULT NULL
        )
    )"; // TODO: add feature vector from deep learning models
    const std::string pictureTagsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tags (
            id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            probability REAL DEFAULT 1.0,
            PRIMARY KEY (id, tag_id),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
        )
    )";
    const std::string pictureFilesTable = R"(
        CREATE TABLE IF NOT EXISTS picture_file_paths (
            id INTEGER NOT NULL,
            file_path TEXT NOT NULL,
            PRIMARY KEY (id, file_path),
            UNIQUE (file_path),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const std::string picturePixivIdsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_pixiv_ids (
            id INTEGER NOT NULL,
            pixiv_id INTEGER NOT NULL,
            pixiv_index INTEGER NOT NULL,
            PRIMARY KEY (pixiv_id, pixiv_index),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const std::string pictureTweetIdsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tweet_ids (
            id INTEGER NOT NULL,
            tweet_id INTEGER NOT NULL,
            tweet_index INTEGER NOT NULL,
            PRIMARY KEY (tweet_id, tweet_index),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const std::string tagsTable = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY,
            tag TEXT NOT NULL UNIQUE,
            is_character BOOLEAN,
            count INTEGER DEFAULT 0
        )
    )";
    const std::string tweetsTable = R"(
        CREATE TABLE IF NOT EXISTS tweets (
            tweet_id INTEGER PRIMARY KEY NOT NULL,
            date DATE NOT NULL,
            author_id INTEGER NOT NULL,
            author_name TEXT NOT NULL,
            author_nick TEXT,
            author_description TEXT,
            author_profile_image TEXT,
            author_profile_banner TEXT,
            favorite_count INTEGER DEFAULT 0,
            quote_count INTEGER DEFAULT 0,
            reply_count INTEGER DEFAULT 0,
            retweet_count INTEGER DEFAULT 0,
            bookmark_count INTEGER DEFAULT 0,
            view_count INTEGER DEFAULT 0,
            description TEXT
        )
    )";
    const std::string tweetHashtagsTable = R"(
        CREATE TABLE IF NOT EXISTS tweet_hashtags (
            tweet_id INTEGER NOT NULL,
            hashtag_id INTEGER NOT NULL,
            PRIMARY KEY (tweet_id, hashtag_id),

            FOREIGN KEY (tweet_id) REFERENCES tweets(tweet_id) ON DELETE CASCADE,
            FOREIGN KEY (hashtag_id) REFERENCES twitter_hashtags(id) ON DELETE CASCADE
        )
    )";
    const std::string twitterHashtagsTable = R"(
        CREATE TABLE IF NOT EXISTS twitter_hashtags (
            id INTEGER PRIMARY KEY,
            hashtag TEXT NOT NULL UNIQUE,
            count INTEGER DEFAULT 0
        )
    )";
    const std::string pixivArtworksTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_artworks (
            pixiv_id INTEGER PRIMARY KEY NOT NULL,
            date DATE NOT NULL,
            author_name TEXT NOT NULL,
            author_id INTEGER NOT NULL,
            title TEXT,
            description TEXT,
            like_count INTEGER DEFAULT 0,
            view_count INTEGER DEFAULT 0,
            x_restrict INTEGER DEFAULT 0,
            ai_type INTEGER DEFAULT 0 
        )
    )";
    const std::string pixivArtworksTagsTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_artworks_tags (
            pixiv_id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            PRIMARY KEY (pixiv_id, tag_id),

            FOREIGN KEY (pixiv_id) REFERENCES pixiv_artworks(pixiv_id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES pixiv_tags(id) ON DELETE CASCADE
        )
    )";
    const std::string pixivTagsTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_tags (
            id INTEGER PRIMARY KEY,
            tag TEXT NOT NULL UNIQUE,
            translated_tag TEXT,
            count INTEGER DEFAULT 0
        )
    )";
    const std::vector<std::string> tables = {// pixiv
                                             pixivArtworksTable,
                                             pixivArtworksTagsTable,
                                             pixivTagsTable,
                                             // tweet
                                             tweetsTable,
                                             tweetHashtagsTable,
                                             twitterHashtagsTable,
                                             // pictures
                                             picturesTable,
                                             pictureTagsTable,
                                             pictureFilesTable,
                                             picturePixivIdsTable,
                                             pictureTweetIdsTable,
                                             tagsTable};
    const std::vector<std::string> indexes = {
        // foreign key indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_pixiv_ids_id ON picture_pixiv_ids(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_pixiv_ids_pixiv_id ON picture_pixiv_ids(pixiv_id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tweet_ids_id ON picture_tweet_ids(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tweet_ids_tweet_id ON picture_tweet_ids(tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_id ON picture_tags(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_file_paths_id ON picture_file_paths(id)",
        "CREATE INDEX IF NOT EXISTS idx_tweet_hashtags_id ON tweet_hashtags(tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags_id ON pixiv_artworks_tags(pixiv_id)",
        // tags indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_tags ON picture_tags(tag_id)",
        "CREATE INDEX IF NOT EXISTS idx_tweet_hashtags ON tweet_hashtags(hashtag_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags ON pixiv_artworks_tags(tag_id)",
        "CREATE INDEX IF NOT EXISTS idx_tags_tag ON tags(tag)",
        "CREATE INDEX IF NOT EXISTS idx_twitter_hashtags_hashtag ON twitter_hashtags(hashtag)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_tags_tag ON pixiv_tags(tag)",
        // author indexes
        "CREATE INDEX IF NOT EXISTS idx_tweets_author_id ON tweets(author_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_author_id ON pixiv_artworks(author_id)",
        // tag search indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_id ON picture_tags(tag_id, id)",
        "CREATE INDEX IF NOT EXISTS idx_tweet_hashtags_tweet_id ON tweet_hashtags(hashtag_id, tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags_pixiv_id ON pixiv_artworks_tags(tag_id, pixiv_id)"};
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
void PicDatabase::getTagMapping() {
    tagToId.clear();
    idToTag.clear();
    twitterHashtagToId.clear();
    idToTwitterHashtag.clear();
    pixivTagToId.clear();
    idToPixivTag.clear();

    sqlite3_stmt* stmt = nullptr;

    stmt = prepare("SELECT id, tag FROM tags");
    if (!stmt) {
        Warn() << "Failed to prepare statement for fetching tags.";
        return;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tagToId[tag] = id;
        idToTag[id] = tag;
    }
    sqlite3_finalize(stmt);

    stmt = prepare("SELECT id, hashtag FROM twitter_hashtags");
    if (!stmt) {
        Warn() << "Failed to prepare statement for fetching twitter hashtags.";
        return;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* hashtag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        twitterHashtagToId[hashtag] = id;
        idToTwitterHashtag[id] = hashtag;
    }
    sqlite3_finalize(stmt);
    stmt = prepare("SELECT id, tag FROM pixiv_tags");
    if (!stmt) {
        Warn() << "Failed to prepare statement for fetching pixiv tags.";
        return;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        pixivTagToId[tag] = id;
        idToPixivTag[id] = tag;
    }
    sqlite3_finalize(stmt);
    Info() << "Tag mappings loaded. Tags:" << tagToId.size() << "Twitter Hashtags:" << twitterHashtagToId.size()
           << "Pixiv Tags:" << pixivTagToId.size();
}
bool PicDatabase::insertPicInfo(const PicInfo& picInfo) {
    if (!insertPicture(picInfo)) {
        Error() << "insertPicture failed.";
        return false;
    }
    if (!insertPictureFilePath(picInfo)) {
        Error() << "insertPictureFilePath failed.";
        return false;
    }
    if (!insertPictureTags(picInfo)) {
        Error() << "insertPictureTags failed.";
        return false;
    }
    if (!insertPicturePixivId(picInfo)) {
        Error() << "insertPicturePixivId failed.";
        return false;
    }
    if (!insertPictureTweetId(picInfo)) {
        Error() << "insertPictureTweetId failed.";
        return false;
    }
    return true;
}
bool PicDatabase::insertPicture(const PicInfo& picInfo) {
    sqlite3_stmt* stmt = nullptr;
    stmt = prepare(R"(
        INSERT OR IGNORE INTO pictures(
            id, width, height, size, file_type, x_restrict, edit_time, download_time
        ) VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?
        )
    )");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(picInfo.id));
    sqlite3_bind_int(stmt, 2, picInfo.width);
    sqlite3_bind_int(stmt, 3, picInfo.height);
    sqlite3_bind_int64(stmt, 4, picInfo.size);
    sqlite3_bind_int(stmt, 5, static_cast<int>(picInfo.fileType));
    sqlite3_bind_int(stmt, 6, static_cast<int>(picInfo.xRestrict));
    sqlite3_bind_text(stmt, 7, picInfo.editTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, picInfo.downloadTime.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Error() << "Failed to insert picture: " << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}
bool PicDatabase::insertPictureFilePath(const PicInfo& picInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& filePath : picInfo.filePaths) {
        stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_file_paths(
                id, file_path
            ) VALUES (?, ?)
        )");
        sqlite3_bind_int64(stmt, 1, uint64_to_int64(picInfo.id));
        sqlite3_bind_text(stmt, 2, filePath.generic_u8string().c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert picture_file_path: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}
bool PicDatabase::insertPictureTags(const PicInfo& picInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& picTag : picInfo.tags) {
        stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_tags(
                id, tag_id, probability
            ) VALUES (?, ?, ?)
        )");
        sqlite3_bind_int64(stmt, 1, uint64_to_int64(picInfo.id));
        sqlite3_bind_int(stmt, 2, tagToId[picTag.tag]);
        sqlite3_bind_double(stmt, 3, picTag.probability);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert picture_tag: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}
bool PicDatabase::insertPictureTags(uint64_t id, int tagId, float probability) {
    sqlite3_stmt* stmt = nullptr;
    stmt = prepare(R"(
        INSERT OR IGNORE INTO picture_tags(
            id, tag_id, probability
        ) VALUES (?, ?, ?)
    )");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
    sqlite3_bind_int(stmt, 2, tagId);
    sqlite3_bind_double(stmt, 3, probability);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Error() << "Failed to insert picture_tag: " << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}
bool PicDatabase::insertPicturePixivId(const PicInfo& picInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& pair : picInfo.pixivIdIndices) {
        uint64_t pixivId = pair.first;
        int pixivIndex = pair.second;
        stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_pixiv_ids(
                id, pixiv_id, pixiv_index
            ) VALUES (
                ?, ?, ?
            )
        )");
        sqlite3_bind_int64(stmt, 1, uint64_to_int64(picInfo.id));
        sqlite3_bind_int64(stmt, 2, pixivId);
        sqlite3_bind_int(stmt, 3, pixivIndex);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert picture_pixiv_id: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}
bool PicDatabase::insertPictureTweetId(const PicInfo& picInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& pair : picInfo.tweetIdIndices) {
        uint64_t tweetId = pair.first;
        int tweetIndex = pair.second;
        stmt = prepare(R"(
            INSERT OR IGNORE INTO picture_tweet_ids(
                id, tweet_id, tweet_index
            ) VALUES (
                ?, ?, ?
            )
        )");
        sqlite3_bind_int64(stmt, 1, uint64_to_int64(picInfo.id));
        sqlite3_bind_int64(stmt, 2, tweetId);
        sqlite3_bind_int(stmt, 3, tweetIndex);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert picture_tweet_id: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}
bool PicDatabase::insertTweetInfo(const TweetInfo& tweetInfo) {
    if (!insertTweet(tweetInfo)) {
        Error() << "insertTweet failed.";
        return false;
    }
    if (!insertTweetHashtags(tweetInfo)) {
        Error() << "insertTweetHashtags failed.";
        return false;
    }
    return true;
}
bool PicDatabase::insertTweet(const TweetInfo& tweetInfo) {
    sqlite3_stmt* stmt = nullptr;
    stmt = prepare(R"(
        INSERT OR IGNORE INTO tweets(
            tweet_id, date, author_id, author_name, author_nick, 
            author_description, author_profile_image, author_profile_banner,
            favorite_count, quote_count, reply_count, retweet_count, bookmark_count, view_count, description
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    sqlite3_bind_int64(stmt, 1, tweetInfo.tweetID);
    sqlite3_bind_text(stmt, 2, tweetInfo.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, tweetInfo.authorID);
    sqlite3_bind_text(stmt, 4, tweetInfo.authorName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, tweetInfo.authorNick.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, tweetInfo.authorDescription.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, tweetInfo.authorProfileImage.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, tweetInfo.authorProfileBanner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, tweetInfo.favoriteCount);
    sqlite3_bind_int(stmt, 10, tweetInfo.quoteCount);
    sqlite3_bind_int(stmt, 11, tweetInfo.replyCount);
    sqlite3_bind_int(stmt, 12, tweetInfo.retweetCount);
    sqlite3_bind_int(stmt, 13, tweetInfo.bookmarkCount);
    sqlite3_bind_int(stmt, 14, tweetInfo.viewCount);
    sqlite3_bind_text(stmt, 15, tweetInfo.description.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Error() << "Failed to insert tweet: " << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}
bool PicDatabase::insertTweetHashtags(const TweetInfo& tweetInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& hashtag : tweetInfo.hashtags) {
        if (twitterHashtagToId.find(hashtag) == twitterHashtagToId.end()) {
            stmt = prepare(R"(
                INSERT OR IGNORE INTO twitter_hashtags(hashtag, count) VALUES (?, 0)
            )");
            sqlite3_bind_text(stmt, 1, hashtag.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                Error() << "Failed to insert twitter hashtag: " << sqlite3_errmsg(db);
                sqlite3_finalize(stmt);
                return false;
            }
            int newId = sqlite3_last_insert_rowid(db);
            twitterHashtagToId[hashtag] = newId;
            idToTwitterHashtag[newId] = hashtag;
            sqlite3_finalize(stmt);
        }
        stmt = prepare(R"(
            INSERT OR IGNORE INTO tweet_hashtags(
                tweet_id, hashtag_id
            ) VALUES (?, ?)
        )");
        sqlite3_bind_int64(stmt, 1, tweetInfo.tweetID);
        sqlite3_bind_int(stmt, 2, twitterHashtagToId[hashtag]);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert tweet_hashtag: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    return true;
}
bool PicDatabase::insertPixivInfo(const PixivInfo& pixivInfo) {
    if (!insertPixivArtwork(pixivInfo)) {
        Error() << "insertPixivArtwork failed.";
        return false;
    }
    if (!insertPixivArtworkTags(pixivInfo)) {
        Error() << "insertPixivArtworkTags failed.";
        return false;
    }
    return true;
}
bool PicDatabase::insertPixivArtwork(const PixivInfo& pixivInfo) {
    sqlite3_stmt* stmt = nullptr;
    stmt = prepare(R"(
        INSERT OR IGNORE INTO pixiv_artworks(
            pixiv_id, date, author_name, author_id, title, description,
            like_count, view_count, x_restrict, ai_type
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    sqlite3_bind_int64(stmt, 1, pixivInfo.pixivID);
    sqlite3_bind_text(stmt, 2, pixivInfo.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pixivInfo.authorName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, pixivInfo.authorID);
    sqlite3_bind_text(stmt, 5, pixivInfo.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, pixivInfo.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, pixivInfo.likeCount);
    sqlite3_bind_int(stmt, 8, pixivInfo.viewCount);
    sqlite3_bind_int(stmt, 9, static_cast<int>(pixivInfo.xRestrict));
    sqlite3_bind_int(stmt, 10, static_cast<int>(pixivInfo.aiType));
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Error() << "Failed to insert pixiv_artwork: " << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    newPixivIDs.insert(pixivInfo.pixivID);
    return true;
}
bool PicDatabase::insertPixivArtworkTags(const PixivInfo& pixivInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& tag : pixivInfo.tags) {
        if (pixivTagToId.find(tag) == pixivTagToId.end()) {
            stmt = prepare(R"(
                INSERT OR IGNORE INTO pixiv_tags(tag, count) VALUES (?, 0)
            )");
            sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                Error() << "Failed to insert pixiv tag: " << sqlite3_errmsg(db);
                sqlite3_finalize(stmt);
                return false;
            }
            int newId = sqlite3_last_insert_rowid(db);
            pixivTagToId[tag] = newId;
            idToPixivTag[newId] = tag;
            sqlite3_finalize(stmt);
        }
        stmt = prepare(R"(
            INSERT OR IGNORE INTO pixiv_artworks_tags(
                pixiv_id, tag_id
            ) VALUES (?, ?)
        )");
        sqlite3_bind_int64(stmt, 1, pixivInfo.pixivID);
        sqlite3_bind_int64(stmt, 2, pixivTagToId.at(tag));
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert pixiv_artworks_tags: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    if (pixivInfo.tags.size() == pixivInfo.tagsTransl.size()) {
        for (size_t i = 0; i < pixivInfo.tags.size(); ++i) {
            stmt = prepare(R"(
                UPDATE pixiv_tags SET translated_tag = ? WHERE id = ?
            )");
            sqlite3_bind_text(stmt, 1, pixivInfo.tagsTransl[i].c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, pixivTagToId[pixivInfo.tags[i]]);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                Error() << "Failed to update translated_tag: " << sqlite3_errmsg(db);
                sqlite3_finalize(stmt);
                return false;
            }
            sqlite3_finalize(stmt);
        }
    }
    return true;
}
bool PicDatabase::updatePixivInfo(const PixivInfo& pixivInfo) {
    if (!updatePixivArtwork(pixivInfo)) {
        Error() << "updatePixivArtwork failed.";
        return false;
    }
    if (!updatePixivArtworkTags(pixivInfo)) {
        Error() << "updatePixivArtworkTags failed.";
        return false;
    }
    return true;
}
bool PicDatabase::updatePixivArtwork(const PixivInfo& pixivInfo) {
    sqlite3_stmt* stmt = nullptr;
    stmt = prepare(R"(
        INSERT INTO pixiv_artworks(
            pixiv_id, date, author_name, author_id, title, description,
            like_count, view_count, x_restrict, ai_type
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(pixiv_id) DO UPDATE SET
            date=excluded.date,
            author_name=excluded.author_name,
            author_id=excluded.author_id,
            title=excluded.title,
            description=excluded.description,
            like_count=excluded.like_count,
            view_count=excluded.view_count,
            -- only update x_restrict when excluded.x_restrict is not NULL and greater than existing
            x_restrict = CASE
                WHEN excluded.x_restrict IS NOT NULL
                     AND (pixiv_artworks.x_restrict IS NULL OR excluded.x_restrict > pixiv_artworks.x_restrict)
                THEN excluded.x_restrict
                ELSE pixiv_artworks.x_restrict
            END,
            -- only update ai_type when excluded.ai_type is not NULL and greater than existing
            ai_type = CASE
                WHEN excluded.ai_type IS NOT NULL
                     AND (pixiv_artworks.ai_type IS NULL OR excluded.ai_type > pixiv_artworks.ai_type)
                THEN excluded.ai_type
                ELSE pixiv_artworks.ai_type
            END
    )");
    sqlite3_bind_int64(stmt, 1, pixivInfo.pixivID);
    sqlite3_bind_text(stmt, 2, pixivInfo.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pixivInfo.authorName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, pixivInfo.authorID);
    sqlite3_bind_text(stmt, 5, pixivInfo.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, pixivInfo.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, pixivInfo.likeCount);
    sqlite3_bind_int(stmt, 8, pixivInfo.viewCount);
    sqlite3_bind_int(stmt, 9, static_cast<int>(pixivInfo.xRestrict));
    sqlite3_bind_int(stmt, 10, static_cast<int>(pixivInfo.aiType));
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Error() << "Failed to update pixiv_artwork: " << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }
    newPixivIDs.insert(pixivInfo.pixivID);
    sqlite3_finalize(stmt);
    return true;
}
bool PicDatabase::updatePixivArtworkTags(const PixivInfo& pixivInfo) {
    sqlite3_stmt* stmt = nullptr;
    for (const auto& tag : pixivInfo.tags) {
        if (pixivTagToId.find(tag) == pixivTagToId.end()) {
            stmt = prepare(R"(
                INSERT INTO pixiv_tags(tag, count) VALUES (?, 0)
                ON CONFLICT(tag) DO NOTHING
            )");
            sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                Error() << "Failed to insert pixiv tag: " << sqlite3_errmsg(db);
                sqlite3_finalize(stmt);
                return false;
            }
            int newId = sqlite3_last_insert_rowid(db);
            pixivTagToId[tag] = newId;
            idToPixivTag[newId] = tag;
            sqlite3_finalize(stmt);
        }
        stmt = prepare(R"(
            INSERT INTO pixiv_artworks_tags(
                pixiv_id, tag_id
            ) VALUES (?, ?)
            ON CONFLICT(pixiv_id, tag_id) DO NOTHING
        )");
        sqlite3_bind_int64(stmt, 1, pixivInfo.pixivID);
        sqlite3_bind_int64(stmt, 2, pixivTagToId.at(tag));
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to update pixiv_artworks_tag: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
    }
    if (pixivInfo.tags.size() == pixivInfo.tagsTransl.size()) {
        for (size_t i = 0; i < pixivInfo.tags.size(); ++i) {
            stmt = prepare(R"(
                UPDATE pixiv_tags SET translated_tag = ? WHERE id = ?
            )");
            sqlite3_bind_text(stmt, 1, pixivInfo.tagsTransl[i].c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 2, pixivTagToId[pixivInfo.tags[i]]);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                Error() << "Failed to update translated_tag: " << sqlite3_errmsg(db);
                sqlite3_finalize(stmt);
                return false;
            }
            sqlite3_finalize(stmt);
        }
    }
    return true;
}
PicInfo PicDatabase::getPicInfo(uint64_t id, int64_t tweetID, int64_t pixivID) const {
    PicInfo info{};
    sqlite3_stmt* stmt = nullptr;

    // 查询主表
    stmt = prepare(
        "SELECT width, height, size, file_type, x_restrict, ai_type, edit_time, download_time, phash FROM pictures WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        info.id = id;
        info.width = sqlite3_column_int(stmt, 0);
        info.height = sqlite3_column_int(stmt, 1);
        info.size = sqlite3_column_int(stmt, 2);
        info.fileType = static_cast<ImageFormat>(sqlite3_column_int(stmt, 3));
        info.xRestrict = static_cast<RestrictType>(sqlite3_column_int(stmt, 4));
        info.aiType = static_cast<AIType>(sqlite3_column_int(stmt, 5));
        info.editTime = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        info.downloadTime = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        const char* phashText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        info.phash = std::string(phashText ? phashText : "");
    } else {
        sqlite3_finalize(stmt);
        return info; // id 不存在，返回空对象
    }
    sqlite3_finalize(stmt);

    // 查询文件路径
    stmt = prepare("SELECT file_path FROM picture_file_paths WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        info.filePaths.insert(std::filesystem::path(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
    }
    sqlite3_finalize(stmt);

    // 查询标签
    stmt = prepare(R"(
        SELECT t.tag, t.is_character, pt.probability
        FROM picture_tags pt
        JOIN tags t ON pt.tag_id = t.id
        WHERE pt.id = ?
    )");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PicTag picTag;
        picTag.tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        picTag.isCharacter = sqlite3_column_int(stmt, 1) != 0;
        picTag.probability = static_cast<float>(sqlite3_column_double(stmt, 2));
        info.tags.push_back(picTag);
    }
    sqlite3_finalize(stmt);

    // 查询 Pixiv 关联
    stmt = prepare("SELECT pixiv_id, pixiv_index FROM picture_pixiv_ids WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        info.pixivIdIndices[sqlite3_column_int64(stmt, 0)] = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    // 查询 Tweet 关联
    stmt = prepare("SELECT tweet_id, tweet_index FROM picture_tweet_ids WHERE id = ?");
    sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        info.tweetIdIndices[sqlite3_column_int64(stmt, 0)] = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    // 查询 TweetInfo
    if (tweetID != 0) {
        TweetInfo tweetInfo = getTweetInfo(tweetID);
        if (tweetInfo.tweetID != 0) { // 确保 TweetInfo 有效
            info.tweetInfo.push_back(tweetInfo);
        }
    } else {
        for (const auto& pair : info.tweetIdIndices) {
            int64_t tweetID = pair.first;
            TweetInfo tweetInfo = getTweetInfo(tweetID);
            if (tweetInfo.tweetID != 0) { // 确保 TweetInfo 有效
                info.tweetInfo.push_back(tweetInfo);
            }
        }
    }

    // 查询 PixivInfo
    if (pixivID != 0) {
        PixivInfo pixivInfo = getPixivInfo(pixivID);
        if (pixivInfo.pixivID != 0) { // 确保 PixivInfo 有效
            info.pixivInfo.push_back(pixivInfo);
        }
    } else {
        for (const auto& pair : info.pixivIdIndices) {
            int64_t pixivID = pair.first;
            PixivInfo pixivInfo = getPixivInfo(pixivID);
            if (pixivInfo.pixivID != 0) { // 确保 PixivInfo 有效
                info.pixivInfo.push_back(pixivInfo);
            }
        }
    }

    return info;
}
// 查询没有任何Pixiv 关联和 Tweet 关联的图片和有关联但对应的 Pixiv 或 Tweet 信息缺失的图片
std::vector<PicInfo> PicDatabase::getNoMetadataPics() const {
    std::vector<PicInfo> pics;
    sqlite3_stmt* stmt = nullptr;

    stmt = prepare(R"(
        SELECT p.id
        FROM pictures p
        LEFT JOIN picture_pixiv_ids ppi ON p.id = ppi.id
        LEFT JOIN picture_tweet_ids pti ON p.id = pti.id
        LEFT JOIN pixiv_artworks pa ON ppi.pixiv_id = pa.pixiv_id
        LEFT JOIN tweets t ON pti.tweet_id = t.tweet_id
        WHERE 
            (ppi.id IS NULL AND pti.id IS NULL)
            OR (ppi.id IS NOT NULL AND pa.pixiv_id IS NULL)
            OR (pti.id IS NOT NULL AND t.tweet_id IS NULL)
    )");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        uint64_t picID = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        PicInfo picInfo = getPicInfo(picID);
        pics.push_back(picInfo);
    }
    sqlite3_finalize(stmt);
    return pics;
}
TweetInfo PicDatabase::getTweetInfo(int64_t tweetID) const {
    TweetInfo info{};
    sqlite3_stmt* stmt = nullptr;

    // 查询主表
    stmt = prepare(R"(
        SELECT date, author_id, author_name, author_nick, author_description, author_profile_image, author_profile_banner, 
        favorite_count, quote_count, reply_count, retweet_count, bookmark_count, view_count, description 
        FROM tweets WHERE tweet_id = ?)");
    sqlite3_bind_int64(stmt, 1, tweetID);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        info.tweetID = tweetID;
        info.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        info.authorID = sqlite3_column_int64(stmt, 1);
        info.authorName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.authorNick = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.authorDescription = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.authorProfileImage = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.authorProfileBanner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        info.favoriteCount = sqlite3_column_int(stmt, 7);
        info.quoteCount = sqlite3_column_int(stmt, 8);
        info.replyCount = sqlite3_column_int(stmt, 9);
        info.retweetCount = sqlite3_column_int(stmt, 10);
        info.bookmarkCount = sqlite3_column_int(stmt, 11);
        info.viewCount = sqlite3_column_int(stmt, 12);
        info.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
    } else {
        sqlite3_finalize(stmt);
        return info;
    }
    sqlite3_finalize(stmt);

    // 查询 hashtags
    stmt = prepare(R"(
        SELECT t.hashtag
        FROM tweet_hashtags th
        JOIN twitter_hashtags t ON th.hashtag_id = t.id
        WHERE tweet_id = ?)");
    sqlite3_bind_int64(stmt, 1, tweetID);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        info.hashtags.insert(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);

    return info;
}
PixivInfo PicDatabase::getPixivInfo(int64_t pixivID) const {
    PixivInfo info{};
    sqlite3_stmt* stmt = nullptr;

    // 查询主表
    stmt = prepare(R"(
        SELECT date, author_name, author_id, title, description, like_count, view_count, x_restrict, ai_type FROM
        pixiv_artworks WHERE pixiv_id = ?)");

    sqlite3_bind_int64(stmt, 1, pixivID);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        info.pixivID = pixivID;
        info.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        info.authorName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.authorID = sqlite3_column_int64(stmt, 2);
        info.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        info.likeCount = sqlite3_column_int(stmt, 5);
        info.viewCount = sqlite3_column_int(stmt, 6);
        info.xRestrict = static_cast<RestrictType>(sqlite3_column_int(stmt, 7));
        info.aiType = static_cast<AIType>(sqlite3_column_int(stmt, 8));
    } else {
        sqlite3_finalize(stmt);
        return info;
    }
    sqlite3_finalize(stmt);

    // 查询 tags
    stmt = prepare(R"(
        SELECT t.tag
        FROM pixiv_artworks_tags pt
        JOIN pixiv_tags t ON pt.tag_id = t.id
        WHERE pixiv_id = ?
        )");

    sqlite3_bind_int64(stmt, 1, pixivID);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        info.tags.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return info;
}
void PicDatabase::processAndImportSingleFile(const std::filesystem::path& filePath, ParserType parserType) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif" || ext == ".webp") {
            PicInfo picInfo = parsePicture(filePath, parserType);
            insertPicInfo(picInfo);
        } else if (ext == ".txt" && parserType == ParserType::Pixiv) {
            PixivInfo pixivInfo = parsePixivMetadata(filePath);
            insertPixivInfo(pixivInfo);
        } else if (ext == ".json") {
            if (parserType == ParserType::Pixiv) {
                std::vector<PixivInfo> pixivInfoVec = parsePixivJson(filePath);
                for (const auto& pixivInfo : pixivInfoVec) {
                    updatePixivInfo(pixivInfo);
                }
            } else if (parserType == ParserType::Twitter) {
                TweetInfo tweetInfo = parseTweetJson(filePath);
                insertTweetInfo(tweetInfo);
            }
        } else if (ext == ".csv" && parserType == ParserType::Pixiv) {
            std::vector<PixivInfo> pixivInfoVec = parsePixivCsv(filePath);
            for (const auto& pixivInfo : pixivInfoVec) {
                updatePixivInfo(pixivInfo);
            }
        }
    } catch (const std::exception& e) {
        Warn() << "Error processing file:" << filePath.c_str() << "Error:" << e.what();
    }
}
void PicDatabase::importFilesFromDirectory(const std::filesystem::path& directory,
                                           ParserType parserType,
                                           ImportProgressCallback progressCallback) {
    auto files = collectFiles(directory);
    disableForeignKeyRestriction();

    const size_t BATCH_SIZE = 1000;
    size_t processed = 0;
    for (size_t i = 0; i < files.size(); i += BATCH_SIZE) {
        beginTransaction();
        auto batch_end = std::min(i + BATCH_SIZE, files.size());
        auto start_time = std::chrono::high_resolution_clock::now();
        for (size_t j = i; j < batch_end; j++) {
            processAndImportSingleFile(files[j], parserType);
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
    // Info() << "Import completed. Total files processed:" << processed;
}
void PicDatabase::syncTables(std::unordered_set<int64_t> newPixivIDs) { // post-import operations
    sqlite3_stmt* stmt = nullptr;
    // count tags
    if (!execute(R"(
        UPDATE tags SET count = (
            SELECT COUNT(*) FROM picture_tags WHERE tag_id = tags.id
        )
    )")) {
        Warn() << "Failed to count tags:" << sqlite3_errmsg(db);
    }
    if (!execute(R"(
        UPDATE twitter_hashtags SET count = (
            SELECT COUNT(*) FROM tweet_hashtags WHERE hashtag_id = twitter_hashtags.id
        )
    )")) {
        Warn() << "Failed to count twitter hashtags:" << sqlite3_errmsg(db);
    }
    if (!execute(R"(
        UPDATE pixiv_tags SET count = (
            SELECT COUNT(*) FROM pixiv_artworks_tags WHERE tag_id = pixiv_tags.id
        )
    )")) {
        Warn() << "Failed to count pixiv tags:" << sqlite3_errmsg(db);
    }
    sqlite3_finalize(stmt);
    // sync x_restrict and ai_type from pixiv_artworks to pictures
    if (newPixivIDs.empty()) {
        newPixivIDs = this->newPixivIDs;
    }
    for (const auto& pixivID : newPixivIDs) {
        stmt = prepare(R"(
            UPDATE pictures SET x_restrict = (
                SELECT x_restrict FROM pixiv_artworks WHERE pixiv_id = ?
            ), ai_type = (
                SELECT ai_type FROM pixiv_artworks WHERE pixiv_id = ?
            ) WHERE id IN (
                SELECT id FROM picture_pixiv_ids WHERE pixiv_id = ?
            )
        )");
        sqlite3_bind_int64(stmt, 1, pixivID);
        sqlite3_bind_int64(stmt, 2, pixivID);
        sqlite3_bind_int64(stmt, 3, pixivID);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Warn() << "Failed to sync pixiv info for pixiv_id:" << pixivID << "Error:" << sqlite3_errmsg(db);
        }
    }
    execute("PRAGMA foreign_keys = ON");
    sqlite3_finalize(stmt);
}
void PicDatabase::importTagSet(const std::vector<std::pair<std::string, bool>>& tagSet) {
    sqlite3_stmt* stmt = nullptr;
    for (int tagId = 0; tagId < tagSet.size(); tagId++) {
        stmt = prepare(R"(
            INSERT INTO tags(id, tag, is_character) VALUES (?, ?, ?)
        )");
        const auto& [tag, isCharacter] = tagSet[tagId];
        sqlite3_bind_int(stmt, 1, tagId);
        sqlite3_bind_text(stmt, 2, tag.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, isCharacter ? 1 : 0);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Error() << "Failed to insert tag: " << sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            continue;
        }
        sqlite3_finalize(stmt);
    }
}
std::vector<uint64_t> PicDatabase::tagSearch(const std::unordered_set<std::string>& includedTags,
                                             const std::unordered_set<std::string>& excludedTags) {
    std::vector<uint64_t> results;
    if (includedTags.empty() && excludedTags.empty()) return results;

    std::string includedTagIdStr;
    std::string excludedTagIdStr;
    if (!includedTags.empty()) {
        int idx = 0;
        for (const auto& tag : includedTags) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += std::to_string(tagToId.at(tag));
        }
        if (includedTagIdStr == "") return results; // No included tags found in database
    }
    if (!excludedTags.empty()) {
        int idx = 0;
        for (const auto& tag : excludedTags) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += std::to_string(tagToId.at(tag));
        }
    }
    std::string sql = "SELECT id FROM picture_tags WHERE 1=1 AND tag_id IN (" + includedTagIdStr +
                      ") AND id NOT IN (SELECT id FROM picture_tags WHERE tag_id IN (" + excludedTagIdStr +
                      ")) GROUP BY id HAVING COUNT(DISTINCT tag_id) = " + std::to_string(includedTags.size());

    sqlite3_stmt* stmt = nullptr;
    stmt = prepare(sql.c_str());
    if (!stmt) {
        Warn() << "Failed to prepare statement:" << sqlite3_errmsg(db);
        return results;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(int64_to_uint64(sqlite3_column_int64(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return results;
}
std::vector<uint64_t> PicDatabase::pixivTagSearch(const std::unordered_set<std::string>& includedTags,
                                                  const std::unordered_set<std::string>& excludedTags) {
    std::vector<uint64_t> results;
    if (includedTags.empty() && excludedTags.empty()) return results;

    std::string includedTagIdStr;
    std::string excludedTagIdStr;
    if (!includedTags.empty()) {
        int idx = 0;
        for (const auto& tag : includedTags) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += std::to_string(pixivTagToId.at(tag));
        }
        if (includedTagIdStr == "") return results; // No included tags found in database
    }
    if (!excludedTags.empty()) {
        int idx = 0;
        for (const auto& tag : excludedTags) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += std::to_string(pixivTagToId.at(tag));
        }
    }
    std::string sql = "SELECT pixiv_id FROM pixiv_artworks_tags WHERE 1=1 AND tag_id IN (" + includedTagIdStr +
                      ") AND pixiv_id NOT IN (SELECT pixiv_id FROM pixiv_artworks_tags WHERE tag_id IN (" + excludedTagIdStr +
                      ")) GROUP BY pixiv_id HAVING COUNT(DISTINCT tag_id) = " + std::to_string(includedTags.size());

    sqlite3_stmt* stmt = prepare(sql.c_str());
    if (!stmt) {
        Warn() << "Pixiv tag search failed:" << sqlite3_errmsg(db);
        return results;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t pixivID = sqlite3_column_int64(stmt, 0);
        sqlite3_stmt* picQuery = prepare(R"(
            SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
        )");
        sqlite3_bind_int64(picQuery, 1, pixivID);
        if (sqlite3_step(picQuery) == SQLITE_ROW) {
            results.push_back(int64_to_uint64(sqlite3_column_int64(picQuery, 0)));
        }
        sqlite3_finalize(picQuery);
    }
    sqlite3_finalize(stmt);
    return results;
}
std::vector<uint64_t> PicDatabase::tweetHashtagSearch(const std::unordered_set<std::string>& includedTags,
                                                      const std::unordered_set<std::string>& excludedTags) {
    std::vector<uint64_t> results;
    if (includedTags.empty() && excludedTags.empty()) return results;

    std::string includedTagIdStr;
    std::string excludedTagIdStr;
    if (!includedTags.empty()) {
        int idx = 0;
        for (const auto& tag : includedTags) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += std::to_string(twitterHashtagToId.at(tag));
        }
        if (includedTagIdStr == "") return results; // No included tags found in database
    }
    if (!excludedTags.empty()) {
        int idx = 0;
        for (const auto& tag : excludedTags) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += std::to_string(twitterHashtagToId.at(tag));
        }
    }
    std::string sql = "SELECT tweet_id FROM tweet_hashtags WHERE 1=1 AND hashtag_id IN (" + includedTagIdStr +
                      ") AND tweet_id NOT IN (SELECT tweet_id FROM tweet_hashtags WHERE hashtag_id IN (" + excludedTagIdStr +
                      ")) GROUP BY tweet_id HAVING COUNT(DISTINCT hashtag_id) = " + std::to_string(includedTags.size());

    sqlite3_stmt* stmt = prepare(sql.c_str());
    if (!stmt) {
        Warn() << "Tweet hashtag search failed:" << sqlite3_errmsg(db);
        return results;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t tweetID = sqlite3_column_int64(stmt, 0);
        sqlite3_stmt* picQuery = prepare(R"(
            SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
        )");
        sqlite3_bind_int64(picQuery, 1, tweetID);
        if (sqlite3_step(picQuery) == SQLITE_ROW) {
            results.push_back(int64_to_uint64(sqlite3_column_int64(picQuery, 0)));
        }
        sqlite3_finalize(picQuery);
    }
    sqlite3_finalize(stmt);
    return results;
}
std::unordered_map<uint64_t, int64_t> PicDatabase::textSearch(const std::string& searchText, SearchField searchField) {
    std::unordered_map<uint64_t, int64_t> results; // id -> tweet_id or pixiv_id
    if (searchText.empty() || searchField == SearchField::None) return results;
    sqlite3_stmt* stmt = nullptr;
    bool isNumeric =
        !searchText.empty() && std::all_of(searchText.begin(), searchText.end(), [](char c) { return std::isdigit(c); });
    switch (searchField) {
    case SearchField::PicID: {
        if (!isNumeric) break;
        uint64_t id = std::stoull(searchText);
        stmt = prepare("SELECT id FROM pictures WHERE id = ?");
        sqlite3_bind_int64(stmt, 1, uint64_to_int64(id));
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            results[id] = 0;
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::PixivID: {
        if (!isNumeric) break;
        int64_t pixivID = std::stoll(searchText);
        stmt = prepare(R"(
                SELECT id FROM picture_pixiv_ids WHERE pixiv_id = ?
            )");
        sqlite3_bind_int64(stmt, 1, pixivID);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t id = int64_to_uint64(sqlite3_column_int64(stmt, 0));
            results[id] = pixivID;
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::PixivAuthorID: {
        if (!isNumeric) break;
        int64_t authorID = std::stoll(searchText);
        stmt = prepare(R"(
                SELECT pixiv_id FROM pixiv_artworks WHERE author_id = ?
            )");
        sqlite3_bind_int64(stmt, 1, authorID);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t pixivID = sqlite3_column_int64(stmt, 0);
            sqlite3_stmt* picQuery = prepare(R"(
                    SELECT id FROM picture_pixiv_ids WHERE pixiv_id = ?
                )");
            sqlite3_bind_int64(picQuery, 1, pixivID);
            if (sqlite3_step(picQuery) == SQLITE_ROW) {
                uint64_t id = int64_to_uint64(sqlite3_column_int64(picQuery, 0));
                results[id] = pixivID;
            }
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::PixivAuthorName: {
        stmt = prepare(R"(
                SELECT pixiv_id FROM pixiv_artworks WHERE author_name LIKE ?
            )");
        sqlite3_bind_text(stmt, 1, (searchText + "%").c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t pixivID = sqlite3_column_int64(stmt, 0);
            sqlite3_stmt* picQuery = prepare(R"(
                        SELECT id FROM picture_pixiv_ids WHERE pixiv_id = ?
            )");
            sqlite3_bind_int64(picQuery, 1, pixivID);
            if (sqlite3_step(picQuery) == SQLITE_ROW) {
                uint64_t id = int64_to_uint64(sqlite3_column_int64(picQuery, 0));
                results[id] = pixivID;
            }
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::PixivTitle: {
        stmt = prepare(R"(
                SELECT pixiv_id FROM pixiv_artworks WHERE title LIKE ?
            )");
        sqlite3_bind_text(stmt, 1, (searchText + "%").c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t pixivID = sqlite3_column_int64(stmt, 0);
            sqlite3_stmt* picQuery = prepare(R"(
                        SELECT id FROM picture_pixiv_ids WHERE pixiv_id = ?
                    )");
            sqlite3_bind_int64(picQuery, 1, pixivID);
            if (sqlite3_step(picQuery) == SQLITE_ROW) {
                uint64_t id = int64_to_uint64(sqlite3_column_int64(picQuery, 0));
                results[id] = pixivID;
            }
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::TweetID: {
        if (!isNumeric) break;
        int64_t tweetID = std::stoll(searchText);
        stmt = prepare(R"(
                SELECT id FROM picture_tweet_ids WHERE tweet_id = ?
            )");
        sqlite3_bind_int64(stmt, 1, tweetID);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t id = int64_to_uint64(sqlite3_column_int64(stmt, 0));
            results[id] = tweetID;
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::TweetAuthorID: {
        if (!isNumeric) break;
        int64_t authorID = std::stoll(searchText);
        stmt = prepare(R"(
                SELECT tweet_id FROM tweets WHERE author_id = ?
            )");
        sqlite3_bind_int64(stmt, 1, authorID);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t tweetID = sqlite3_column_int64(stmt, 0);
            sqlite3_stmt* picQuery = prepare(R"(
                        SELECT id FROM picture_tweet_ids WHERE tweet_id = ?
                    )");
            sqlite3_bind_int64(picQuery, 1, tweetID);
            if (sqlite3_step(picQuery) == SQLITE_ROW) {
                uint64_t id = int64_to_uint64(sqlite3_column_int64(picQuery, 0));
                results[id] = tweetID;
            }
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::TweetAuthorName: {
        stmt = prepare(R"(
                SELECT tweet_id FROM tweets WHERE author_name LIKE ?
            )");
        sqlite3_bind_text(stmt, 1, (searchText + "%").c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t tweetID = sqlite3_column_int64(stmt, 0);
            sqlite3_stmt* picQuery = prepare(R"(
                    SELECT id FROM picture_tweet_ids WHERE tweet_id = ?
                )");
            sqlite3_bind_int64(picQuery, 1, tweetID);
            if (sqlite3_step(picQuery) == SQLITE_ROW) {
                uint64_t id = int64_to_uint64(sqlite3_column_int64(picQuery, 0));
                results[id] = tweetID;
            }
        }
        sqlite3_finalize(stmt);
    } break;
    case SearchField::TweetAuthorNick: {
        stmt = prepare(R"(
                SELECT tweet_id FROM tweets WHERE author_nick LIKE ?
            )");
        sqlite3_bind_text(stmt, 1, (searchText + "%").c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t tweetID = sqlite3_column_int64(stmt, 0);
            sqlite3_stmt* picQuery = prepare(R"(
                    SELECT id FROM picture_tweet_ids WHERE tweet_id = ?
                )");
            sqlite3_bind_int64(picQuery, 1, tweetID);
            if (sqlite3_step(picQuery) == SQLITE_ROW) {
                uint64_t id = int64_to_uint64(sqlite3_column_int64(picQuery, 0));
                results[id] = tweetID;
            }
        }
        sqlite3_finalize(stmt);
    } break;
    }
    return results;
}
std::vector<std::tuple<std::string, int, bool>> PicDatabase::getTags() const {
    std::vector<std::tuple<std::string, int, bool>> tagCounts;
    sqlite3_stmt* stmt = prepare("SELECT tag, count, is_character FROM tags");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int count = sqlite3_column_int(stmt, 1);
        bool isCharacter = sqlite3_column_int(stmt, 2) != 0;
        tagCounts.push_back({tag, count, isCharacter});
    }
    sqlite3_finalize(stmt);
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return std::get<1>(b) < std::get<1>(a); // 按照 count 降序排序
    });
    return tagCounts;
}
std::vector<std::pair<std::string, int>> PicDatabase::getPixivTags() const {
    std::vector<std::pair<std::string, int>> tagCounts;
    sqlite3_stmt* stmt = prepare("SELECT tag, count FROM pixiv_tags");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int count = sqlite3_column_int(stmt, 1);
        tagCounts.push_back({tag, count});
    }
    sqlite3_finalize(stmt);
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // 按照 count 降序排序
    });
    return tagCounts;
}
std::vector<std::pair<std::string, int>> PicDatabase::getTwitterHashtags() const {
    std::vector<std::pair<std::string, int>> tagCounts;
    sqlite3_stmt* stmt = prepare("SELECT hashtag, count FROM twitter_hashtags");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int count = sqlite3_column_int(stmt, 1);
        tagCounts.push_back({tag, count});
    }
    sqlite3_finalize(stmt);
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // 按照 count 降序排序
    });
    return tagCounts;
}
void PicDatabase::enableForeignKeyRestriction() const {
    if (!execute("PRAGMA foreign_keys = ON;")) {
        Warn() << "Failed to enable foreign key restriction:" << sqlite3_errmsg(db);
    }
}
void PicDatabase::disableForeignKeyRestriction() const {
    if (!execute("PRAGMA foreign_keys = OFF;")) {
        Warn() << "Failed to disable foreign key restriction:" << sqlite3_errmsg(db);
    }
}
bool PicDatabase::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}
bool PicDatabase::commitTransaction() {
    return execute("COMMIT;");
}
bool PicDatabase::rollbackTransaction() {
    return execute("ROLLBACK;");
}

MultiThreadedImporter::MultiThreadedImporter(const std::filesystem::path& directory,
                                             ImportProgressCallback progressCallback,
                                             ParserType parserType,
                                             std::string dbFile,
                                             size_t threadCount)
    : parserType(parserType), progressCallback(progressCallback), dbFile(dbFile) {
    files = collectFiles(directory);
    supportedFileCount = files.size();
    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back(&MultiThreadedImporter::workerThreadFunc, this);
    }
    insertThread = std::thread(&MultiThreadedImporter::insertThreadFunc, this);
}
MultiThreadedImporter::~MultiThreadedImporter() {
    forceStop();
    finish();
}
void MultiThreadedImporter::forceStop() {
    stopFlag.store(true);
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
    finished = true;
    stopFlag.store(true);
    cv.notify_all();
    if (insertThread.joinable()) {
        insertThread.join();
    }
    return true;
}
bool processSingleFile(const std::filesystem::path& filePath,
                       ParserType parserType,
                       std::vector<PicInfo>& picInfos,
                       std::vector<PixivInfo>& pixivInfos,
                       std::vector<TweetInfo>& tweetInfos,
                       std::vector<std::vector<PixivInfo>>& pixivInfoVecs) {
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif" || ext == ".webp") {
            picInfos.push_back(parsePicture(filePath, parserType));
            return true;
        } else if (ext == ".txt" && parserType == ParserType::Pixiv) {
            pixivInfos.push_back(parsePixivMetadata(filePath));
            return true;
        } else if (ext == ".json") {
            if (parserType == ParserType::Pixiv) {
                pixivInfoVecs.push_back(parsePixivJson(filePath));
            } else if (parserType == ParserType::Twitter) {
                tweetInfos.push_back(parseTweetJson(filePath));
            }
            return true;
        } else if (ext == ".csv" && parserType == ParserType::Pixiv) {
            pixivInfoVecs.push_back(parsePixivCsv(filePath));
            return true;
        }
    } catch (const std::exception& e) {
        Warn() << "Error processing file:" << filePath.c_str() << "Error:" << e.what();
        return false;
    }
    return false;
}
void MultiThreadedImporter::workerThreadFunc() {
    std::vector<PicInfo> picInfos;
    picInfos.reserve(500);
    std::vector<PixivInfo> pixivInfos;
    pixivInfos.reserve(500);
    std::vector<TweetInfo> tweetInfos;
    tweetInfos.reserve(500);
    std::vector<std::vector<PixivInfo>> pixivInfoVecs;
    pixivInfoVecs.reserve(500);
    size_t index = 0;
    while ((index = nextFileIndex.fetch_add(1, std::memory_order_relaxed)) < files.size()) {
        if (picInfos.size() >= 100) {
            if (picMutex.try_lock()) {
                while (!picInfos.empty()) {
                    picQueue.push(std::move(picInfos.back()));
                    picInfos.pop_back();
                }
                picMutex.unlock();
                picInfos.clear();
                cv.notify_one();
            }
        }
        if (pixivInfos.size() >= 100) {
            if (pixivMutex.try_lock()) {
                while (!pixivInfos.empty()) {
                    pixivQueue.push(std::move(pixivInfos.back()));
                    pixivInfos.pop_back();
                }
                pixivMutex.unlock();
                pixivInfos.clear();
                cv.notify_one();
            }
        }
        if (tweetInfos.size() >= 100) {
            if (tweetMutex.try_lock()) {
                while (!tweetInfos.empty()) {
                    tweetQueue.push(std::move(tweetInfos.back()));
                    tweetInfos.pop_back();
                }
                tweetMutex.unlock();
                tweetInfos.clear();
                cv.notify_one();
            }
        }
        if (pixivInfoVecs.size() >= 100) {
            if (pixivVecMutex.try_lock()) {
                while (!pixivInfoVecs.empty()) {
                    pixivVecQueue.push(std::move(pixivInfoVecs.back()));
                    pixivInfoVecs.pop_back();
                }
                pixivVecMutex.unlock();
                pixivInfoVecs.clear();
                cv.notify_one();
            }
        }
        if (stopFlag.load()) break;
        const auto& filePath = files[index];
        if (!processSingleFile(filePath, parserType, picInfos, pixivInfos, tweetInfos, pixivInfoVecs))
            supportedFileCount.fetch_sub(1, std::memory_order_relaxed);
    }
    if (!picInfos.empty()) {
        std::lock_guard<std::mutex> lock(picMutex);
        while (!picInfos.empty()) {
            picQueue.push(std::move(picInfos.back()));
            picInfos.pop_back();
        }
        picInfos.clear();
        cv.notify_one();
    }
    if (!pixivInfos.empty()) {
        std::lock_guard<std::mutex> lock(pixivMutex);
        while (!pixivInfos.empty()) {
            pixivQueue.push(std::move(pixivInfos.back()));
            pixivInfos.pop_back();
        }
        pixivInfos.clear();
        cv.notify_one();
    }
    if (!tweetInfos.empty()) {
        std::lock_guard<std::mutex> lock(tweetMutex);
        while (!tweetInfos.empty()) {
            tweetQueue.push(std::move(tweetInfos.back()));
            tweetInfos.pop_back();
        }
        tweetInfos.clear();
        cv.notify_one();
    }
    if (!pixivInfoVecs.empty()) {
        std::lock_guard<std::mutex> lock(pixivVecMutex);
        while (!pixivInfoVecs.empty()) {
            pixivVecQueue.push(std::move(pixivInfoVecs.back()));
            pixivInfoVecs.pop_back();
        }
        pixivInfoVecs.clear();
        cv.notify_one();
    }
}
void MultiThreadedImporter::insertThreadFunc() {
    PicDatabase threadDb(dbFile, DbMode::Import);
    std::mutex conditionMutex;
    std::vector<PicInfo> picsToInsert;
    picsToInsert.reserve(1000);
    std::vector<PixivInfo> pixivsToInsert;
    pixivsToInsert.reserve(1000);
    std::vector<TweetInfo> tweetsToInsert;
    tweetsToInsert.reserve(1000);
    std::vector<std::vector<PixivInfo>> pixivVecsToInsert;
    pixivVecsToInsert.reserve(1000);
    while (!stopFlag.load() || importedCount < supportedFileCount.load(std::memory_order_relaxed)) {
        if (progressCallback) progressCallback(importedCount, supportedFileCount.load(std::memory_order_relaxed));
        std::unique_lock<std::mutex> lock(conditionMutex);
        cv.wait(lock, [this]() {
            return !picQueue.empty() || !pixivQueue.empty() || !tweetQueue.empty() || !pixivVecQueue.empty() || stopFlag.load();
        });
        if (stopFlag.load()) break;
        if (!picQueue.empty()) {
            if (picMutex.try_lock()) {
                while (!picQueue.empty()) {
                    picsToInsert.push_back(std::move(picQueue.front()));
                    picQueue.pop();
                }
                picMutex.unlock();
                threadDb.beginTransaction();
                for (const auto& picInfo : picsToInsert) {
                    threadDb.insertPicInfo(picInfo);
                    importedCount++;
                }
                picsToInsert.clear();
                if (!threadDb.commitTransaction()) {
                    Warn() << "Batch commit failed in insert thread, rolling back";
                    threadDb.rollbackTransaction();
                }
            }
        }
        if (!pixivQueue.empty()) {
            if (pixivMutex.try_lock()) {
                while (!pixivQueue.empty()) {
                    pixivsToInsert.push_back(std::move(pixivQueue.front()));
                    pixivQueue.pop();
                }
                pixivMutex.unlock();
                threadDb.beginTransaction();
                for (const auto& pixivInfo : pixivsToInsert) {
                    threadDb.insertPixivInfo(pixivInfo);
                    importedCount++;
                }
                pixivsToInsert.clear();
                if (!threadDb.commitTransaction()) {
                    Warn() << "Batch commit failed in insert thread, rolling back";
                    threadDb.rollbackTransaction();
                }
            }
        }
        if (!tweetQueue.empty()) {
            if (tweetMutex.try_lock()) {
                while (!tweetQueue.empty()) {
                    tweetsToInsert.push_back(std::move(tweetQueue.front()));
                    tweetQueue.pop();
                }
                tweetMutex.unlock();
                threadDb.beginTransaction();
                for (const auto& tweetInfo : tweetsToInsert) {
                    threadDb.insertTweetInfo(tweetInfo);
                    importedCount++;
                }
                tweetsToInsert.clear();
                if (!threadDb.commitTransaction()) {
                    Warn() << "Batch commit failed in insert thread, rolling back";
                    threadDb.rollbackTransaction();
                }
            }
        }
        if (!pixivVecQueue.empty()) {
            if (pixivVecMutex.try_lock()) {
                while (!pixivVecQueue.empty()) {
                    pixivVecsToInsert.push_back(std::move(pixivVecQueue.front()));
                    pixivVecQueue.pop();
                }
                pixivVecMutex.unlock();
                threadDb.beginTransaction();
                for (const auto& pixivInfoVec : pixivVecsToInsert) {
                    for (const auto& pixivInfo : pixivInfoVec) {
                        threadDb.updatePixivInfo(pixivInfo);
                        importedCount++;
                    }
                }
                pixivVecsToInsert.clear();
                if (!threadDb.commitTransaction()) {
                    Warn() << "Batch commit failed in insert thread, rolling back";
                    threadDb.rollbackTransaction();
                }
            }
        }
    }
    threadDb.syncTables();
    if (progressCallback) progressCallback(importedCount, supportedFileCount.load(std::memory_order_relaxed));
}
