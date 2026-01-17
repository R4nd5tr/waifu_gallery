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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#ifdef QT_CORE_LIB
#include <QDebug>
#endif

enum class LogLevel { Debug, Info, Warning, Error };

class Logger {
public:
    Logger(LogLevel level);
    ~Logger();

    template <typename T> Logger& operator<<(const T& value) {
        ss_ << value;
        return *this;
    }

#ifdef QT_CORE_LIB
    Logger& operator<<(const QString& value) {
        ss_ << value.toStdString();
        return *this;
    }
    Logger& operator<<(const QByteArray& value) {
        ss_ << value.toStdString();
        return *this;
    }
#endif

    // 支持链式调用 std::endl
    Logger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        ss_ << manip;
        return *this;
    }

    static void setLogFile(const std::filesystem::path& logFile);

private:
    LogLevel level_;
    std::ostringstream ss_;
    static std::ofstream file_;
    static std::mutex mutex_;
    static bool useFile_;
};

// 便捷宏
#define Debug() Logger(LogLevel::Debug)
#define Info() Logger(LogLevel::Info)
#define Warn() Logger(LogLevel::Warning)
#define Error() Logger(LogLevel::Error)