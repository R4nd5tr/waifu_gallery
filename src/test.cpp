#include "service/database.h"
#include <iostream>
#include <Qstring>
#include <QCoreApplication>

void databaseScanTest ();

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    databaseScanTest();

    return 0;
}

void databaseScanTest () {
    PicDatabase pixivDatabase = PicDatabase(QString("pixiv.db"));
    std::cout << "start scan pixiv pictures" << std::endl;
    pixivDatabase.scanDirectory("C:/Users/Exusiai/Pictures/pixiv", ParserType::Pixiv);
    std::cout << "pixiv pictures finished" << std::endl;

    PicDatabase twitterDatabase = PicDatabase(QString("twitter.db"));
    std::cout << "start scan twitter pictures" << std::endl;
    twitterDatabase.scanDirectory("C:/Users/Exusiai/Pictures/twitter", ParserType::Twitter);
    std::cout << "twitter pictures finished" << std::endl;

    PicDatabase pixivCsvDatabase = PicDatabase(QString("pixiv_csv.db"));
    std::cout << "start scan pixiv csv" << std::endl;
    pixivCsvDatabase.scanDirectory("C:/Users/Exusiai/Downloads/pixiv_csv", ParserType::Pixiv);
    std::cout << "pixiv csv finished" << std::endl;

    PicDatabase pixivJsonDatabase = PicDatabase(QString("pixiv_json.db"));
    std::cout << "start scan pixiv json" << std::endl;
    pixivJsonDatabase.scanDirectory("C:/Users/Exusiai/Downloads/pixiv_json", ParserType::Pixiv);
    std::cout << "pixiv json finished" << std::endl;

    PicDatabase commonImageDatabase = PicDatabase(QString("common_image.db"));
    std::cout << "start scan common images" << std::endl;
    commonImageDatabase.scanDirectory("C:/Users/Exusiai/Pictures/pictures");
    std::cout << "common images finished" << std::endl;
}
