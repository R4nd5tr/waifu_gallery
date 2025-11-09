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

class Logger { // TODO: implement log file rotation
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