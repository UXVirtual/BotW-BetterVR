#include "vkroots.h"

template <>
struct std::formatter<VkResult> : std::formatter<std::string> {
    auto format(VkResult result, std::format_context& ctx) {
        return std::formatter<string>::format(std::format("{} ({})", std::to_underlying(result), vkroots::helpers::enumString(result)), ctx);
    }
};

template <>
struct std::formatter<XrResult> : std::formatter<std::string> {
    auto format(XrResult result, std::format_context& ctx) {
        return std::formatter<string>::format(std::to_string(std::to_underlying(result)), ctx);
    }
};

template <>
struct std::formatter<VkFormat> : std::formatter<std::string> {
    auto format(VkFormat format, std::format_context& ctx) {
        return std::formatter<string>::format(std::format("{} ({})", std::to_underlying(format), vkroots::helpers::enumString(format)), ctx);
    }
};

template <>
struct std::formatter<DXGI_FORMAT> : std::formatter<std::string> {
    auto format(DXGI_FORMAT format, std::format_context& ctx) {
        return std::formatter<string>::format(std::format("{}", std::to_underlying(format)), ctx);
    }
};


template <>
struct std::formatter<D3D_FEATURE_LEVEL> : std::formatter<std::string> {
    auto format(D3D_FEATURE_LEVEL featureLevel, std::format_context& ctx) {
        switch (featureLevel) {
            case D3D_FEATURE_LEVEL_1_0_CORE:
                return std::formatter<string>::format(std::string("1.0"), ctx);
            case D3D_FEATURE_LEVEL_9_1:
                return std::formatter<string>::format(std::string("9.1"), ctx);
            case D3D_FEATURE_LEVEL_9_2:
                return std::formatter<string>::format(std::string("9.2"), ctx);
            case D3D_FEATURE_LEVEL_9_3:
                return std::formatter<string>::format(std::string("9.3"), ctx);
            case D3D_FEATURE_LEVEL_10_0:
                return std::formatter<string>::format(std::string("10.0"), ctx);
            case D3D_FEATURE_LEVEL_10_1:
                return std::formatter<string>::format(std::string("10.0"), ctx);
            case D3D_FEATURE_LEVEL_11_0:
                return std::formatter<string>::format(std::string("11.0"), ctx);
            case D3D_FEATURE_LEVEL_11_1:
                return std::formatter<string>::format(std::string("11.1"), ctx);
            case D3D_FEATURE_LEVEL_12_0:
                return std::formatter<string>::format(std::string("12.0"), ctx);
            case D3D_FEATURE_LEVEL_12_1:
                return std::formatter<string>::format(std::string("12.1"), ctx);
        }
        return std::formatter<string>::format(std::format("{:X}", std::to_underlying(featureLevel)), ctx);
    }
};

class Log {
public:
    Log();
    ~Log();

    static void print(const char* message);

    template <class... Args>
    static void print(const char* format, Args&&... args) {
        Log::print(std::vformat(format, std::make_format_args(args...)).c_str());
    }

    static void printTimeElapsed(const char* message_prefix, LARGE_INTEGER time);
};

static void checkXRResult(XrResult result, const char* errorMessage) {
    XrResult resultCheck = result;
    if (XR_FAILED(result)) {
        if (errorMessage == nullptr) {
            Log::print("[Error] An unknown error (result was {}) has occurred!", result);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, std::format("An unknown error {} has occurred which caused a fatal crash!", result).c_str(), "An error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error("Undescribed error occurred!");
        }
        else {
            Log::print("[Error] Error {}: {}", result, errorMessage);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, errorMessage, "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error(errorMessage);
        }
    }
}

static void checkHResult(HRESULT result, const char* errorMessage) {
    if (FAILED(result)) {
        if (errorMessage == nullptr) {
            Log::print("[Error] An unknown error (result was {}) has occurred!", result);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, std::format("An unknown error {} has occurred which caused a fatal crash!", result).c_str(), "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error("Undescribed error occurred!");
        }
        else {
            Log::print("[Error] Error {}: {}", result, errorMessage);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, errorMessage, "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error(errorMessage);
        }
    }
}

static void checkVkResult(VkResult result, const char* errorMessage) {
    if (result != VK_SUCCESS) {
        if (errorMessage == nullptr) {
            Log::print("[Error] An unknown error (result was {}) has occurred!", (std::underlying_type_t<VkResult>)result);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, std::format("An unknown error {} has occurred which caused a fatal crash!", (std::underlying_type_t<VkResult>)result).c_str(), "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error("Undescribed error occurred!");
        }
        else {
            Log::print("[Error] Error {}: {}", (std::underlying_type_t<VkResult>)result, errorMessage);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, errorMessage, "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error(errorMessage);
        }
    }
}

static void checkAssert(bool assert, const char* errorMessage) {
    if (!assert) {
        if (errorMessage == nullptr) {
            Log::print("[Error] Something unexpected happened that prevents further execution!");
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, "Something unexpected happened that prevents further execution!", "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error("Unexpected assertion occurred!");
        }
        else {
            Log::print("[Error] {}", errorMessage);
#ifdef _DEBUG
            __debugbreak();
#endif
            MessageBoxA(NULL, errorMessage, "A fatal error occurred!", MB_OK | MB_ICONERROR);
            throw std::runtime_error(errorMessage);
        }
    }
}