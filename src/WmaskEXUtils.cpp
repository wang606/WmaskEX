#include "header.h"

std::wofstream WmaskEXLog::logFile; 
bool WmaskEXLog::initialized = false; 

void WmaskEXLog::init(const std::wstring& logFilePath) {
    if (!initialized) {
        logFile.open(logFilePath, std::ios::app); 
        logFile.imbue(std::locale("")); 
        initialized = true; 
    }
}

void WmaskEXLog::log(const std::wstring& message) {
    if (!initialized) init(); 
    auto now = std::chrono::system_clock::now(); 
    auto time_t = std::chrono::system_clock::to_time_t(now); 
    std::wstringstream ss; 
    ss << std::put_time(std::localtime(&time_t), L"%H:%M:%S") << L" - " << message << std::endl;
    if (logFile.is_open()) {
        logFile << ss.str(); 
        logFile.flush(); 
    }
    OutputDebugString(ss.str().c_str());
}

void WmaskEXLog::close() {
    if (logFile.is_open()) {
        logFile.close(); 
    }
}

// ========== Spine资源直接解析函数 ========== //
struct ParsedSkeletonInfo {
    std::string version;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool valid = false;
};

ParsedSkeletonInfo parseJsonSkeleton(const std::wstring& jsonPath) {
    ParsedSkeletonInfo info;
    try {
        std::ifstream ifs(jsonPath);
        if (!ifs) return info;
        
        // 读取文件前2KB来查找skeleton部分
        const size_t bufferSize = 2048;
        std::string buffer(bufferSize, '\0');
        ifs.read(&buffer[0], bufferSize);
        size_t bytesRead = ifs.gcount();
        buffer.resize(bytesRead);
        
        // 查找skeleton对象
        size_t skeletonPos = buffer.find("\"skeleton\"");
        if (skeletonPos == std::string::npos) return info;
        
        // 查找skeleton对象开始的 '{'
        size_t bracePos = buffer.find('{', skeletonPos);
        if (bracePos == std::string::npos) return info;
        
        size_t skeletonEnd = buffer.find('}', bracePos);
        if (skeletonEnd == std::string::npos) skeletonEnd = buffer.length();
        
        // 解析字符串字段的lambda
        auto parseStringField = [&](const std::string& fieldName) -> std::string {
            size_t fieldPos = buffer.find("\"" + fieldName + "\"", bracePos);
            if (fieldPos != std::string::npos && fieldPos < skeletonEnd) {
                size_t colonPos = buffer.find(':', fieldPos);
                if (colonPos != std::string::npos) {
                    size_t quoteStart = buffer.find('"', colonPos);
                    size_t quoteEnd = buffer.find('"', quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        return buffer.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    }
                }
            }
            return "";
        };
        
        // 解析数字字段的lambda
        auto parseFloatField = [&](const std::string& fieldName) -> float {
            size_t fieldPos = buffer.find("\"" + fieldName + "\"", bracePos);
            if (fieldPos != std::string::npos && fieldPos < skeletonEnd) {
                size_t colonPos = buffer.find(':', fieldPos);
                if (colonPos != std::string::npos) {
                    size_t numStart = colonPos + 1;
                    while (numStart < buffer.length() && (buffer[numStart] == ' ' || buffer[numStart] == '\t')) numStart++;
                    size_t numEnd = numStart;
                    while (numEnd < buffer.length() && (std::isdigit(buffer[numEnd]) || buffer[numEnd] == '.' || buffer[numEnd] == '-')) numEnd++;
                    if (numEnd > numStart) {
                        return std::stof(buffer.substr(numStart, numEnd - numStart));
                    }
                }
            }
            return 0.0f;
        };
        
        // 解析所有字段
        info.version = parseStringField("spine");
        if (info.version.empty()) return info;
        
        info.x = parseFloatField("x");
        info.y = parseFloatField("y");
        info.width = parseFloatField("width");
        info.height = parseFloatField("height");
        
        info.valid = true;
    } catch (...) {}
    return info;
}

