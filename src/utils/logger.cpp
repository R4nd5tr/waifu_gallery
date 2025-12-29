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

#include "logger.h"

std::ofstream Logger::file_;
std::mutex Logger::mutex_;
bool Logger::useFile_ = false;

Logger::Logger(LogLevel level) : level_(level) {}

Logger::~Logger() {
    std::string msg = ss_.str();

#ifdef QT_CORE_LIB
    switch (level_) {
    case LogLevel::Debug:
        qDebug().noquote() << QString::fromStdString(msg);
        break;
    case LogLevel::Info:
        qInfo().noquote() << QString::fromStdString(msg);
        break;
    case LogLevel::Warning:
        qWarning().noquote() << QString::fromStdString(msg);
        break;
    case LogLevel::Error:
        qCritical().noquote() << QString::fromStdString(msg);
        break;
    }
#else
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostream* out = &std::cout;
    if (useFile_ && file_.is_open()) out = &file_;
    switch (level_) {
    case LogLevel::Debug:
        (*out) << "[DEBUG] ";
        break;
    case LogLevel::Info:
        (*out) << "[INFO] ";
        break;
    case LogLevel::Warning:
        (*out) << "[WARN] ";
        break;
    case LogLevel::Error:
        (*out) << "[ERROR] ";
        break;
    }
    (*out) << msg << std::endl;
#endif
}

void Logger::setLogFile(const std::filesystem::path& logFile) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(logFile, std::ios::app);
    useFile_ = file_.is_open();
}