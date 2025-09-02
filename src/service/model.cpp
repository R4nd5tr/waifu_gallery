#include <string>
#include <algorithm>
#include "model.h"

float PicInfo::getRatio() const {
    if (height > 0) {
        return static_cast<float>(width) / static_cast<float>(height);
    }
    return 0.0f;
}