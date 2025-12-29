#pragma once
#include "external/autotagger/autotagger.h"
#include <string>
#include <windows.h>

using CreateFunc = AutoTagger* (*)();
using DestroyFunc = void (*)(AutoTagger*);

class AutoTaggerLoader { // DLL 动态加载器，与 AutoTagger 实例使用共同的生命周期
public:
    AutoTaggerLoader() = default;
    ~AutoTaggerLoader() { unload(); }
    std::vector<std::filesystem::path> discoverAutoTaggers() {
        std::vector<std::filesystem::path> taggerPaths;
        std::filesystem::path searchPath = "./model/";
        for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                taggerPaths.push_back(entry.path());
            }
        }
        return taggerPaths;
    };

    // 加载 DLL
    bool load(const std::filesystem::path& dllPath) {
        hDll_ = LoadLibraryW(dllPath.wstring().c_str());
        if (!hDll_) return false;

        // 获取函数指针
        createFunc_ = reinterpret_cast<CreateFunc>(GetProcAddress(hDll_, "createAutoTagger"));
        destroyFunc_ = reinterpret_cast<DestroyFunc>(GetProcAddress(hDll_, "destroyAutoTagger"));

        return createFunc_ && destroyFunc_;
    }

    // 卸载 DLL
    void unload() {
        if (taggerInstance_) {
            destroyFunc_(taggerInstance_);
            taggerInstance_ = nullptr;
        }
        if (hDll_) {
            FreeLibrary(hDll_);
            hDll_ = nullptr;
            createFunc_ = nullptr;
            destroyFunc_ = nullptr;
        }
    }

    // 创建标签器实例
    AutoTagger* getTagger() {
        if (taggerInstance_ == nullptr) {
            taggerInstance_ = createFunc_ ? createFunc_() : nullptr;
        }
        return taggerInstance_;
    }

    bool isLoaded() const { return hDll_ != nullptr; }

private:
    AutoTagger* taggerInstance_ = nullptr;
    HMODULE hDll_ = nullptr;
    CreateFunc createFunc_ = nullptr;
    DestroyFunc destroyFunc_ = nullptr;
};
