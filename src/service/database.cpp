#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QString>
#include <QFile>
#include "database.h"

PicDatabase::PicDatabase(QString databaseFile){
    initDatabase(databaseFile);
}

PicDatabase::~PicDatabase(){
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
            tweet_id INTEGER DEFAULT NULL,
            tweet_num INTEGER DEFAULT NULL,
            pixiv_id INTEGER DEFAULT NULL,
            pixiv_num INTEGER DEFAULT NULL,
            width INTEGER,
            height INTEGER,
            size INTEGER,
            x_restrict INTEGER DEFAULT NULL,

            FOREIGN KEY (tweet_id) REFERENCES tweets(tweet_id) ON DELETE SET NULL,
            FOREIGN KEY (pixiv_id) REFERENCES pixiv_artworks(pixiv_id) ON DELETE SET NULL
        )
    )";//xRestrict: 0 - all ages, 1 - R18, 2 - R18G TODO: duplicate pixiv id and tweet id
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
        )";
    const QString tweetsTable = R"(
        CREATE TABLE IF NOT EXISTS tweets (
            tweet_id INTEGER PRIMARY KEY NOT NULL,
            date DATE NOT NULL,
            author_id INTEGER NOT NULL,
            author_name TEXT NOT NULL,
            author_nick TEXT,
            author_description TEXT,
            favorite_count INTEGER DEFAULT 0,
            quote_count INTEGER DEFAULT 0,
            reply_count INTEGER DEFAULT 0,
            retweet_count INTEGER DEFAULT 0,
            bookmark_count INTEGER DEFAULT 0,
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
            x_restrict INTEGER
        )
    )";
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
        //tweet
        tweetsTable,
        tweetHashtagsTable,
        //pictures
        picturesTable, 
        pictureTagsTable,
        pictureFilesTable,
        pixivTagsTable
    };
    const QStringList indexes = {
        // foreign key indexes
        "CREATE INDEX IF NOT EXISTS idx_pictures_pixiv_id ON pictures(pixiv_id)",
        "CREATE INDEX IF NOT EXISTS idx_pictures_tweet_id ON pictures(tweet_id)",
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
            return false;
        }
    }
    for (const QString &indexSql : indexes) {
        if (!query.exec(indexSql)) {
            qCritical() << "Failed to create index:" << query.lastError().text();
            return false;
        }
    }
    database.commit();
    return true;
}
    
bool PicDatabase::insertPicInfo(const PicInfo& picInfo){
    if (!database.transaction()) {
        qCritical() << "Failed to begin transaction:" << database.lastError().text();
        return false;
    }

    if (!insertPicture(picInfo)) {
        database.rollback();
        qCritical() << "insertPicture failed, transaction rolled back.";
        return false;
    }

    if (!insertPictureFilePath(picInfo)) {
        database.rollback();
        qCritical() << "insertPictureFilePath failed, transaction rolled back.";
        return false;
    }

    if (!insertPictureTags(picInfo)) {
        database.rollback();
        qCritical() << "insertPictureTags failed, transaction rolled back.";
        return false;
    }

    if (!database.commit()) {
        qCritical() << "Failed to commit transaction:" << database.lastError().text();
        return false;
    }
    return true;
}

bool PicDatabase::insertPicture(const PicInfo& picInfo){
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO pictures(
            id, tweet_id, tweet_num, pixiv_id, pixiv_num,
            width, height, size, x_restrict
        ) VALUES (
            :id, :tweet_id, :tweet_num, :pixiv_id, :pixiv_num,
            :width, :height, :size, :x_restrict
        )
    )");
    query.bindValue(":id", QVariant::fromValue(picInfo.id));
    query.bindValue(":tweet_id", 
        picInfo.tweetID > 0 ? QVariant::fromValue(picInfo.tweetID) : QVariant()
    );
    query.bindValue(":tweet_num", picInfo.tweetNum);
    query.bindValue(":pixiv_id", 
        picInfo.pixivID > 0 ? QVariant::fromValue(picInfo.pixivID) : QVariant()
    );
    query.bindValue(":pixiv_num", picInfo.pixivNum);
    query.bindValue(":width", picInfo.width);
    query.bindValue(":height", picInfo.height);
    query.bindValue(":size", picInfo.size);
    query.bindValue(":x_restrict", static_cast<int>(picInfo.xRestrict));
    if (!query.exec()){
        qCritical() << "Failed to insert picture: " << query.lastError().text();
        return false;
    }
    return true;
}

