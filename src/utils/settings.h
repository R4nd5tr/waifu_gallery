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

#pragma once
#include "service/parser.h"
#include <filesystem>
#include <string>
#include <vector>

const std::string SETTINGS_FILE_PATH = "settings.json";

class Settings {
public:
    static void loadSettings();
    static void saveSettings();

    static void setWidthHeight(uint32_t width, uint32_t height) {
        windowWidth = width;
        windowHeight = height;
    }

    static bool settingsLoaded;

    static uint32_t windowWidth;
    static uint32_t windowHeight;
    static std::vector<std::pair<std::filesystem::path, ParserType>> picDirectories;
    static bool autoImportOnStartup;
    static bool autoTagAfterImport;
    static std::filesystem::path autoTaggerDLLPath;
};
