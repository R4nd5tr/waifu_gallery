#include "logger.h"

std::ofstream Logger::file_;
std::mutex Logger::mutex_;
bool Logger::useFile_ = false;

Logger::Logger(LogLevel level) : level_(level) {}

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
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