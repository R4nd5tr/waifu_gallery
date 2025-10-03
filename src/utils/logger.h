#pragma once
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

    // 支持链式调用 std::endl
    Logger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        ss_ << manip;
        return *this;
    }

    static void setLogFile(const std::string& filename);

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