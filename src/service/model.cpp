#include <string>
#include <algorithm>
#include "model.h"

std::string PicInfo::getFileType() const {
    for (const auto& path : filePaths) {
        auto pos = path.find_last_of('.');
        if (pos != std::string::npos && pos + 1 < path.size()) {
            std::string ext = path.substr(pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        }
    }
    return "";
}

float PicInfo::getRatio() const {
    if (height > 0) {
        return static_cast<float>(width) / static_cast<float>(height);
    }
    return 0.0f;
}