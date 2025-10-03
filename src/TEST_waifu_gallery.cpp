#include "service/database.h"

int main(int argc, char* argv[]) {
    PicDatabase picDatabase = PicDatabase("test.db");
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/pixiv"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/twitter"), ParserType::Twitter);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv_csv"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Downloads/pixiv_json"), ParserType::Pixiv);
    picDatabase.importFilesFromDirectory(std::filesystem::path("C:/Users/Exusiai/Pictures/pictures"));
    picDatabase.syncTables();
    return 0;
}