#include "service/database.h"
#include <iostream>
#include <Qstring>
#include <filesystem>
#include <QCoreApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
    QLoggingCategory::setFilterRules(
        "qt.gui.imageio=false\n"
        "qt.gui.icc=false"
    );
    QCoreApplication app(argc, argv);
    PicDatabase picDatabase = PicDatabase(QString("pictures.db"));
    picDatabase.scanDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv"));
    picDatabase.scanDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/pixiv"));
    // picDatabase.scanDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/twitter"));
    // picDatabase.scanDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv_csv"));
    // picDatabase.scanDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv_json"));
    picDatabase.scanDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/pictures"));
    return 0;
}