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
std::vector<std::pair<std::filesystem::path, ParserType>> Settings::picDirectories;
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
            picDirectories.clear();
            if (j.contains("picDirectories") && j["picDirectories"].is_array()) {
                for (const auto& item : j["picDirectories"]) {
                    if (item.is_array() && item.size() == 2) {
                        std::filesystem::path dirPath = item[0].get<std::string>();
                        ParserType parserType = static_cast<ParserType>(item[1].get<int>());
                        picDirectories.emplace_back(dirPath, parserType);
                    }
                }
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
        j["picDirectories"] = json::array();
        for (const auto& pair : picDirectories) {
            j["picDirectories"].push_back({pair.first.string(), static_cast<int>(pair.second)});
        }
        j["autoImportOnStartup"] = autoImportOnStartup;

        std::ofstream outFile(SETTINGS_FILE_PATH);
        outFile << j.dump(4);
    } catch (const std::exception& e) {
        Error() << "Error saving settings: " << e.what();
    }
    Info() << "Settings saved.";
}