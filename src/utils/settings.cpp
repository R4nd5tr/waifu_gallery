/*
 * Waifu Gallery - A anime illustration gallery application.
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

#include "settings.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool Settings::settingsLoaded = false;

uint32_t Settings::windowWidth = 1200;
uint32_t Settings::windowHeight = 800;
std::vector<std::filesystem::path> Settings::picDirectories;
std::vector<std::filesystem::path> Settings::pixivDirectories;
std::vector<std::filesystem::path> Settings::tweetDirectories;
bool Settings::autoImportOnStartup = false;

void Settings::loadSettings() {
    if (settingsLoaded) {
        Info() << "Settings already loaded.";
        return;
    }
    try {
        if (std::filesystem::exists(SETTINGS_FILE_PATH)) {
            std::ifstream inFile(SETTINGS_FILE_PATH);
            json j;
            inFile >> j;
            windowWidth = j.value("windowWidth", 800);
            windowHeight = j.value("windowHeight", 600);
            {
                auto tmp = j.value("picDirectories", std::vector<std::string>{});
                picDirectories.clear();
                for (const auto& s : tmp)
                    picDirectories.emplace_back(s);
            }
            {
                auto tmp = j.value("pixivDirectories", std::vector<std::string>{});
                pixivDirectories.clear();
                for (const auto& s : tmp)
                    pixivDirectories.emplace_back(s);
            }
            {
                auto tmp = j.value("tweetDirectories", std::vector<std::string>{});
                tweetDirectories.clear();
                for (const auto& s : tmp)
                    tweetDirectories.emplace_back(s);
            }
            autoImportOnStartup = j.value("autoImportOnStartup", false);
        } else {
            Info() << "Settings file not found. Using default settings.";
        }
    } catch (const std::exception& e) {
        Error() << "Error loading settings: " << e.what();
    }
    settingsLoaded = true;
    Info() << "Settings loaded: " << "Window Size(" << windowWidth << "x" << windowHeight << "), "
           << "Auto Scan On Startup(" << (autoImportOnStartup ? "true" : "false") << ")";
}
void Settings::saveSettings() {
    if (!settingsLoaded) {
        Info() << "Settings not loaded. Skipping save.";
        return;
    }
    try {
        json j;
        j["windowWidth"] = windowWidth;
        j["windowHeight"] = windowHeight;
        {
            std::vector<std::string> tmp;
            for (const auto& path : picDirectories)
                tmp.push_back(path.string());
            j["picDirectories"] = tmp;
        }
        {
            std::vector<std::string> tmp;
            for (const auto& path : pixivDirectories)
                tmp.push_back(path.string());
            j["pixivDirectories"] = tmp;
        }
        {
            std::vector<std::string> tmp;
            for (const auto& path : tweetDirectories)
                tmp.push_back(path.string());
            j["tweetDirectories"] = tmp;
        }
        j["autoImportOnStartup"] = autoImportOnStartup;

        std::ofstream outFile(SETTINGS_FILE_PATH);
        outFile << j.dump(4);
    } catch (const std::exception& e) {
        Error() << "Error saving settings: " << e.what();
    }
    Info() << "Settings saved.";
}