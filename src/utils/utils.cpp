#include "utils.h"

std::vector<std::filesystem::path> collectFiles(const std::filesystem::path& directory) {
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