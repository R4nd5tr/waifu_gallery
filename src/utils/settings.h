/*
 * Waifu Gallery - A Qt-based image gallery application.
 * Copyright (C) 2025 R4nd5tr <r4nd5tr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