bool PicDatabase::insertPictureFilePath(const PicInfo& picInfo){
    QSqlQuery query(database);
    for (const std::string& filePath : picInfo.filePaths) {
        query.prepare(R"(
            INSERT INTO picture_file_paths(
                id, file_path
            ) VALUES (
                :id, :file_path
            )
        )");
        query.bindValue(":id", QVariant::fromValue(picInfo.id));
        query.bindValue(":file_path", QString::fromStdString(filePath));
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_file_path: " << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool PicDatabase::insertPictureTags(const PicInfo& picInfo){
    QSqlQuery query(database);
    for (const std::pair<const std::string, char>& tagPair : picInfo.tags) {
        query.prepare(R"(
            INSERT INTO picture_tags(
                id, tag, source
            ) VALUES (
                :id, :tag, :source
            )
        )");
        query.bindValue(":id", QVariant::fromValue(picInfo.id));
        query.bindValue(":tag", QString::fromStdString(tagPair.first));
        query.bindValue(":source", static_cast<int>(tagPair.second));
        if (!query.exec()) {
            qCritical() << "Failed to insert picture_tag: " << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool PicDatabase::insertTweetInfo(const TweetInfo& tweetInfo) {
    if (!database.transaction()) {
        qCritical() << "Failed to begin transaction:" << database.lastError().text();
        return false;
    }
    if (!insertTweet(tweetInfo)) {
        database.rollback();
        qCritical() << "insertTweet failed, transaction rolled back.";
        return false;
    }
    if (!insertTweetHashtags(tweetInfo)) {
        database.rollback();
        qCritical() << "insertTweetHashtags failed, transaction rolled back.";
        return false;
    }
    if (!database.commit()) {
        qCritical() << "Failed to commit transaction:" << database.lastError().text();
        return false;
    }
    return true;
}

bool PicDatabase::insertTweet(const TweetInfo& tweetInfo) {
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO tweets(
            tweet_id, date, author_id, author_name, author_nick, author_description,
            favorite_count, quote_count, reply_count, retweet_count, bookmark_count, description
        ) VALUES (
            :tweet_id, :date, :author_id, :author_name, :author_nick, :author_description,
            :favorite_count, :quote_count, :reply_count, :retweet_count, :bookmark_count, :description
        )
    )");
    query.bindValue(":tweet_id", QVariant::fromValue(tweetInfo.tweetID));
    query.bindValue(":date", QString::fromStdString(tweetInfo.date));
    query.bindValue(":author_id", QVariant::fromValue(tweetInfo.authorID));
    query.bindValue(":author_name", QString::fromStdString(tweetInfo.authorName));
    query.bindValue(":author_nick", QString::fromStdString(tweetInfo.authorNick));
    query.bindValue(":author_description", QString::fromStdString(tweetInfo.authorDescription));
    query.bindValue(":favorite_count", QVariant::fromValue(tweetInfo.favoriteCount));
    query.bindValue(":quote_count", QVariant::fromValue(tweetInfo.quoteCount));
    query.bindValue(":reply_count", QVariant::fromValue(tweetInfo.replyCount));
    query.bindValue(":retweet_count", QVariant::fromValue(tweetInfo.retweetCount));
    query.bindValue(":bookmark_count", QVariant::fromValue(tweetInfo.bookmarkCount));
    query.bindValue(":description", QString::fromStdString(tweetInfo.description));
    if (!query.exec()) {
        qCritical() << "Failed to insert tweet: " << query.lastError().text();
        return false;
    }
    return true;
}

bool PicDatabase::insertTweetHashtags(const TweetInfo& tweetInfo) {
    QSqlQuery query(database);
    for (const std::string& hashtag : tweetInfo.hashtags) {
        query.prepare(R"(
            INSERT INTO tweet_hashtags(
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
    if (!database.transaction()) {
        qCritical() << "Failed to begin transaction:" << database.lastError().text();
        return false;
    }
    if (!insertPixivArtwork(pixivInfo)) {
        database.rollback();
        qCritical() << "insertPixivArtwork failed, transaction rolled back.";
        return false;
    }
    if (!insertPixivArtworkTags(pixivInfo)) {
        database.rollback();
        qCritical() << "insertPixivArtworkTags failed, transaction rolled back.";
        return false;
    }
    if (!database.commit()) {
        qCritical() << "Failed to commit transaction:" << database.lastError().text();
        return false;
    }
    return true;
}

bool PicDatabase::insertPixivArtwork(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO pixiv_artworks(
            pixiv_id, date, author_name, author_id, title, description,
            like_count, view_count, x_restrict
        ) VALUES (
            :pixiv_id, :date, :author_name, :author_id, :title, :description,
            :like_count, :view_count, :x_restrict
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
    if (!query.exec()) {
        qCritical() << "Failed to insert pixiv_artwork: " << query.lastError().text();
        return false;
    }
    return true;
}

bool PicDatabase::insertPixivArtworkTags(const PixivInfo& pixivInfo) {
    QSqlQuery query(database);
    for (const std::string& tag : pixivInfo.tags) {
        query.prepare(R"(
            INSERT INTO pixiv_artworks_tags(
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