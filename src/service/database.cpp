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
std::vector<std::filesystem::path> collectAllFiles(const std::string& directory) {
    std::vector<std::filesystem::path> files;
    int fileCount = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        files.push_back(entry.path());
        if (++fileCount % 1000 == 0) {
            qInfo() << "Collected files:" << fileCount;
        }
        
    }
    qInfo() << "Total files collected:" << fileCount;
    return files;
}
PicDatabase::PicDatabase(QString databaseFile) {
    initDatabase(databaseFile);
}
PicDatabase::~PicDatabase() {
    database.close();
}
void PicDatabase::initDatabase(QString databaseFile){
    bool databaseExists = QFile::exists(databaseFile);
    database = QSqlDatabase::addDatabase("QSQLITE");
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
            x_restrict INTEGER DEFAULT NULL
        )
    )";//TODO: add feature vector from deep learning models
    const QString pictureTagsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tags (
            id INTEGER NOT NULL,
            tag TEXT NOT NULL,
            source INTEGER NOT NULL,
            PRIMARY KEY (id, tag),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";//source: 0 - pixiv, 1 - completed tags, 2 - ai predicted tags
    const QString pictureFilesTable = R"(
        CREATE TABLE IF NOT EXISTS picture_file_paths (
            id INTEGER NOT NULL,
            file_path TEXT NOT NULL,
            PRIMARY KEY (id, file_path),

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
            hashtag TEXT NOT NULL,
            PRIMARY KEY (tweet_id, hashtag),

            FOREIGN KEY (tweet_id) REFERENCES tweets(tweet_id) ON DELETE CASCADE
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
    )";// ai_type: 0-unknown, 1-not ai, 2-ai
    const QString pixivArtworksTagsTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_artworks_tags (
            pixiv_id INTEGER NOT NULL,
            tag TEXT NOT NULL,
            PRIMARY KEY (pixiv_id, tag),

            FOREIGN KEY (pixiv_id) REFERENCES pixiv_artworks(pixiv_id) ON DELETE CASCADE
        )
    )";
    const QString pixivTagsTable = R"(
        CREATE TABLE IF NOT EXISTS pixiv_tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tag TEXT NOT NULL,
            translated_tag TEXT
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
        //pictures
        picturesTable, 
        pictureTagsTable,
        pictureFilesTable,
        picturePixivIdsTable,
        pictureTweetIdsTable
    };
    const QStringList indexes = {
        // foreign key indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_pixiv_id ON picture_pixiv_ids(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tweet_id ON picture_tweet_ids(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_id ON picture_tags(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_file_paths_id ON picture_file_paths(id)",
        "CREATE INDEX IF NOT EXISTS idx_tweet_hashtags_id ON tweet_hashtags(tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags_id ON pixiv_artworks_tags(pixiv_id)",
        // tags indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_tags ON picture_tags(tag)",
        "CREATE INDEX IF NOT EXISTS idx_tweet_hashtags ON tweet_hashtags(hashtag)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags ON pixiv_artworks_tags(tag)",
        // author indexes
        "CREATE INDEX IF NOT EXISTS idx_tweets_author_id ON tweets(author_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_author_id ON pixiv_artworks(author_id)"
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
    database.commit();
    return true;
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
        query.bindValue(":file_path", QString::fromStdString(filePath));
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_file_path: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
bool PicDatabase::insertPictureTags(const PicInfo& picInfo) {
    QSqlQuery query(database);
    for (const auto& tagPair : picInfo.tags) {
        query.prepare(R"(
            INSERT OR IGNORE INTO picture_tags(
                id, tag, source
            ) VALUES (
                :id, :tag, :source
            )
        )");
        query.bindValue(":id", QVariant::fromValue(uint64_to_int64(picInfo.id)));
        query.bindValue(":tag", QString::fromStdString(tagPair.first));
        query.bindValue(":source", static_cast<int>(tagPair.second));
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
        query.prepare(R"(
            INSERT OR IGNORE INTO tweet_hashtags(
                tweet_id, hashtag
            ) VALUES (
                :tweet_id, :hashtag
            )
        )");
        query.bindValue(":tweet_id", QVariant::fromValue(tweetInfo.tweetID));
        query.bindValue(":hashtag", QString::fromStdString(hashtag));
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
    return true;
}
bool PicDatabase::insertPixivArtworkTags(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    for (const auto& tag : pixivInfo.tags) {
        query.prepare(R"(
            INSERT OR IGNORE INTO pixiv_artworks_tags(
                pixiv_id, tag
            ) VALUES (
                :pixiv_id, :tag
            )
        )");
        query.bindValue(":pixiv_id", QVariant::fromValue(pixivInfo.pixivID));
        query.bindValue(":tag", QString::fromStdString(tag));
        if (!query.exec()) {
            qCritical() << "Failed to insert pixiv_artworks_tag: " << query.lastError().text();
            return false;
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
    return true;
}
bool PicDatabase::updatePixivArtworkTags(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    for (const auto& tag : pixivInfo.tags) {
        query.prepare(R"(
            INSERT INTO pixiv_artworks_tags(
                pixiv_id, tag
            ) VALUES (
                :pixiv_id, :tag
            )
            ON CONFLICT(pixiv_id, tag) DO NOTHING
        )");
        query.bindValue(":pixiv_id", QVariant::fromValue(pixivInfo.pixivID));
        query.bindValue(":tag", QString::fromStdString(tag));
        if (!query.exec()) {
            qCritical() << "Failed to update pixiv_artworks_tag: " << query.lastError().text();
            return false;
        }
    }
    return true;
}
PicInfo PicDatabase::getPicInfo(uint64_t id) const {
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
            info.filePaths.insert(query.value(0).toString().toStdString());
        }
    }

    // 查询标签
    query.prepare("SELECT tag, source FROM picture_tags WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.tags.insert(std::make_pair(query.value(0).toString().toStdString(), static_cast<uint8_t>(query.value(1).toInt())));
        }
    }

    // 查询 Pixiv 关联
    query.prepare("SELECT pixiv_id, pixiv_index FROM picture_pixiv_ids WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.pixivIdIndices.insert({query.value(0).toULongLong(), query.value(1).toInt()});
        }
    }

    // 查询 Tweet 关联
    query.prepare("SELECT tweet_id, tweet_index FROM picture_tweet_ids WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(uint64_to_int64(id)));
    if (query.exec()) {
        while (query.next()) {
            info.tweetIdIndices.insert({query.value(0).toULongLong(), query.value(1).toInt()});
        }
    }

    return info;
}
TweetInfo PicDatabase::getTweetInfo(uint64_t tweetID) const {
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
    query.prepare("SELECT hashtag FROM tweet_hashtags WHERE tweet_id = :tweet_id");
    query.bindValue(":tweet_id", QVariant::fromValue(tweetID));
    if (query.exec()) {
        while (query.next()) {
            info.hashtags.insert(query.value(0).toString().toStdString());
        }
    }

    return info;
}
PixivInfo PicDatabase::getPixivInfo(uint64_t pixivID) const {
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
    query.prepare("SELECT tag FROM pixiv_artworks_tags WHERE pixiv_id = :pixiv_id");
    query.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
    if (query.exec()) {
        while (query.next()) {
            info.tags.push_back(query.value(0).toString().toStdString());
        }
    }

    // 查询 tagsTransl
    query.prepare("SELECT translated_tag FROM pixiv_tags WHERE tag IN (SELECT tag FROM pixiv_artworks_tags WHERE pixiv_id = :pixiv_id)");
    query.bindValue(":pixiv_id", QVariant::fromValue(pixivID));
    if (query.exec()) {
        while (query.next()) {
            info.tagsTransl.push_back(query.value(0).toString().toStdString());
        }
    }

    return info;
}
void PicDatabase::processSingleFile(const std::filesystem::path& path, ParserType parser) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    const std::string pathStr = path.string();
    try {
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".gif") {
            PicInfo picInfo = parsePicture(pathStr, parser);
            insertPicInfo(picInfo);
        } 
        else if (ext == ".txt" && parser == ParserType::Pixiv) {
            PixivInfo pixivInfo = parsePixivMetadata(pathStr);
            insertPixivInfo(pixivInfo);
        }
        else if (ext == ".json") {
            if (parser == ParserType::Pixiv) {
                PixivInfo pixivInfo = parsePixivJson(pathStr);
                updatePixivInfo(pixivInfo);
            } 
            else if (parser == ParserType::Twitter) {
                TweetInfo tweetInfo = parseTweetJson(pathStr);
                insertTweetInfo(tweetInfo);
            }
        }
        else if (ext == ".csv") {
            std::vector<PixivInfo> pixivInfoVec = parsePixivCsv(pathStr);
            for (const auto& pixivInfo : pixivInfoVec) {
                updatePixivInfo(pixivInfo);
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Error processing file:" << pathStr.c_str() << "Error:" << e.what();
    }
}
void PicDatabase::scanDirectory(const std::string& directory, ParserType parser){
    auto files = collectAllFiles(directory);
    qInfo() << "Collected" << files.size() << "files";
    const size_t BATCH_SIZE = 1000;
    size_t processed = 0;
    for (size_t i = 0; i < files.size(); i += BATCH_SIZE) {
        QSqlDatabase::database().transaction();
        auto batch_end = std::min(i + BATCH_SIZE, files.size());
        for (size_t j = i; j < batch_end; j++) {
            processSingleFile(files[j], parser);
            processed++;
            std::cout << "\rProcessed files: " << std::setw(8) << processed << std::flush;
            
        }
        if (!QSqlDatabase::database().commit()) {
            qWarning() << "Batch commit failed, rolling back";
            QSqlDatabase::database().rollback();
        }
    }
    qInfo() << "Scan completed. Total files processed:" << processed;
}