ParsedSkeletonInfo parseSkelSkeleton(const std::wstring& skelPath) {
    ParsedSkeletonInfo info;
    try {
        std::ifstream ifs(skelPath, std::ios::binary);
        if (!ifs) return info;
        
        auto readVarint = [](std::istream& s) -> int {
            int result = 0;
            int shift = 0;
            while (true) {
                char b;
                s.read(&b, 1);
                result |= (b & 0x7F) << shift;
                if (!(b & 0x80)) break;
                shift += 7;
            }
            return result;
        };
        auto readString = [&](std::istream& s) -> std::string {
            int len = readVarint(s);
            if (len == 0) return std::string();
            std::string str(len - 1, '\0');
            s.read(&str[0], len - 1);
            return str;
        };
        auto readInt = [](std::istream& s) -> int {
            int result = 0;
            char b;
            s.read(&b, 1); result = (uint8_t)b;
            result <<= 8;
            s.read(&b, 1); result |= (uint8_t)b;
            result <<= 8;
            s.read(&b, 1); result |= (uint8_t)b;
            result <<= 8;
            s.read(&b, 1); result |= (uint8_t)b;
            return result;
        };
        auto readFloat = [&](std::istream& s) -> float {
            union {
                int intValue;
                float floatValue;
            } intToFloat;
            intToFloat.intValue = readInt(s);
            return intToFloat.floatValue;
        };
        
        // 先读取前64字节来检测版本
        std::streampos startPos = ifs.tellg();
        char buffer[64];
        ifs.read(buffer, 64);
        ifs.seekg(startPos); // 重置到开始位置
        
        std::string bufferStr(buffer, 64);
        bool isNewFormat = false;
        
        // 在缓冲区中查找版本字符串
        if (bufferStr.find("4.2.") != std::string::npos ||
            bufferStr.find("4.1.") != std::string::npos ||
            bufferStr.find("4.0.") != std::string::npos) {
            isNewFormat = true;
        } else if (bufferStr.find("3.8.") != std::string::npos ||
                   bufferStr.find("3.7.") != std::string::npos) {
            isNewFormat = false;
        } else {
            // 如果没找到明确版本标识，尝试新格式
            isNewFormat = true;
        }
        
        if (isNewFormat) {
            // 新格式（4.0+）：先读两个int作为hash
            readInt(ifs); // lowHash
            readInt(ifs); // highHash
            info.version = readString(ifs);
        } else {
            // 旧格式（3.7, 3.8）：直接读取hash字符串
            readString(ifs); // hash
            info.version = readString(ifs); // version
        }
        
        // 根据版本读取bounds
        std::string v = info.version.substr(0, 3);
        if (v == "3.7") {
            // 3.7版本没有x,y字段
            info.x = 0.0f;
            info.y = 0.0f;
            info.width = readFloat(ifs);
            info.height = readFloat(ifs);
        } else {
            // 3.8+ 版本有x,y,width,height
            info.x = readFloat(ifs);
            info.y = readFloat(ifs);
            info.width = readFloat(ifs);
            info.height = readFloat(ifs);
        }
        
        info.valid = true;
    } catch (...) {}
    return info;
}

bool parseAtlasPMA(const std::wstring& atlasPath) {
    try {
        std::ifstream ifs(atlasPath);
        if (!ifs) return false;
        std::string line;
        int lineCount = 0;
        while (std::getline(ifs, line) && lineCount < 10) {
            lineCount++;
            if (line.find("pma") != std::string::npos) {
                if (line.find("true") != std::string::npos) return true;
                else return false;
            }
        }
        return false; // 前10行没有找到pma字段，默认false
    } catch (...) { return false; }
}

