#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct Settings {
    int windowWidth = 800;
    int windowHeight = 600;
    std::vector<std::filesystem::path> picDirectories;
    std::vector<std::filesystem::path> pixivDirectories;
    std::vector<std::filesystem::path> tweetDirectories;
    bool autoScanOnStartup = false;
    bool copyImage = false;
    bool openImage = false;
    bool copyImagePath = false;
    bool openImagePath = false;
};

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();
    void loadSettings();
    void saveSettings();

private:
};
