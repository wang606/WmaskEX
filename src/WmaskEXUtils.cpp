#include "header.h"

void to_json(json& j, const WmaskEXConfig& c) {
    j = json{
        {"active", c.active},
        {"name", c.name},
        {"exePath", c.exePath},
        {"assetsPath", c.assetsPath},
        {"previewPath", c.previewPath},
        {"sizeType", static_cast<int>(c.sizeType)},
        {"scale", c.scale},
        {"horizontal", c.horizontal},
        {"xShift", c.xShift},
        {"vertical", c.vertical},
        {"yShift", c.yShift},
        {"duration", c.duration},
        {"opacity", c.opacity}
    };
}

void from_json(const json& j, WmaskEXConfig& c) {
    j.at("active").get_to(c.active);
    j.at("name").get_to(c.name);
    j.at("exePath").get_to(c.exePath);
    j.at("assetsPath").get_to(c.assetsPath);
    j.at("previewPath").get_to(c.previewPath);
    c.sizeType = static_cast<WmaskEXConfig::SizeType>(j.at("sizeType").get<int>());
    j.at("scale").get_to(c.scale);
    j.at("horizontal").get_to(c.horizontal);
    j.at("xShift").get_to(c.xShift);
    j.at("vertical").get_to(c.vertical);
    j.at("yShift").get_to(c.yShift);
    j.at("duration").get_to(c.duration);
    j.at("opacity").get_to(c.opacity);
}

float getRandomFloat() {
    static std::random_device rd; 
    static std::mt19937 gen(rd()); 
    static std::uniform_real_distribution<float> dis(0.0f, std::nextafter(1.0f, 0.0f)); 
    return dis(gen);
}

float getCurrentTimeInSeconds() {
    return std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

bool isValidWmaskEXParentWindow(HWND hwnd) {
    return IsWindow(hwnd) && IsWindowVisible(hwnd) && IsWindowEnabled(hwnd) &&
        !(GetWindowLongPtr(hwnd, GWL_STYLE) & (WS_POPUP | WS_CHILD)); 
}

bool getRandomAsset(const std::wstring& assetsPath, WmaskEXAssetConfig& assetConfig) {
    fs::path assetsDir(assetsPath);
    if (!fs::exists(assetsDir) || !fs::is_directory(assetsDir)) {
        MessageBox(NULL, L"Assets path does not exist or is not a directory.", L"Error", MB_ICONERROR | MB_OK);
        return false;
    }
    std::vector<WmaskEXAssetConfig> assetConfigs;
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir)) {
        WmaskEXAssetConfig _assetConfig;
        if (entry.is_directory()) {
            auto stem = entry.path().stem();
            fs::path atlasFile = entry.path() / (stem.wstring() + L".atlas");
            fs::path jsonFile = entry.path() / (stem.wstring() + L".json");
            fs::path skelFile = entry.path() / (stem.wstring() + L".skel");
            fs::path wmaskexJsonFile = entry.path() / (stem.wstring() + L".wmaskex.json");
            fs::path wmaskexAutogenJsonFile = entry.path() / (stem.wstring() + L".wmaskex.autogen.json");
            if (fs::exists(atlasFile) && (fs::exists(jsonFile) || fs::exists(skelFile)) &&
                (fs::exists(wmaskexJsonFile) || fs::exists(wmaskexAutogenJsonFile))) {
                _assetConfig.type = WmaskEXAssetConfig::AssetType::AT_Spine;
                _assetConfig.assetPath = entry.path().wstring();
                fs::path metaFile = fs::exists(wmaskexJsonFile) ? wmaskexJsonFile : wmaskexAutogenJsonFile;
                try {
                    std::ifstream metaStream(metaFile);
                    json metaJson = json::parse(metaStream);
                    metaStream.close();
                    if (metaJson.contains("version")) {
                        std::string versionStr = metaJson["version"].get<std::string>();
                        auto it = std::find(validSpineVersions.begin(), validSpineVersions.end(), versionStr);
                        if (it != validSpineVersions.end())
                            _assetConfig.spineVersion = static_cast<WmaskEXAssetConfig::SpineVersion>(std::distance(validSpineVersions.begin(), it));
                        else
                            _assetConfig.spineVersion = WmaskEXAssetConfig::SpineVersion::SV_Invalid;
                    }
                    if (metaJson.contains("bounds") && metaJson["bounds"].is_array() && metaJson["bounds"].size() == 4) {
                        _assetConfig.bounds.x = metaJson["bounds"][0].get<float>();
                        _assetConfig.bounds.y = metaJson["bounds"][1].get<float>();
                        _assetConfig.bounds.width = metaJson["bounds"][2].get<float>();
                        _assetConfig.bounds.height = metaJson["bounds"][3].get<float>();
                    }
                    if (metaJson.contains("pma"))
                        _assetConfig.pma = metaJson["pma"].get<bool>();
                } catch (...) {}
                assetConfigs.push_back(_assetConfig);
            }
        }
    }   
    for (const auto& entry : fs::directory_iterator(assetsDir)) {
        WmaskEXAssetConfig _assetConfig;
        if (entry.is_regular_file()) {
            std::wstring ext = entry.path().extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
            if (validImageExtensions.contains(ext)) {
                _assetConfig.type = WmaskEXAssetConfig::AssetType::AT_Image;
                _assetConfig.assetPath = entry.path().wstring();
                assetConfigs.push_back(_assetConfig);
            }
        }
    }
    if (assetConfigs.empty()) return false;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, assetConfigs.size() - 1);
    assetConfig = assetConfigs[dis(gen)];
    return true;
}

bool openConfig(const std::wstring& configFilePath, std::map<std::wstring, WmaskEXConfig>& configs) {
    configs.clear();
    try {
        std::ifstream file(configFilePath);
        if (file.good()) {
            json j = json::parse(file);
            file.close(); 
            for (WmaskEXConfig c: j)
                configs[c.name] = c; 
        }
        return true; 
    } catch (...) {
        MessageBox(NULL, L"Invalid config file!", L"Warn", MB_ICONWARNING | MB_OK);
        return false; 
    }
}

bool saveConfig(const std::wstring& configFilePath, const std::map<std::wstring, WmaskEXConfig>& configs) {
    try {
        json j = json::array();
        for (const auto& [name, config] : configs) {
            j.push_back(config);
        }
        std::ofstream file(configFilePath);
        if (file.good()) {
            file << j.dump(4); // Pretty print with 4 spaces
            file.close();
        }
        return true;
    } catch (...) {
        MessageBox(NULL, L"Failed to save config file!", L"Error", MB_ICONERROR | MB_OK);
        return false;
    }
}