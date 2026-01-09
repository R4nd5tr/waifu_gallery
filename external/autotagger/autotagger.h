#pragma once
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#ifdef AUTOTAGGER_EXPORTS
#define AUTOTAGGER __declspec(dllexport)
#else
#define AUTOTAGGER __declspec(dllimport)
#endif
#else
#define AUTOTAGGER
#endif

typedef void (*LogCallback)(const std::string& message);

enum class ModelRestrictType { Unknown, General, Sensitive, Questionable, Explicit };

struct ImageTagResult {
    std::vector<int> tagIndexes;
    std::vector<float> tagProbabilities;
    ModelRestrictType restrictType;
    std::vector<uint8_t> featureHash; // 512 bits
};

struct PredictResult {
    std::vector<float> tagProbabilities;
    std::vector<uint8_t> featureHash;
};

class AutoTagger {
public:
    AutoTagger() = default;
    virtual ~AutoTagger() = default;

    // image analysis function single thread synchronous call
    virtual ImageTagResult analyzeImage(const std::filesystem::path& imagePath) = 0;

    // fine-grained processing functions for better control
    virtual std::vector<float> preprocess(const std::filesystem::path& imagePath) = 0;
    virtual PredictResult predict(const std::vector<float>& inputTensorVec) = 0; // do not call this concurrently
    virtual ImageTagResult postprocess(PredictResult& outputTensorVec) = 0;

    // model info functions
    virtual std::vector<std::pair<std::string, bool>> getTagSet() = 0; // pair<tag, is_character_tag> tags are in index order
    virtual std::string getModelName() = 0;

    // system info functions
    virtual bool gpuAvailable() = 0;
    virtual void setLogCallback(LogCallback callback) = 0;
};

extern "C" {
AUTOTAGGER AutoTagger* createAutoTagger();
AUTOTAGGER void destroyAutoTagger(AutoTagger* ptr);
}
