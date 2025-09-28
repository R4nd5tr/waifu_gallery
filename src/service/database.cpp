#include "database.h"
#include "parser.h"
#include "model.h"
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QString>
#include <QFile>

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
std::vector<std::filesystem::path> collectAllFiles(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    int fileCount = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        files.push_back(entry.path());
        fileCount++;
    }
    qInfo() << "Total files collected:" << fileCount;
    return files;
}
PicDatabase::PicDatabase(const QString& connectionName, const QString& databaseFile) {
    if (connectionName.isEmpty()) {
        this->connectionName = QString("db_conn_%1").arg(reinterpret_cast<quintptr>(this));
    } else {
        this->connectionName = connectionName;
    }
    initDatabase(databaseFile);
    getTagMapping();
}
PicDatabase::~PicDatabase() {
    database.close();
}
void PicDatabase::initDatabase(QString databaseFile){
    bool databaseExists = QFile::exists(databaseFile);
    database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    database.setDatabaseName(databaseFile);
    if (!database.open()) {
        qCritical() << "Error opening database:" << database.lastError().text();
        return;
    }
    if (!databaseExists) {
        if (!createTables()) {
            qCritical() << "Failed to create tables";
            database.close();
        }
    }
    QSqlQuery query(database);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "Failed to enable foreign keys:" << query.lastError().text();
    }
    qInfo() << "database initialized";
}
bool PicDatabase::createTables() {
    QSqlQuery query(database);
    const QString picturesTable = R"(
        CREATE TABLE IF NOT EXISTS pictures (
            id INTEGER PRIMARY KEY NOT NULL,
            width INTEGER,
            height INTEGER,
            size INTEGER,
            x_restrict INTEGER DEFAULT NULL,
            ai_type INTEGER DEFAULT NULL
        )
    )";//TODO: add feature vector from deep learning models
    const QString pictureTagsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tags (
            id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            PRIMARY KEY (id, tag_id),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
        )
    )";
    const QString pictureFilesTable = R"(
        CREATE TABLE IF NOT EXISTS picture_file_paths (
            id INTEGER NOT NULL,
            file_path TEXT NOT NULL,
            PRIMARY KEY (id, file_path),
            UNIQUE (file_path),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const QString picturePixivIdsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_pixiv_ids (
            id INTEGER NOT NULL,
            pixiv_id INTEGER NOT NULL,
            pixiv_index INTEGER NOT NULL,
            PRIMARY KEY (pixiv_id, pixiv_index),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const QString pictureTweetIdsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tweet_ids (
            id INTEGER NOT NULL,
            tweet_id INTEGER NOT NULL,
            tweet_index INTEGER NOT NULL,
            PRIMARY KEY (tweet_id, tweet_index),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";
    const QString tagsTable = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tag TEXT NOT NULL UNIQUE,
            is_character BOOLEAN,
            count INTEGER DEFAULT 0
        )
    )";
    const QString tweetsTable = R"(
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
    const QString tweetHashtagsTable = R"(
        CREATE TABLE IF NOT EXISTS tweet_hashtags (
            tweet_id INTEGER NOT NULL,
            hashtag_id INTEGER NOT NULL,
            PRIMARY KEY (tweet_id, hashtag_id),

            FOREIGN KEY (tweet_id) REFERENCES tweets(tweet_id) ON DELETE CASCADE,
            FOREIGN KEY (hashtag_id) REFERENCES twitter_hashtags(id) ON DELETE CASCADE
        )
    )";
    const QString twitterHashtagsTable = R"(
        CREATE TABLE IF NOT EXISTS twitter_hashtags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hashtag TEXT NOT NULL UNIQUE,
            count INTEGER DEFAULT 0
        )
    )";
    const QString pixivArtworksTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_artworks (
            pixiv_id INTEGER PRIMARY KEY NOT NULL,
            date DATE NOT NULL,
            author_name TEXT NOT NULL,
            author_id INTEGER NOT NULL,
            title TEXT,
            description TEXT,
            like_count INTEGER DEFAULT 0,
            view_count INTEGER DEFAULT 0,
            x_restrict INTEGER,
            ai_type INTEGER DEFAULT 0 
        )
    )";
    const QString pixivArtworksTagsTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_artworks_tags (
            pixiv_id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            PRIMARY KEY (pixiv_id, tag_id),

            FOREIGN KEY (pixiv_id) REFERENCES pixiv_artworks(pixiv_id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES pixiv_tags(id) ON DELETE CASCADE
        )
    )";
    const QString pixivTagsTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tag TEXT NOT NULL UNIQUE,
            translated_tag TEXT,
            count INTEGER DEFAULT 0
        )
    )";
    const QStringList tables = {
        //pixiv
        pixivArtworksTable, 
        pixivArtworksTagsTable,
        pixivTagsTable,
        //tweet
        tweetsTable,
        tweetHashtagsTable,
        twitterHashtagsTable,
        //pictures
        picturesTable, 
        pictureTagsTable,
        pictureFilesTable,
        picturePixivIdsTable,
        pictureTweetIdsTable,
        tagsTable
    };
    const QStringList indexes = {
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
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags_pixiv_id ON pixiv_artworks_tags(tag_id, pixiv_id)"
    };
    database.transaction();
    for (const QString &tableSql : tables) {
        if (!query.exec(tableSql)) {
            qCritical() << "Failed to create table:" << query.lastError().text();
            database.rollback();
            return false;
        }
    }
    for (const QString &indexSql : indexes) {
        if (!query.exec(indexSql)) {
            qCritical() << "Failed to create index:" << query.lastError().text();
            database.rollback();
            return false;
        }
    }
    if (!setupFTS5()) {
        qCritical() << "Failed to setup FTS5";
        database.rollback();
        return false;
    }
    database.commit();
    return true;
}
bool PicDatabase::setupFTS5() {
    QSqlQuery query(database);

    const QString pixivFtsTable = R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS pixiv_fts USING fts5(
            title,
            description,
            content=pixiv_artworks,
            content_rowid=pixiv_id
        )
    )";
    const QString tweetFtsTable = R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS tweets_fts USING fts5(
            description,
            content=tweets,
            content_rowid=tweet_id
        )
    )";
    if (!query.exec(pixivFtsTable)) {
        qCritical() << "Failed to create FTS5 table for pixiv_artworks:" << query.lastError().text();
        return false;
    }
    if (!query.exec(tweetFtsTable)) {
        qCritical() << "Failed to create FTS5 table for tweets:" << query.lastError().text();
        return false;
    }
    query.exec(R"(
        CREATE TRIGGER IF NOT EXISTS pixiv_artworks_ai AFTER INSERT ON pixiv_artworks BEGIN
            INSERT INTO pixiv_fts(rowid, title, description)
            VALUES (new.pixiv_id, new.title, new.description);
        END;
    )");
    query.exec(R"(
        CREATE TRIGGER IF NOT EXISTS pixiv_artworks_ad AFTER DELETE ON pixiv_artworks BEGIN
            INSERT INTO pixiv_fts(pixiv_fts, rowid, title, description)
            VALUES('delete', old.pixiv_id, old.title, old.description);
        END;
    )");
    query.exec(R"(
        CREATE TRIGGER IF NOT EXISTS pixiv_artworks_au AFTER UPDATE ON pixiv_artworks BEGIN
            INSERT INTO pixiv_fts(pixiv_fts, rowid, title, description)
            VALUES('delete', old.pixiv_id, old.title, old.description);
            INSERT INTO pixiv_fts(rowid, title, description)
            VALUES (new.pixiv_id, new.title, new.description);
        END;
    )");
    query.exec(R"(
        CREATE TRIGGER IF NOT EXISTS tweets_ai AFTER INSERT ON tweets BEGIN
            INSERT INTO tweets_fts(rowid, description)
            VALUES (new.tweet_id, new.description);
        END;
    )");
    query.exec(R"(
        CREATE TRIGGER IF NOT EXISTS tweets_ad AFTER DELETE ON tweets BEGIN
            INSERT INTO tweets_fts(tweets_fts, rowid, description)
            VALUES('delete', old.tweet_id, old.description);
        END;
    )");
    query.exec(R"(
        CREATE TRIGGER IF NOT EXISTS tweets_au AFTER UPDATE ON tweets BEGIN
            INSERT INTO tweets_fts(tweets_fts, rowid, description)
            VALUES('delete', old.tweet_id, old.description);
            INSERT INTO tweets_fts(rowid, description)
            VALUES (new.tweet_id, new.description);
        END;
    )");
    return true;
}
void PicDatabase::getTagMapping() {
    tagToId.clear();
    twitterHashtagToId.clear();
    pixivTagToId.clear();
    QSqlQuery query(database);
    if (!query.exec("SELECT id, tag FROM tags")) {
        qWarning() << "Failed to fetch tags:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        int id = query.value(0).toInt();
        QString tag = query.value(1).toString();
        tagToId[tag.toStdString()] = id;
        idToTag[id] = tag.toStdString();
    }
    if (!query.exec("SELECT id, hashtag FROM twitter_hashtags")) {
        qWarning() << "Failed to fetch twitter hashtags:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        int id = query.value(0).toInt();
        QString hashtag = query.value(1).toString();
        twitterHashtagToId[hashtag.toStdString()] = id;
        idToTwitterHashtag[id] = hashtag.toStdString();
    }
    if (!query.exec("SELECT id, tag FROM pixiv_tags")) {
        qWarning() << "Failed to fetch pixiv tags:" << query.lastError().text();
        return;
    }
    while (query.next()) {
        int id = query.value(0).toInt();
        QString tag = query.value(1).toString();
        pixivTagToId[tag.toStdString()] = id;
        idToPixivTag[id] = tag.toStdString();
    }
    qInfo() << "Tag mappings loaded. Tags:" << tagToId.size()
            << "Twitter Hashtags:" << twitterHashtagToId.size()
            << "Pixiv Tags:" << pixivTagToId.size();
}
bool PicDatabase::insertPicInfo(const PicInfo& picInfo) {
    if (!insertPicture(picInfo)) {
        qCritical() << "insertPicture failed.";
        return false;
    }
    if (!insertPictureFilePath(picInfo)) {
        qCritical() << "insertPictureFilePath failed.";
        return false;
    }
    if (!insertPictureTags(picInfo)) {
        qCritical() << "insertPictureTags failed.";
        return false;
    }
    if (!insertPicturePixivId(picInfo)) {
        qCritical() << "insertPicturePixivId failed.";
        return false;
    }
    if (!insertPictureTweetId(picInfo)) {
        qCritical() << "insertPictureTweetId failed.";
        return false;
    }
    return true;
}
bool PicDatabase::insertPicture(const PicInfo& picInfo) {
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT OR IGNORE INTO pictures(
            id, width, height, size, x_restrict
        ) VALUES (
            :id, :width, :height, :size, :x_restrict
        )
    )");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(picInfo.id)));// SQLite database does not support uint64 as integer primary key,
    query.bindValue(":width", QVariant::fromValue(picInfo.width));           // convert to a signed integer with the same binary representation for storage
    query.bindValue(":height", QVariant::fromValue(picInfo.height));
    query.bindValue(":size", QVariant::fromValue(picInfo.size));
    query.bindValue(":x_restrict", static_cast<int>(picInfo.xRestrict));
    if (!query.exec()){
        qCritical() << "Failed to insert picture: " << query.lastError().text();
        return false;
    }
    return true;
}
bool PicDatabase::insertPictureFilePath(const PicInfo& picInfo) {
    QSqlQuery query(database);
    for (const auto& filePath : picInfo.filePaths) {
        query.prepare(R"(
            INSERT OR IGNORE INTO picture_file_paths(
                id, file_path
            ) VALUES (
                :id, :file_path
            )
        )");
        query.bindValue(":id", QVariant::fromValue(uint64_to_int64(picInfo.id)));
        query.bindValue(":file_path", QString::fromStdString(filePath.string()));
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_file_path: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
bool PicDatabase::insertPictureTags(const PicInfo& picInfo) {
    QSqlQuery query(database);
    for (const auto& tag : picInfo.tags) {
        if (tagToId.find(tag) == tagToId.end()) {
            QSqlQuery insertTagQuery(database);
            insertTagQuery.prepare(R"(
                INSERT OR IGNORE INTO tags(tag, count) VALUES (:tag, 0)
            )");
            insertTagQuery.bindValue(":tag", QString::fromStdString(tag));
            if (!insertTagQuery.exec()) {
                qCritical() << "Failed to insert tag: " << insertTagQuery.lastError().text();
                return false;
            }
            int newId = insertTagQuery.lastInsertId().toInt();
            tagToId[tag] = newId;
            idToTag[newId] = tag;
        }
        query.prepare(R"(
            INSERT OR IGNORE INTO picture_tags(
                id, tag_id
            ) VALUES (
                :id, :tag_id
            )
        )");
        query.bindValue(":id", QVariant::fromValue(uint64_to_int64(picInfo.id)));
        query.bindValue(":tag_id", tagToId[tag]);
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_tag: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
bool PicDatabase::insertPicturePixivId(const PicInfo& picInfo) {
    QSqlQuery query(database);
    for (const auto& pair : picInfo.pixivIdIndices) {
        uint64_t pixivId = pair.first;
        int pixivIndex = pair.second;
        query.prepare(R"(
            INSERT OR IGNORE INTO picture_pixiv_ids(
                id, pixiv_id, pixiv_index
            ) VALUES (
                :id, :pixiv_id, :pixiv_index
            )
        )");
        query.bindValue(":id", QVariant::fromValue(uint64_to_int64(picInfo.id)));
        query.bindValue(":pixiv_id", QVariant::fromValue(pixivId));
        query.bindValue(":pixiv_index", pixivIndex);
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_pixiv_id: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
bool PicDatabase::insertPictureTweetId(const PicInfo& picInfo) {
    QSqlQuery query(database);
    for (const auto& pair : picInfo.tweetIdIndices) {
        uint64_t tweetId = pair.first;
        int tweetIndex = pair.second;
        query.prepare(R"(
            INSERT OR IGNORE INTO picture_tweet_ids(
                id, tweet_id, tweet_index
            ) VALUES (
                :id, :tweet_id, :tweet_index
            )
        )");
        query.bindValue(":id", QVariant::fromValue(uint64_to_int64(picInfo.id)));
        query.bindValue(":tweet_id", QVariant::fromValue(tweetId));
        query.bindValue(":tweet_index", tweetIndex);
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_tweet_id: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
bool PicDatabase::insertTweetInfo(const TweetInfo& tweetInfo) {
    if (!insertTweet(tweetInfo)) {
        qCritical() << "insertTweet failed.";
        return false;
    }
    if (!insertTweetHashtags(tweetInfo)) {
        qCritical() << "insertTweetHashtags failed.";
        return false;
    }
    return true;
}
bool PicDatabase::insertTweet(const TweetInfo& tweetInfo) {
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT OR IGNORE INTO tweets(
            tweet_id, date, author_id, author_name, author_nick, 
            author_description, author_profile_image, author_profile_banner,
            favorite_count, quote_count, reply_count, retweet_count, bookmark_count, view_count, description
        ) VALUES (
            :tweet_id, :date, :author_id, :author_name, :author_nick, 
            :author_description, :author_profile_image, :author_profile_banner,
            :favorite_count, :quote_count, :reply_count, :retweet_count, :bookmark_count, :view_count, :description
        )
    )");
    query.bindValue(":tweet_id", QVariant::fromValue(tweetInfo.tweetID));
    query.bindValue(":date", QString::fromStdString(tweetInfo.date));
    query.bindValue(":author_id", QVariant::fromValue(tweetInfo.authorID));
    query.bindValue(":author_name", QString::fromStdString(tweetInfo.authorName));
    query.bindValue(":author_nick", QString::fromStdString(tweetInfo.authorNick));
    query.bindValue(":author_description", QString::fromStdString(tweetInfo.authorDescription));
    query.bindValue(":author_profile_image", QString::fromStdString(tweetInfo.authorProfileImage));
    query.bindValue(":author_profile_banner", QString::fromStdString(tweetInfo.authorProfileBanner));
    query.bindValue(":favorite_count", QVariant::fromValue(tweetInfo.favoriteCount));
    query.bindValue(":quote_count", QVariant::fromValue(tweetInfo.quoteCount));
    query.bindValue(":reply_count", QVariant::fromValue(tweetInfo.replyCount));
    query.bindValue(":retweet_count", QVariant::fromValue(tweetInfo.retweetCount));
    query.bindValue(":bookmark_count", QVariant::fromValue(tweetInfo.bookmarkCount));
    query.bindValue(":view_count", QVariant::fromValue(tweetInfo.viewCount));
    query.bindValue(":description", QString::fromStdString(tweetInfo.description));
    if (!query.exec()) {
        qCritical() << "Failed to insert tweet: " << query.lastError().text();
        return false;
    }
    return true;
}
bool PicDatabase::insertTweetHashtags(const TweetInfo& tweetInfo) {
    QSqlQuery query(database);
    for (const auto& hashtag : tweetInfo.hashtags) {
        if (twitterHashtagToId.find(hashtag) == twitterHashtagToId.end()) {
            QSqlQuery insertHashtagQuery(database);
            insertHashtagQuery.prepare(R"(
                INSERT OR IGNORE INTO twitter_hashtags(hashtag, count) VALUES (:hashtag, 0)
            )");
            insertHashtagQuery.bindValue(":hashtag", QString::fromStdString(hashtag));
            if (!insertHashtagQuery.exec()) {
                qCritical() << "Failed to insert twitter hashtag: " << insertHashtagQuery.lastError().text();
                return false;
            }
            int newId = insertHashtagQuery.lastInsertId().toInt();
            twitterHashtagToId[hashtag] = newId;
            idToTwitterHashtag[newId] = hashtag;
        }
        query.prepare(R"(
            INSERT OR IGNORE INTO tweet_hashtags(
                tweet_id, hashtag_id
            ) VALUES (
                :tweet_id, :hashtag_id
            )
        )");
        query.bindValue(":tweet_id", QVariant::fromValue(tweetInfo.tweetID));
        query.bindValue(":hashtag_id", QVariant::fromValue(twitterHashtagToId.at(hashtag)));
        if (!query.exec()) {
            qCritical() << "Failed to insert tweet_hashtag: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
bool PicDatabase::insertPixivInfo(const PixivInfo& pixivInfo) {
    if (!insertPixivArtwork(pixivInfo)) {
        qCritical() << "insertPixivArtwork failed.";
        return false;
    }
    if (!insertPixivArtworkTags(pixivInfo)) {
        qCritical() << "insertPixivArtworkTags failed.";
        return false;
    }
    return true;
}
bool PicDatabase::insertPixivArtwork(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT OR IGNORE INTO pixiv_artworks(
            pixiv_id, date, author_name, author_id, title, description,
            like_count, view_count, x_restrict, ai_type
        ) VALUES (
            :pixiv_id, :date, :author_name, :author_id, :title, :description,
            :like_count, :view_count, :x_restrict, :ai_type
        )
    )");
    query.bindValue(":pixiv_id", QVariant::fromValue(pixivInfo.pixivID));
    query.bindValue(":date", QString::fromStdString(pixivInfo.date));
    query.bindValue(":author_name", QString::fromStdString(pixivInfo.authorName));
    query.bindValue(":author_id", QVariant::fromValue(pixivInfo.authorID));
    query.bindValue(":title", QString::fromStdString(pixivInfo.title));
    query.bindValue(":description", QString::fromStdString(pixivInfo.description));
    query.bindValue(":like_count", QVariant::fromValue(pixivInfo.likeCount));
    query.bindValue(":view_count", QVariant::fromValue(pixivInfo.viewCount));
    query.bindValue(":x_restrict", static_cast<int>(pixivInfo.xRestrict));
    query.bindValue(":ai_type", static_cast<int>(pixivInfo.aiType));
    if (!query.exec()) {
        qCritical() << "Failed to insert pixiv_artwork: " << query.lastError().text();
        return false;
    }
    newPixivIDs.insert(pixivInfo.pixivID);
    return true;
}
bool PicDatabase::insertPixivArtworkTags(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    for (const auto& tag : pixivInfo.tags) {
        if (pixivTagToId.find(tag) == pixivTagToId.end()) {
            QSqlQuery insertTagQuery(database);
            insertTagQuery.prepare(R"(
                INSERT OR IGNORE INTO pixiv_tags(tag, count) VALUES (:tag, 0)
            )");
            insertTagQuery.bindValue(":tag", QString::fromStdString(tag));
            if (!insertTagQuery.exec()) {
                qCritical() << "Failed to insert pixiv tag: " << insertTagQuery.lastError().text();
                return false;
            }
            int newId = insertTagQuery.lastInsertId().toInt();
            pixivTagToId[tag] = newId;
            idToPixivTag[newId] = tag;
        }
        query.prepare(R"(
            INSERT OR IGNORE INTO pixiv_artworks_tags(
                pixiv_id, tag_id
            ) VALUES (
                :pixiv_id, :tag_id
            )
        )");
        query.bindValue(":pixiv_id", QVariant::fromValue(pixivInfo.pixivID));
        query.bindValue(":tag_id", pixivTagToId.at(tag));
        if (!query.exec()) {
            qCritical() << "Failed to insert pixiv_artworks_tags: " << query.lastError().text();
            return false;
        }
    }
    if (pixivInfo.tags.size() == pixivInfo.tagsTransl.size()) {
        for (size_t i = 0; i < pixivInfo.tags.size(); ++i) {
            QSqlQuery updateTranslQuery(database);
            updateTranslQuery.prepare(R"(
                UPDATE pixiv_tags SET translated_tag = :translated_tag WHERE id = :id
            )");
            updateTranslQuery.bindValue(":translated_tag", QString::fromStdString(pixivInfo.tagsTransl[i]));
            updateTranslQuery.bindValue(":id", pixivTagToId[pixivInfo.tags[i]]);
            if (!updateTranslQuery.exec()) {
                qCritical() << "Failed to update translated_tag: " << updateTranslQuery.lastError().text();
                return false;
            }
        }
    }
    return true;
}
bool PicDatabase::updatePixivInfo(const PixivInfo& pixivInfo) {
    if (!updatePixivArtwork(pixivInfo)) {
        qCritical() << "updatePixivArtwork failed.";
        return false;
    }
    if (!updatePixivArtworkTags(pixivInfo)) {
        qCritical() << "updatePixivArtworkTags failed.";
        return false;
    }
    return true;
}
bool PicDatabase::updatePixivArtwork(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO pixiv_artworks(
            pixiv_id, date, author_name, author_id, title, description,
            like_count, view_count, x_restrict, ai_type
        ) VALUES (
            :pixiv_id, :date, :author_name, :author_id, :title, :description,
            :like_count, :view_count, :x_restrict, :ai_type
        )
        ON CONFLICT(pixiv_id) DO UPDATE SET
            date=excluded.date,
            author_name=excluded.author_name,
            author_id=excluded.author_id,
            title=excluded.title,
            description=excluded.description,
            like_count=excluded.like_count,
            view_count=excluded.view_count,
            x_restrict=excluded.x_restrict,
            ai_type=excluded.ai_type
    )");
    query.bindValue(":pixiv_id", QVariant::fromValue(pixivInfo.pixivID));
    query.bindValue(":date", QString::fromStdString(pixivInfo.date));
    query.bindValue(":author_name", QString::fromStdString(pixivInfo.authorName));
    query.bindValue(":author_id", QVariant::fromValue(pixivInfo.authorID));
    query.bindValue(":title", QString::fromStdString(pixivInfo.title));
    query.bindValue(":description", QString::fromStdString(pixivInfo.description));
    query.bindValue(":like_count", QVariant::fromValue(pixivInfo.likeCount));
    query.bindValue(":view_count", QVariant::fromValue(pixivInfo.viewCount));
    query.bindValue(":x_restrict", static_cast<int>(pixivInfo.xRestrict));
    query.bindValue(":ai_type", static_cast<int>(pixivInfo.aiType));
    if (!query.exec()) {
        qCritical() << "Failed to update pixiv_artwork: " << query.lastError().text();
        return false;
    }
    newPixivIDs.insert(pixivInfo.pixivID);
    return true;
}
bool PicDatabase::updatePixivArtworkTags(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    for (const auto& tag : pixivInfo.tags) {
        if (pixivTagToId.find(tag) == pixivTagToId.end()) {
            QSqlQuery insertTagQuery(database);
            insertTagQuery.prepare(R"(
                INSERT INTO pixiv_tags(tag, count) VALUES (:tag, 0)
                ON CONFLICT(tag) DO NOTHING
            )");
            insertTagQuery.bindValue(":tag", QString::fromStdString(tag));
            if (!insertTagQuery.exec()) {
                qCritical() << "Failed to insert pixiv tag: " << insertTagQuery.lastError().text();
                return false;
            }
            int newId = insertTagQuery.lastInsertId().toInt();
            pixivTagToId[tag] = newId;
            idToPixivTag[newId] = tag;
        }
        query.prepare(R"(
            INSERT INTO pixiv_artworks_tags(
                pixiv_id, tag_id
            ) VALUES (
                :pixiv_id, :tag_id
            )
            ON CONFLICT(pixiv_id, tag_id) DO NOTHING
        )");
        query.bindValue(":pixiv_id", QVariant::fromValue(pixivInfo.pixivID));
        query.bindValue(":tag_id", pixivTagToId.at(tag));
        if (!query.exec()) {
            qCritical() << "Failed to update pixiv_artworks_tag: " << query.lastError().text();
            return false;
        }
    }
    if (pixivInfo.tags.size() == pixivInfo.tagsTransl.size()) {
        for (size_t i = 0; i < pixivInfo.tags.size(); ++i) {
            QSqlQuery updateTranslQuery(database);
            updateTranslQuery.prepare(R"(
                UPDATE pixiv_tags SET translated_tag = :translated_tag WHERE id = :id
            )");
            updateTranslQuery.bindValue(":translated_tag", QString::fromStdString(pixivInfo.tagsTransl[i]));
            updateTranslQuery.bindValue(":id", pixivTagToId[pixivInfo.tags[i]]);
            if (!updateTranslQuery.exec()) {
                qCritical() << "Failed to update translated_tag: " << updateTranslQuery.lastError().text();
                return false;
            }
        }
    }
    return true;
}
PicInfo PicDatabase::getPicInfo(uint64_t id, int64_t tweetID, int64_t pixivID) const {
    PicInfo info{};
    QSqlQuery query(database);

    // 查询主表
    query.prepare("SELECT width, height, size, x_restrict FROM pictures WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec() && query.next()) {
        info.id = id;
        info.width = query.value(0).toUInt();
        info.height = query.value(1).toUInt();
        info.size = query.value(2).toUInt();
        info.xRestrict = static_cast<XRestrictType>(query.value(3).toInt());
    } else {
        return info; // id 不存在，返回空对象
    }

    // 查询文件路径
    query.prepare("SELECT file_path FROM picture_file_paths WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.filePaths.insert(std::filesystem::path(query.value(0).toString().toStdString()));
        }
    }

    // 查询标签
    query.prepare("SELECT tag_id FROM picture_tags WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.tags.insert(idToTag.at(query.value(0).toInt()));
        }
    }

    // 查询 Pixiv 关联
    query.prepare("SELECT pixiv_id, pixiv_index FROM picture_pixiv_ids WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.pixivIdIndices[query.value(0).toULongLong()] = query.value(1).toInt();
        }
    }

    // 查询 Tweet 关联
    query.prepare("SELECT tweet_id, tweet_index FROM picture_tweet_ids WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.tweetIdIndices[query.value(0).toULongLong()] = query.value(1).toInt();
        }
    }

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
TweetInfo PicDatabase::getTweetInfo(int64_t tweetID) const {
    TweetInfo info{};
    QSqlQuery query(database);

    // 查询主表
    query.prepare("SELECT date, author_id, author_name, author_nick, author_description, favorite_count, quote_count, reply_count, retweet_count, bookmark_count, view_count, description FROM tweets WHERE tweet_id = :tweet_id");
    query.bindValue(":tweet_id", QVariant::fromValue(tweetID));
    if (query.exec() && query.next()) {
        info.tweetID = tweetID;
        info.date = query.value(0).toString().toStdString();
        info.authorID = query.value(1).toUInt();
        info.authorName = query.value(2).toString().toStdString();
        info.authorNick = query.value(3).toString().toStdString();
        info.authorDescription = query.value(4).toString().toStdString();
        info.authorProfileImage = query.value(5).toString().toStdString();
        info.authorProfileBanner = query.value(6).toString().toStdString();
        info.favoriteCount = query.value(7).toUInt();
        info.quoteCount = query.value(8).toUInt();
        info.replyCount = query.value(9).toUInt();
        info.retweetCount = query.value(10).toUInt();
        info.bookmarkCount = query.value(11).toUInt();
        info.viewCount = query.value(12).toUInt();
        info.description = query.value(13).toString().toStdString();
    } else {
        return info;
    }

    // 查询 hashtags
    query.prepare("SELECT hashtag_id FROM tweet_hashtags WHERE tweet_id = :tweet_id");
    query.bindValue(":tweet_id", QVariant::fromValue(tweetID));
    if (query.exec()) {
        while (query.next()) {
            info.hashtags.insert(idToTwitterHashtag.at(query.value(0).toInt()));
        }
    }

    return info;
}
PixivInfo PicDatabase::getPixivInfo(int64_t pixivID) const {
    PixivInfo info{};
    QSqlQuery query(database);

    // 查询主表
    query.prepare("SELECT date, author_name, author_id, title, description, like_count, view_count, x_restrict FROM pixiv_artworks WHERE pixiv_id = :pixiv_id");
    query.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
    if (query.exec() && query.next()) {
        info.pixivID = pixivID;
        info.date = query.value(0).toString().toStdString();
        info.authorName = query.value(1).toString().toStdString();
        info.authorID = query.value(2).toUInt();
        info.title = query.value(3).toString().toStdString();
        info.description = query.value(4).toString().toStdString();
        info.likeCount = query.value(5).toUInt();
        info.viewCount = query.value(6).toUInt();
        info.xRestrict = static_cast<XRestrictType>(query.value(7).toInt());
        info.aiType = static_cast<AIType>(query.value(8).toInt());
    } else {
        return info;
    }

    // 查询 tags
    query.prepare("SELECT tag_id FROM pixiv_artworks_tags WHERE pixiv_id = :pixiv_id");
    query.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
    if (query.exec()) {
        while (query.next()) {
            info.tags.push_back(idToPixivTag.at(query.value(0).toInt()));
            // For translated tags, we need to query the pixiv_tags table
            QSqlQuery translQuery(database);
            translQuery.prepare("SELECT translated_tag FROM pixiv_tags WHERE id = :id");
            translQuery.bindValue(":id", query.value(0).toInt());
            if (translQuery.exec() && translQuery.next()) {
                info.tagsTransl.push_back(translQuery.value(0).toString().toStdString());
            } else {
                info.tagsTransl.push_back(""); // No translation available
            }
        }
    }
    return info;
}
void PicDatabase::processSingleFile(const std::filesystem::path& path, ParserType parserType) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif" || ext == ".webp") {
            PicInfo picInfo = parsePicture(path, parserType);
            insertPicInfo(picInfo);
        } 
        else if (ext == ".txt" && parserType == ParserType::Pixiv) {
            PixivInfo pixivInfo = parsePixivMetadata(path);
            insertPixivInfo(pixivInfo);
        }
        else if (ext == ".json") {
            QByteArray data = readJsonFile(path);
            if (parserType == ParserType::Pixiv) {
                std::vector<PixivInfo> pixivInfoVec = parsePixivJson(data);
                for (const auto& pixivInfo : pixivInfoVec) {
                    updatePixivInfo(pixivInfo);
                }
            } 
            else if (parserType == ParserType::Twitter) {
                TweetInfo tweetInfo = parseTweetJson(data);
                insertTweetInfo(tweetInfo);
            }
        }
        else if (ext == ".csv" && parserType == ParserType::Pixiv) {
            std::vector<PixivInfo> pixivInfoVec = parsePixivCsv(path);
            for (const auto& pixivInfo : pixivInfoVec) {
                updatePixivInfo(pixivInfo);
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Error processing file:" << path.c_str() << "Error:" << e.what();
    }
}
void PicDatabase::scanDirectory(const std::filesystem::path& directory, ParserType parserType){
    auto files = collectAllFiles(directory);
    QSqlQuery query(database);

    query.exec("PRAGMA foreign_keys = OFF;");

    const size_t BATCH_SIZE = 1000;
    size_t processed = 0;
    for (size_t i = 0; i < files.size(); i += BATCH_SIZE) {
        database.transaction();
        auto batch_end = std::min(i + BATCH_SIZE, files.size());
        for (size_t j = i; j < batch_end; j++) {
            processSingleFile(files[j], parserType);
            processed++;
        }
        if (!database.commit()) {
            qWarning() << "Batch commit failed, rolling back" << database.lastError().text();
            database.rollback();
        }
        qInfo() << "Processed files: " << processed;
    }
    qInfo() << "Scan completed. Total files processed:" << processed;
}
void PicDatabase::syncTables() {
    QSqlQuery query(database);
    // count tags
    if (!query.exec(R"(
        UPDATE tags SET count = (
            SELECT COUNT(*) FROM picture_tags WHERE tag_id = tags.id
        )
    )")) {
        qWarning() << "Failed to count tags:" << query.lastError().text();
    }
    if (!query.exec(R"(
        UPDATE twitter_hashtags SET count = (
            SELECT COUNT(*) FROM tweet_hashtags WHERE hashtag_id = twitter_hashtags.id
        )
    )")) {
        qWarning() << "Failed to count twitter hashtags:" << query.lastError().text();
    }
    if (!query.exec(R"(
        UPDATE pixiv_tags SET count = (
            SELECT COUNT(*) FROM pixiv_artworks_tags WHERE tag_id = pixiv_tags.id
        )
    )")) {
        qWarning() << "Failed to count pixiv tags:" << query.lastError().text();
    }
    // sync x_restrict and ai_type from pixiv_artworks to pictures
    for (const auto& pixivID : newPixivIDs) {
        QSqlQuery updateQuery(database);
        updateQuery.prepare(R"(
            UPDATE pictures SET x_restrict = (
                SELECT x_restrict FROM pixiv_artworks WHERE pixiv_id = :pixiv_id
            ), ai_type = (
                SELECT ai_type FROM pixiv_artworks WHERE pixiv_id = :pixiv_id
            ) WHERE id IN (
                SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
            )
        )");
        updateQuery.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
        if (!updateQuery.exec()) {
            qWarning() << "Failed to sync pixiv info for pixiv_id:" << pixivID << "Error:" << updateQuery.lastError().text();
        }
    }
    query.exec("PRAGMA foreign_keys = ON");
}
std::vector<uint64_t> PicDatabase::tagSearch(
    const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags
) {
    std::vector<uint64_t> results;
    if (includedTags.empty() && excludedTags.empty()) return results;

    QString includedTagIdStr;
    QString excludedTagIdStr;
    if (!includedTags.empty()) {
        int idx = 0;
        for (const auto& tag : includedTags) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += QString::number(tagToId.at(tag));
        }
        if (includedTagIdStr == "") return results; // No included tags found in database
    }
    if (!excludedTags.empty()) {
        int idx = 0;
        for (const auto& tag : excludedTags) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += QString::number(tagToId.at(tag));
        }
        
    }
    QString sql = "SELECT id FROM picture_tags WHERE 1=1 AND tag_id IN (" + 
        includedTagIdStr + ") AND id NOT IN (SELECT id FROM picture_tags WHERE tag_id IN (" +
        excludedTagIdStr + ")) GROUP BY id HAVING COUNT(DISTINCT tag_id) = " + 
        QString::number(includedTags.size());

    QSqlQuery query(database);
    if (!query.exec(sql)) {
        qWarning() << "Tag search failed:" << query.lastError().text();
        return results;
    }
    while (query.next()) {
        results.push_back(int64_to_uint64(query.value(0).toLongLong()));
    }
    return results;
}
std::vector<uint64_t> PicDatabase::pixivTagSearch(
    const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags
) {
    std::vector<uint64_t> results;
    if (includedTags.empty() && excludedTags.empty()) return results;
    
    QString includedTagIdStr;
    QString excludedTagIdStr;
    if (!includedTags.empty()) {
        int idx = 0;
        for (const auto& tag : includedTags) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += QString::number(pixivTagToId.at(tag));
        }
        if (includedTagIdStr == "") return results; // No included tags found in database
    }
    if (!excludedTags.empty()) {
        int idx = 0;
        for (const auto& tag : excludedTags) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += QString::number(pixivTagToId.at(tag));
        }   
    }
    QString sql = "SELECT pixiv_id FROM pixiv_artworks_tags WHERE 1=1 AND tag_id IN (" + 
        includedTagIdStr + ") AND pixiv_id NOT IN (SELECT pixiv_id FROM pixiv_artworks_tags WHERE tag_id IN (" +
        excludedTagIdStr + ")) GROUP BY pixiv_id HAVING COUNT(DISTINCT tag_id) = " + 
        QString::number(includedTags.size());

    QSqlQuery query(database);
    if (!query.exec(sql)) {
        qWarning() << "Pixiv tag search failed:" << query.lastError().text();
        return results;
    }
    while (query.next()) {
        int64_t pixivID = query.value(0).toLongLong();
        QSqlQuery picQuery(database);
        picQuery.prepare(R"(
            SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
        )");
        picQuery.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
        if (picQuery.exec() && picQuery.next()) {
            results.push_back(int64_to_uint64(picQuery.value(0).toLongLong()));
        }
    }
    return results;
}
std::vector<uint64_t> PicDatabase::tweetHashtagSearch(
    const std::unordered_set<std::string>& includedTags, const std::unordered_set<std::string>& excludedTags
) {
    std::vector<uint64_t> results;
    if (includedTags.empty() && excludedTags.empty()) return results;
    
    QString includedTagIdStr;
    QString excludedTagIdStr;
    if (!includedTags.empty()) {
        int idx = 0;
        for (const auto& tag : includedTags) {
            if (idx++) includedTagIdStr += ",";
            includedTagIdStr += QString::number(twitterHashtagToId.at(tag));
        }
        if (includedTagIdStr == "") return results; // No included tags found in database
    }
    if (!excludedTags.empty()) {
        int idx = 0;
        for (const auto& tag : excludedTags) {
            if (idx++) excludedTagIdStr += ",";
            excludedTagIdStr += QString::number(twitterHashtagToId.at(tag));
        }   
    }
    QString sql = "SELECT tweet_id FROM tweet_hashtags WHERE 1=1 AND hashtag_id IN (" + 
        includedTagIdStr + ") AND tweet_id NOT IN (SELECT tweet_id FROM tweet_hashtags WHERE hashtag_id IN (" +
        excludedTagIdStr + ")) GROUP BY tweet_id HAVING COUNT(DISTINCT hashtag_id) = " + 
        QString::number(includedTags.size());

    QSqlQuery query(database);
    if (!query.exec(sql)) {
        qWarning() << "Tweet hashtag search failed:" << query.lastError().text();
        return results;
    }
    while (query.next()) {
        int64_t tweetID = query.value(0).toLongLong();
        QSqlQuery picQuery(database);
        picQuery.prepare(R"(
            SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
        )");
        picQuery.bindValue(":tweet_id", QVariant::fromValue(tweetID));
        if (picQuery.exec() && picQuery.next()) {
            results.push_back(int64_to_uint64(picQuery.value(0).toLongLong()));
        }
    }
    return results;
}
std::unordered_map<uint64_t, int64_t> PicDatabase::textSearch(const std::string& searchText, SearchField searchField) {
    std::unordered_map<uint64_t, int64_t> results; // id -> tweet_id or pixiv_id
    if (searchText.empty() || searchField == SearchField::None) return results;
    switch (searchField) {
        case SearchField::PicID: {
            uint64_t id = std::stoull(searchText);
            QSqlQuery query(database);
            query.prepare("SELECT id FROM pictures WHERE id = :id");
            query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
            if (query.exec() && query.next()) {
                results[id] = 0;
            }
        }
        break;
        case SearchField::PixivID: {
            int64_t pixivID = std::stoll(searchText);
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
            )");
            query.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
            if (query.exec()) {
                while (query.next()) {
                    uint64_t id = int64_to_uint64(query.value(0).toLongLong());
                    results[id] = pixivID;
                }
            }
        }
        break;
        case SearchField::PixivAuthorID: {
            int64_t authorID = std::stoll(searchText);
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT pixiv_id FROM pixiv_artworks WHERE author_id = :author_id
            )");
            query.bindValue(":author_id", QVariant::fromValue(authorID));
            if (query.exec()) {
                while (query.next()) {
                    int64_t pixivID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
                    )");
                    picQuery.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = pixivID;
                    }
                }
            }
        }
        break;
        case SearchField::PixivAuthorName: {
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT pixiv_id FROM pixiv_artworks WHERE author_name LIKE ?
            )");
            query.bindValue(0, QString::fromStdString(searchText) + "%");
            if (query.exec()) {
                while (query.next()) {
                    int64_t pixivID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
                    )");
                    picQuery.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = pixivID;
                    }
                }
            }
        }
        break;
        case SearchField::PixivTitle: {
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT pixiv_id FROM pixiv_fts WHERE title MATCH ?
            )");
            query.bindValue(0, QString::fromStdString(searchText));
            if (query.exec()) {
                while (query.next()) {
                    int64_t pixivID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
                    )");
                    picQuery.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = pixivID;
                    }
                }
            }
        }
        break;
        case SearchField::PixivDescription: {
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT pixiv_id FROM pixiv_fts WHERE description MATCH ?
            )");
            query.bindValue(0, QString::fromStdString(searchText));
            if (query.exec()) {
                while (query.next()) {
                    int64_t pixivID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_pixiv_ids WHERE pixiv_id = :pixiv_id
                    )");
                    picQuery.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = pixivID;
                    }
                }
            }
        }
        break;
        case SearchField::TweetID: {
            int64_t tweetID = std::stoll(searchText);
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
            )");
            query.bindValue(":tweet_id", QVariant::fromValue(tweetID));
            if (query.exec()) {
                while (query.next()) {
                    uint64_t id = int64_to_uint64(query.value(0).toLongLong());
                    results[id] = tweetID;
                }
            }
        }
        break;
        case SearchField::TweetAuthorID: {
            int64_t authorID = std::stoll(searchText);
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT tweet_id FROM tweets WHERE author_id = :author_id
            )");
            query.bindValue(":author_id", QVariant::fromValue(authorID));
            if (query.exec()) {
                while (query.next()) {
                    int64_t tweetID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
                    )");
                    picQuery.bindValue(":tweet_id", QVariant::fromValue(tweetID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = tweetID;
                    }
                }
            }
        }
        break;
        case SearchField::TweetAuthorName: {
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT tweet_id FROM tweets WHERE author_name LIKE ?
            )");
            query.bindValue(0, QString::fromStdString(searchText) + "%");
            if (query.exec()) {
                while (query.next()) {
                    int64_t tweetID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
                    )");
                    picQuery.bindValue(":tweet_id", QVariant::fromValue(tweetID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = tweetID;
                    }
                }
            }
        }
        break;
        case SearchField::TweetAuthorNick: {
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT tweet_id FROM tweets WHERE author_nick LIKE ?
            )");
            query.bindValue(0, QString::fromStdString(searchText) + "%");
            if (query.exec()) {
                while (query.next()) {
                    int64_t tweetID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
                    )");
                    picQuery.bindValue(":tweet_id", QVariant::fromValue(tweetID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = tweetID;
                    }
                }
            }
        }
        break;
        case SearchField::TweetDescription: {
            QSqlQuery query(database);
            query.prepare(R"(
                SELECT tweet_id FROM tweet_fts WHERE description MATCH ?
            )");
            query.bindValue(0, QString::fromStdString(searchText));
            if (query.exec()) {
                while (query.next()) {
                    int64_t tweetID = query.value(0).toLongLong();
                    QSqlQuery picQuery(database);
                    picQuery.prepare(R"(
                        SELECT id FROM picture_tweet_ids WHERE tweet_id = :tweet_id
                    )");
                    picQuery.bindValue(":tweet_id", QVariant::fromValue(tweetID));
                    if (picQuery.exec() && picQuery.next()) {
                        uint64_t id = int64_to_uint64(picQuery.value(0).toLongLong());
                        results[id] = tweetID;
                    }
                }
            }
        }
        break;
    }
    return results;
}
std::vector<std::pair<std::string, int>> PicDatabase::getGeneralTags() const {
    std::vector<std::pair<std::string, int>> tagCounts;
    QSqlQuery query(database);
    if (!query.exec("SELECT tag, count FROM tags WHERE is_character = false")) {
        qWarning() << "Failed to retrieve general tags:" << query.lastError().text();
        return tagCounts;
    }
    while (query.next()) {
        std::string tag = query.value(0).toString().toStdString();
        int count = query.value(1).toInt();
        tagCounts.push_back({tag, count});
    }
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // 按照 count 降序排序
    });
    return tagCounts;
}
std::vector<std::pair<std::string, int>> PicDatabase::getCharacterTags() const {
    std::vector<std::pair<std::string, int>> tagCounts;
    QSqlQuery query(database);
    if (!query.exec("SELECT tag, count FROM tags WHERE is_character = true")) {
        qWarning() << "Failed to retrieve character tags:" << query.lastError().text();
        return tagCounts;
    }
    while (query.next()) {
        std::string tag = query.value(0).toString().toStdString();
        int count = query.value(1).toInt();
        tagCounts.push_back({tag, count});
    }
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // 按照 count 降序排序
    });
    return tagCounts;
}
std::vector<std::pair<std::string, int>> PicDatabase::getPixivTags() const {
    std::vector<std::pair<std::string, int>> tagCounts;
    QSqlQuery query(database);
    if (!query.exec("SELECT tag, count FROM pixiv_tags")) {
        qWarning() << "Failed to retrieve pixiv tags:" << query.lastError().text();
        return tagCounts;
    }
    while (query.next()) {
        std::string tag = query.value(0).toString().toStdString();
        int count = query.value(1).toInt();
        tagCounts.push_back({tag, count});
    }
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // 按照 count 降序排序
    });
    return tagCounts;
}
std::vector<std::pair<std::string, int>> PicDatabase::getTwitterHashtags() const {
    std::vector<std::pair<std::string, int>> tagCounts;
    QSqlQuery query(database);
    if (!query.exec("SELECT hashtag, count FROM twitter_hashtags")) {
        qWarning() << "Failed to retrieve twitter hashtags:" << query.lastError().text();
        return tagCounts;
    }
    while (query.next()) {
        std::string tag = query.value(0).toString().toStdString();
        int count = query.value(1).toInt();
        tagCounts.push_back({tag, count});
    }
    std::sort(tagCounts.begin(), tagCounts.end(), [](const auto& a, const auto& b) {
        return b.second < a.second; // 按照 count 降序排序
    });
    return tagCounts;
}