WmaskEXAssetConfig::SpineVersion getSpineVersionFromString(const std::string& versionStr) {
    if (versionStr.starts_with("3.7")) return WmaskEXAssetConfig::SpineVersion::SV_37;
    if (versionStr.starts_with("3.8")) return WmaskEXAssetConfig::SpineVersion::SV_38;
    if (versionStr.starts_with("4.0")) return WmaskEXAssetConfig::SpineVersion::SV_40;
    if (versionStr.starts_with("4.1")) return WmaskEXAssetConfig::SpineVersion::SV_41;
    if (versionStr.starts_with("4.2")) return WmaskEXAssetConfig::SpineVersion::SV_42;
    return WmaskEXAssetConfig::SpineVersion::SV_Invalid; 
}

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
        LOG(L"ERROR: Assets path invalid: " + assetsPath);
        return false;
    }
    
    struct SpineAssetPaths {
        fs::path atlasFile;
        fs::path jsonFile;
        fs::path skelFile;
        fs::path wmaskexJsonFile;
    };
    
    std::vector<SpineAssetPaths> spineAssets;
    std::vector<fs::path> imageAssets;

    // 扫描 Spine 资源：以 .atlas 文件为基准，只收集路径
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir)) {
        if (entry.is_regular_file()) {
            fs::path filePath = entry.path();
            
            // 检查是否是 .atlas 文件
            if (filePath.extension() == L".atlas") {
                auto stem = filePath.stem();
                auto dir = filePath.parent_path();
                fs::path jsonFile = dir / (stem.wstring() + L".json");
                fs::path skelFile = dir / (stem.wstring() + L".skel");
                fs::path wmaskexJsonFile = dir / (stem.wstring() + L".wmaskex.json");
                
                // 检查同名的 .json 或 .skel 文件是否存在
                if (fs::exists(jsonFile) || fs::exists(skelFile)) {
                    spineAssets.push_back({filePath, jsonFile, skelFile, wmaskexJsonFile});
                }
            }
        }
    }

    // 扫描图片资源：只收集路径
    for (const auto& entry : fs::directory_iterator(assetsDir)) {
        if (entry.is_regular_file()) {
            fs::path filePath = entry.path();
            
            std::wstring ext = filePath.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
            if (validImageExtensions.contains(ext)) {
                imageAssets.push_back(filePath);
            }
        }
    }
    
    // 计算总资源数量
    size_t totalAssets = spineAssets.size() + imageAssets.size();
    if (totalAssets == 0) {
        return false;
    }
    
    // 随机选择一个资源
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, totalAssets - 1);
    size_t selectedIndex = dis(gen);
    
    // 如果选中的是Spine资源
    if (selectedIndex < spineAssets.size()) {
        const auto& spineAsset = spineAssets[selectedIndex];
        
        assetConfig.type = WmaskEXAssetConfig::AssetType::AT_Spine;
        assetConfig.assetPath = spineAsset.atlasFile.wstring();
        
        bool parseSuccess = false;
        
        // 优先读取 .wmaskex.json 文件
        if (fs::exists(spineAsset.wmaskexJsonFile)) {
            try {
                std::ifstream metaStream(spineAsset.wmaskexJsonFile);
                json metaJson = json::parse(metaStream);
                metaStream.close();
                
                if (metaJson.contains("version")) {
                    std::string versionStr = metaJson["version"].get<std::string>();
                    assetConfig.spineVersion = getSpineVersionFromString(versionStr);
                }
                if (metaJson.contains("bounds") && metaJson["bounds"].is_array() && metaJson["bounds"].size() == 4) {
                    assetConfig.bounds.x = metaJson["bounds"][0].get<float>();
                    assetConfig.bounds.y = metaJson["bounds"][1].get<float>();
                    assetConfig.bounds.width = metaJson["bounds"][2].get<float>();
                    assetConfig.bounds.height = metaJson["bounds"][3].get<float>();
                }
                if (metaJson.contains("pma"))
                    assetConfig.pma = metaJson["pma"].get<bool>();
                
                if (assetConfig.spineVersion != WmaskEXAssetConfig::SpineVersion::SV_Invalid) {
                    parseSuccess = true;
                }
            } catch (...) {
                // 如果 .wmaskex.json 解析失败，继续用直接解析
            }
        }
        
        // 如果没有 .wmaskex.json 或解析失败，直接解析文件
        if (!parseSuccess) {
            ParsedSkeletonInfo info;
            if (fs::exists(spineAsset.skelFile)) {
                info = parseSkelSkeleton(spineAsset.skelFile.wstring());
            } else if (fs::exists(spineAsset.jsonFile)) {
                info = parseJsonSkeleton(spineAsset.jsonFile.wstring());
            }
            
            if (info.valid) {
                assetConfig.spineVersion = getSpineVersionFromString(info.version);
                assetConfig.bounds.x = info.x;
                assetConfig.bounds.y = info.y;
                assetConfig.bounds.width = info.width;
                assetConfig.bounds.height = info.height;
                assetConfig.pma = parseAtlasPMA(spineAsset.atlasFile.wstring());
                parseSuccess = true;
            }
        }
        
        // 如果解析失败，返回false
        if (!parseSuccess || assetConfig.spineVersion == WmaskEXAssetConfig::SpineVersion::SV_Invalid) {
            return false;
        }
        
        return true;
    }
    // 如果选中的是图片资源
    else {
        size_t imageIndex = selectedIndex - spineAssets.size();
        assetConfig.type = WmaskEXAssetConfig::AssetType::AT_Image;
        assetConfig.assetPath = imageAssets[imageIndex].wstring();
        return true;
    }
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
        LOG(L"ERROR: Invalid config file: " + configFilePath);
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
        LOG(L"ERROR: Failed to save config: " + configFilePath);
        return false;
    }
}