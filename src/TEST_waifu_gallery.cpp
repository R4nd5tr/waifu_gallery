#include "service/database.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QString>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    QLoggingCategory::setFilterRules("qt.gui.imageio=false\n"
                                     "qt.gui.icc=false");
    QCoreApplication app(argc, argv);
    PicDatabase picDatabase = PicDatabase(QString("test_conn"), QString("database.db"));
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/pixiv"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/twitter"), ParserType::Twitter);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv_csv"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv_json"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/pictures"));
    picDatabase.syncTables();
    return 0;
}
// #include <QSqlDatabase>
// #include <QSqlQuery>
// #include <QDebug>
// #include <QSqlError>

// int main(int argc, char *argv[]) {
//     QLoggingCategory::setFilterRules(
//         "qt.gui.imageio=false\n"
//         "qt.gui.icc=false"
//     );
//     QCoreApplication app(argc, argv);
//     QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//     db.setDatabaseName(":memory:"); // 使用内存数据库进行快速测试

//     if (!db.open()) {
//         qCritical() << "Could not open database:" << db.lastError();
//         return 1;
//     }

//     QSqlQuery query;

//     // 检查 SQLite 版本
//     if (query.exec("SELECT sqlite_version();") && query.next()) {
//         qDebug() << "SQLite Version:" << query.value(0).toString();
//     }

//     // 尝试创建 FTS5 虚拟表（这是目前最新、功能最强的版本）
//     if (query.exec("CREATE VIRTUAL TABLE IF NOT EXISTS test_fts5 USING fts5(content);")) {
//         qDebug() << "FTS5 support: YES";
//         query.exec("DROP TABLE test_fts5;"); // 清理
//     } else {
//         qDebug() << "FTS5 support: NO -" << query.lastError().text();
//     }

//     // 尝试创建 FTS4 虚拟表（较旧的版本）
//     if (query.exec("CREATE VIRTUAL TABLE IF NOT EXISTS test_fts4 USING fts4(content);")) {
//         qDebug() << "FTS4 support: YES";
//         query.exec("DROP TABLE test_fts4;"); // 清理
//     } else {
//         qDebug() << "FTS4 support: NO -" << query.lastError().text();
//     }

//     db.close();
//     return 0;
// }
// SQLite Version: "3.48.0"
// FTS5 support: YES
// FTS4 support: YES