#include<QtSql/QSqlDatabase>
#include<QtSql/QSqlError>
#include<QtSql/QSqlQuery>
#include<QtSql/QSqlRecord>
#include<QString>
#include<QFile>
#include"database.h"

PicDatabase::PicDatabase(QString databaseFile = "database.db"){
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
}

bool PicDatabase::createTables() {
    QSqlQuery query;
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
    )";//xRestrict: 0 - all ages, 1 - R18, 2 - R18G 
    const QString pictureTagsTable = R"(
        CREATE TABLE IF NOT EXISTS picture_tags (
            id INTEGER NOT NULL,
            tag TEXT NOT NULL,
            source INTEGER NOT NULL,
            PRIMARY KEY (picture_id, tag),

            FOREIGN KEY (id) REFERENCES pictures(id) ON DELETE CASCADE
        )
    )";//source: 0 - pixiv, 1 - completed tags, 2 - ai predicted tags
    const QString pictureDirectoriesTable = R"(
        CREATE TABLE IF NOT EXISTS picture_directories (
            id INTEGER NOT NULL,
            directory TEXT NOT NULL,
            PRIMARY KEY (id, directory),

            FOREIGN KEY (id) REFERENCES picture(id) ON DELETE CASCADE
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
        pictureDirectoriesTable,
        pixivTagsTable
    };
    const QStringList indexes = {
        //id indexes
        "CREATE INDEX IF NOT EXISTS idx_pictures_id ON pictures(id)",
        "CREATE INDEX IF NOT EXISTS idx_tweets_id ON tweets(tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_id ON pixiv_artworks(pixiv_id)",
        // foreign key indexes
        "CREATE INDEX IF NOT EXISTS idx_pictures_pixiv_id ON pictures(pixiv_id)",
        "CREATE INDEX IF NOT EXISTS idx_pictures_tweet_id ON pictures(tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_id ON picture_tags(id)",
        "CREATE INDEX IF NOT EXISTS idx_picture_directories_id ON picture_directories(id)",
        "CREATE INDEX IF NOT EXISTS idx_tweet_hashtags_id ON tweet_hashtags(tweet_id)",
        "CREATE INDEX IF NOT EXISTS idx_pixiv_artworks_tags_id ON tweet_hashtags(tweet_id)",
        // tags indexes
        "CREATE INDEX IF NOT EXISTS idx_picture_tags_tag ON picture_tags(tag)",
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
    
bool PicDatabase::insertPicInfo(PicInfo picInfo){
    
}