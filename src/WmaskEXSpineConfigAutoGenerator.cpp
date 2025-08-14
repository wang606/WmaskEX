#include "header.h"
#include <iostream>
#include <iomanip>
#include <fstream>

void printUsage() {
    std::cout << "WmaskEX Spine Config Auto Generator\n";
    std::cout << "Usage: WmaskEXSpineConfigAutoGenerator.exe [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -i, --input <path>      Input assets directory path (default: ./assets)\n";
    std::cout << "  --overwrite             Overwrite existing .wmaskex.autogen.json files\n";
    std::cout << "  --scan-only             Only scan and list found assets, don't generate config\n";
    std::cout << "  --verbose               Enable verbose output\n";
    std::cout << "  --pma <mode>            PMA mode: true, false, or auto (default: auto)\n\n";
    std::cout << "PMA Modes:\n";
    std::cout << "  true                    Force PMA to true for all assets\n";
    std::cout << "  false                   Force PMA to false for all assets\n";
    std::cout << "  auto                    Auto-detect PMA from atlas file content\n\n";
    std::cout << "Examples:\n";
    std::cout << "  WmaskEXSpineConfigAutoGenerator.exe -i ./assets\n";
    std::cout << "  WmaskEXSpineConfigAutoGenerator.exe --scan-only --verbose\n";
    std::cout << "  WmaskEXSpineConfigAutoGenerator.exe --pma true -i ./assets\n";
    std::cout << "  WmaskEXSpineConfigAutoGenerator.exe --pma auto -i ./assets\n";
}

enum class PMAMode {
    AUTO,
    FORCE_TRUE,
    FORCE_FALSE
};

struct GeneratorOptions {
    std::string inputPath = "./assets";
    bool overwrite = false;
    bool scanOnly = false;
    bool verbose = false;
    bool showHelp = false;
    PMAMode pmaMode = PMAMode::AUTO;
};

bool parseArguments(int argc, char* argv[], GeneratorOptions& options) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            options.showHelp = true;
            return true;
        }
        else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                options.inputPath = argv[++i];
            } else {
                std::cerr << "Error: --input requires a path argument\n";
                return false;
            }
        }
        else if (arg == "--overwrite") {
            options.overwrite = true;
        }
        else if (arg == "--scan-only") {
            options.scanOnly = true;
        }
        else if (arg == "--verbose") {
            options.verbose = true;
        }
        else if (arg == "--pma") {
            if (i + 1 < argc) {
                std::string pmaValue = argv[++i];
                if (pmaValue == "true") {
                    options.pmaMode = PMAMode::FORCE_TRUE;
                } else if (pmaValue == "false") {
                    options.pmaMode = PMAMode::FORCE_FALSE;
                } else if (pmaValue == "auto") {
                    options.pmaMode = PMAMode::AUTO;
                } else {
                    std::cerr << "Error: Invalid PMA mode. Use 'true', 'false', or 'auto'\n";
                    return false;
                }
            } else {
                std::cerr << "Error: --pma requires a mode argument (true/false/auto)\n";
                return false;
            }
        }
        else {
            std::cerr << "Error: Unknown argument: " << arg << "\n";
            return false;
        }
    }
    return true;
}

struct SpineAssetInfo {
    std::string folderPath;
    std::string assetName;
    std::string atlasPath;
    std::string skeletonPath;
    WmaskEXAssetConfig::SpineVersion version;
    bool isValidAsset;
};

std::string getVersionString(WmaskEXAssetConfig::SpineVersion version) {
    switch (version) {
        case WmaskEXAssetConfig::SpineVersion::SV_37: return "3.7";
        case WmaskEXAssetConfig::SpineVersion::SV_38: return "3.8";
        case WmaskEXAssetConfig::SpineVersion::SV_40: return "4.0";
        case WmaskEXAssetConfig::SpineVersion::SV_41: return "4.1";
        case WmaskEXAssetConfig::SpineVersion::SV_42: return "4.2";
        default: return "4.2";
    }
}

std::string getPMAModeString(PMAMode mode) {
    switch (mode) {
        case PMAMode::AUTO: return "auto";
        case PMAMode::FORCE_TRUE: return "true";
        case PMAMode::FORCE_FALSE: return "false";
        default: return "auto";
    }
}

WmaskEXAssetConfig::SpineVersion detectSpineVersionFromJson(const std::string& jsonPath) {
    const std::vector<std::string> validSpineVersions = {"3.7", "3.8", "4.0", "4.1", "4.2"};
    
    try {
        std::ifstream ifs(jsonPath);
        if (!ifs) return WmaskEXAssetConfig::SpineVersion::SV_Invalid;
        
        nlohmann::json j;
        ifs >> j;
        
        if (!j.contains("skeleton") || !j["skeleton"].contains("spine")) {
            return WmaskEXAssetConfig::SpineVersion::SV_Invalid;
        }
        
        std::string version = j["skeleton"]["spine"].get<std::string>();
        version = version.substr(0, version.find_last_of('.'));
        
        for (int i = 0; i < validSpineVersions.size(); ++i) {
            if (version == validSpineVersions[i]) {
                return static_cast<WmaskEXAssetConfig::SpineVersion>(i);
            }
        }
    }
    catch (...) {
        // JSON parsing error
        std::cerr << "Error: Failed to parse JSON file: " << jsonPath << "\n";
    }
    
    return WmaskEXAssetConfig::SpineVersion::SV_Invalid;
}

WmaskEXAssetConfig::SpineVersion detectSpineVersionFromSkel(const std::string& skelPath) {
    const std::vector<std::string> validSpineVersions = {"3.7", "3.8", "4.0", "4.1", "4.2"};
    
    try {
        std::ifstream ifs(skelPath, std::ios::binary);
        if (!ifs) return WmaskEXAssetConfig::SpineVersion::SV_Invalid;
        
        const size_t headerSize = 64;
        char buffer[headerSize] = {0};
        ifs.read(buffer, headerSize);
        std::string data(buffer, ifs.gcount());
        
        for (int i = 0; i < validSpineVersions.size(); ++i) {
            if (data.find(validSpineVersions[i]) != std::string::npos) {
                return static_cast<WmaskEXAssetConfig::SpineVersion>(i);
            }
        }
    }
    catch (...) {
        // File reading error
        std::cerr << "Error: Failed to read SKEL file: " << skelPath << "\n";
    }
    
    return WmaskEXAssetConfig::SpineVersion::SV_Invalid;
}

bool detectPMAFromAtlas(const std::string& atlasPath, bool verbose = false) {
    try {
        std::ifstream ifs(atlasPath);
        if (!ifs) {
            if (verbose) {
                std::cout << "    Warning: Cannot read atlas file, defaulting PMA to false\n";
            }
            return false;
        }
        
        std::string line;
        while (std::getline(ifs, line)) {
            // Remove leading/trailing whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            // Check for PMA setting in atlas file
            if (line.find("pma:") == 0 || line.find("pma :") == 0) {
                // Extract value after colon
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string value = line.substr(colonPos + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    bool isPMA = (value == "true" || value == "True" || value == "TRUE");
                    if (verbose) {
                        std::cout << "    Detected PMA from atlas: " << (isPMA ? "true" : "false") << "\n";
                    }
                    return isPMA;
                }
            }
        }
        
        if (verbose) {
            std::cout << "    No PMA setting found in atlas, defaulting to false\n";
        }
        return false; // Default to false if no PMA setting found
    }
    catch (...) {
        if (verbose) {
            std::cout << "    Error reading atlas file, defaulting PMA to false\n";
        }
        return false;
    }
}

// Global OpenGL context for reuse
struct SpineContext {
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC hglrc = nullptr;
    ISpineRuntime* spineRuntime = nullptr;
    bool initialized = false;
    
    bool initialize() {
        if (initialized) return true;
        
        std::cout << "Initializing shared OpenGL context..." << std::flush;
        
        // Create a hidden window for OpenGL context
        hwnd = CreateWindowEx(0, L"STATIC", L"OffscreenGL", WS_POPUP,
                             0, 0, 1, 1, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (!hwnd) {
            std::cerr << " FAILED (window creation)\n";
            return false;
        }
        
        hdc = GetDC(hwnd);
        if (!hdc) {
            std::cerr << " FAILED (device context)\n";
            cleanup();
            return false;
        }
        
        // Set up pixel format
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
            PFD_TYPE_RGBA,
            32, 0, 0, 0, 0, 0, 0,
            8, 0, 0, 0, 0, 0, 0,
            24, 8, 0,
            PFD_MAIN_PLANE,
            0, 0, 0, 0
        };
        
        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        if (!pixelFormat || !SetPixelFormat(hdc, pixelFormat, &pfd)) {
            std::cerr << " FAILED (pixel format)\n";
            cleanup();
            return false;
        }
        
        // Create OpenGL context
        hglrc = wglCreateContext(hdc);
        if (!hglrc || !wglMakeCurrent(hdc, hglrc)) {
            std::cerr << " FAILED (OpenGL context)\n";
            cleanup();
            return false;
        }
        
        // Initialize glbinding
        glbinding::initialize((glbinding::ContextHandle)hglrc, nullptr, true, false);
        
        initialized = true;
        std::cout << " OK\n";
        return true;
    }
    
    void cleanup() {
        wglMakeCurrent(hdc, hglrc); 
        glbinding::useContext((glbinding::ContextHandle)hglrc);
        spineRuntime->dispose(); 
        glbinding::releaseCurrentContext(); 
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hwnd, hdc); 
        DestroyWindow(hwnd);
        initialized = false;
    }
    
    ~SpineContext() {
        cleanup();
    }
};

Bounds getBoundsFromRuntime(const std::string& atlasPath, const std::string& skeletonPath, 
                           WmaskEXAssetConfig::SpineVersion version, SpineContext& spineContext) {
    Bounds defaultBounds = {0.0f, 0.0f, 100.0f, 100.0f};
    
    if (!spineContext.initialize()) {
        return defaultBounds;
    }
    
    try {
        std::cout << "  Creating Spine runtime..." << std::flush;
        spineContext.spineRuntime = nullptr;
        
        // Create runtime based on version
        switch (version) {
            case WmaskEXAssetConfig::SpineVersion::SV_37:
                spineContext.spineRuntime = createSpineRuntime37();
                break;
            case WmaskEXAssetConfig::SpineVersion::SV_38:
                spineContext.spineRuntime  = createSpineRuntime38();
                break;
            case WmaskEXAssetConfig::SpineVersion::SV_40:
                spineContext.spineRuntime = createSpineRuntime40();
                break;
            case WmaskEXAssetConfig::SpineVersion::SV_41:
                spineContext.spineRuntime = createSpineRuntime41();
                break;
            case WmaskEXAssetConfig::SpineVersion::SV_42:
                spineContext.spineRuntime = createSpineRuntime42();
                break;
            default:
                throw std::runtime_error("Invalid Spine version");
        }

        if (!spineContext.spineRuntime) {
            throw std::runtime_error("Failed to create Spine runtime");
        }
        std::cout << " OK\n";
        
        std::cout << "  Loading Spine asset..." << std::flush;
        // Initialize runtime and get bounds
        spineContext.spineRuntime->init(atlasPath, skeletonPath);
        std::cout << " OK\n";
        
        std::cout << "  Getting bounds..." << std::flush;
        Bounds bounds = spineContext.spineRuntime->getBounds();
        std::cout << " OK\n";
        
        std::cout << "  Cleaning up runtime..." << std::flush;
        spineContext.cleanup(); 
        std::cout << " OK\n";
        
        std::cout << "  Bounds: [" << bounds.x << ", " << bounds.y << ", " 
                  << bounds.width << ", " << bounds.height << "]\n";
        
        return bounds;
    }
    catch (const std::exception& ex) {
        std::cerr << " FAILED: " << ex.what() << "\n";
        return defaultBounds;
    }
    catch (...) {
        std::cerr << " FAILED: Unknown exception in Spine runtime initialization\n";
        return defaultBounds;
    }
}

std::vector<SpineAssetInfo> scanSpineAssets(const std::string& assetsPath, bool verbose) {
    std::vector<SpineAssetInfo> assets;
    
    if (verbose) {
        std::cout << "Scanning Spine assets in: " << assetsPath << "\n";
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
            if (!entry.is_directory()) continue;

            std::string folderPath = entry.path().string();
            std::string assetName = entry.path().stem().string();
            
            std::string atlasPath = folderPath + "/" + assetName + ".atlas";
            if (!fs::exists(atlasPath)) continue;

            SpineAssetInfo asset;
            asset.folderPath = folderPath;
            asset.assetName = assetName;
            asset.atlasPath = atlasPath;
            asset.isValidAsset = false;
            asset.version = WmaskEXAssetConfig::SpineVersion::SV_Invalid;

            // Check JSON skeleton file
            std::string jsonPath = folderPath + "/" + assetName + ".json";
            if (fs::exists(jsonPath)) {
                asset.skeletonPath = jsonPath;
                asset.version = detectSpineVersionFromJson(jsonPath);
                if (asset.version != WmaskEXAssetConfig::SpineVersion::SV_Invalid) {
                    asset.isValidAsset = true;
                }
            }
            
            // If no valid JSON found, check SKEL file
            if (!asset.isValidAsset) {
                std::string skelPath = folderPath + "/" + assetName + ".skel";
                if (fs::exists(skelPath)) {
                    asset.skeletonPath = skelPath;
                    asset.version = detectSpineVersionFromSkel(skelPath);
                    if (asset.version != WmaskEXAssetConfig::SpineVersion::SV_Invalid) {
                        asset.isValidAsset = true;
                    }
                }
            }

            if (asset.isValidAsset) {
                assets.push_back(asset);

                if (verbose) {
                    std::cout << "Found Spine asset: " << asset.assetName 
                              << " (Version: " << getVersionString(asset.version) << ")\n";
                    std::cout << "  Folder: " << asset.folderPath << "\n";
                    std::cout << "  Atlas: " << asset.atlasPath << "\n";
                    std::cout << "  Skeleton: " << asset.skeletonPath << "\n";
                }
            }
        }
    }
    catch (const fs::filesystem_error& ex) {
        std::cerr << "Filesystem error: " << ex.what() << "\n";
    }

    return assets;
}

bool generateConfigFile(const SpineAssetInfo& asset, bool overwrite, bool verbose, PMAMode pmaMode, SpineContext& spineContext) {
    std::string configPath = asset.folderPath + "/" + asset.assetName + ".wmaskex.autogen.json";
    
    // Check if file already exists
    if (!overwrite && fs::exists(configPath)) {
        if (verbose) {
            std::cout << "Skipping existing config file: " << configPath << "\n";
        }
        return true;
    }

    try {
        if (verbose) {
            std::cout << "Getting bounds for " << asset.assetName << "...\n";
        }
        
        // Get bounds
        Bounds bounds = getBoundsFromRuntime(asset.atlasPath, asset.skeletonPath, asset.version, spineContext);
        
        if (verbose) {
            std::cout << "Got bounds: [" << bounds.x << ", " << bounds.y << ", " 
                      << bounds.width << ", " << bounds.height << "]\n";
            std::cout << "Creating JSON config...\n";
        }
        
        // Determine PMA value based on mode
        bool pmaValue = false;
        switch (pmaMode) {
            case PMAMode::FORCE_TRUE:
                pmaValue = true;
                if (verbose) {
                    std::cout << "PMA mode: Force true\n";
                }
                break;
            case PMAMode::FORCE_FALSE:
                pmaValue = false;
                if (verbose) {
                    std::cout << "PMA mode: Force false\n";
                }
                break;
            case PMAMode::AUTO:
                pmaValue = detectPMAFromAtlas(asset.atlasPath, verbose);
                if (verbose) {
                    std::cout << "PMA mode: Auto-detected from atlas\n";
                }
                break;
        }
        
        // Create JSON config
        nlohmann::json config;
        config["version"] = getVersionString(asset.version);
        config["bounds"] = {bounds.x, bounds.y, bounds.width, bounds.height};
        config["pma"] = pmaValue;
        
        if (verbose) {
            std::cout << "Writing config file: " << configPath << "\n";
        }
        
        // Write to file
        std::ofstream ofs(configPath);
        if (!ofs) {
            std::cerr << "Error: Cannot create config file: " << configPath << "\n";
            return false;
        }
        
        ofs << config.dump(4); // 4 spaces indentation
        ofs.close();
        
        if (verbose) {
            std::cout << "Generated config file: " << configPath << "\n";
            std::cout << "  Version: " << getVersionString(asset.version) << "\n";
            std::cout << "  Bounds: [" << bounds.x << ", " << bounds.y << ", " 
                      << bounds.width << ", " << bounds.height << "]\n";
            std::cout << "  PMA: " << (pmaValue ? "true" : "false") << "\n";
            std::cout << "Config generation completed for " << asset.assetName << "\n";
        }
        
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error generating config for " << asset.assetName << ": " << ex.what() << "\n";
        return false;
    }
}

int main(int argc, char* argv[]) {
    // Set console output encoding
    SetConsoleOutputCP(CP_UTF8);
    GeneratorOptions options;

    if (!parseArguments(argc, argv, options)) {
        printUsage();
        return 1;
    }

    if (options.showHelp) {
        printUsage();
        return 0;
    }

    // Check if input path exists
    if (!fs::exists(options.inputPath)) {
        std::cerr << "Error: Input path does not exist: " << options.inputPath << "\n";
        return 1;
    }

    std::cout << "WmaskEX Spine Config Auto Generator\n";
    std::cout << "===================================\n";

    if (options.verbose) {
        std::cout << "Options:\n";
        std::cout << "  Input path: " << options.inputPath << "\n";
        std::cout << "  Overwrite existing: " << (options.overwrite ? "Yes" : "No") << "\n";
        std::cout << "  Scan only: " << (options.scanOnly ? "Yes" : "No") << "\n";
        std::cout << "  PMA mode: " << getPMAModeString(options.pmaMode) << "\n\n";
    }

    // Scan Spine assets
    auto assets = scanSpineAssets(options.inputPath, options.verbose);

    std::cout << "\nScan Results:\n";
    std::cout << "=============\n";
    std::cout << "Total Spine assets found: " << assets.size() << "\n";

    if (assets.empty()) {
        std::cout << "No valid Spine assets found in the specified directory.\n";
        std::cout << "Valid Spine assets must have:\n";
        std::cout << "  - Same-named .atlas file\n";
        std::cout << "  - Same-named .json or .skel file\n";
        std::cout << "  - Valid Spine version information\n";
        return 0;
    }

    // Group and display by version
    std::map<WmaskEXAssetConfig::SpineVersion, int> versionCounts;
    for (const auto& asset : assets) {
        versionCounts[asset.version]++;
    }

    for (const auto& [version, count] : versionCounts) {
        std::cout << "  Version " << getVersionString(version) << ": " << count << " assets\n";
    }

    if (options.scanOnly) {
        std::cout << "\nScan completed (scan-only mode).\n";
        return 0;
    }

    // Generate config files
    std::cout << "\nGenerating config files...\n";
    std::cout << "==========================\n";

    // Create shared OpenGL context
    SpineContext spineContext;

    int successCount = 0;
    int failedCount = 0;
    int skippedCount = 0;

    for (const auto& asset : assets) {
        std::string configPath = asset.folderPath + "/" + asset.assetName + ".wmaskex.autogen.json";

        if (!options.overwrite && fs::exists(configPath)) {
            skippedCount++;
            if (options.verbose) {
                std::cout << "Skipped existing: " << asset.assetName << "\n";
            }
            continue;
        }

        std::cout << "Processing: " << asset.folderPath << "...\n";
        if (generateConfigFile(asset, options.overwrite, options.verbose, options.pmaMode, spineContext)) {
            successCount++;
            std::cout << asset.assetName << " SUCCESS\n";
        } else {
            failedCount++;
            std::cout << asset.assetName << " FAILED\n";
        }
    }

    std::cout << "\nGeneration Summary:\n";
    std::cout << "==================\n";
    std::cout << "Successfully generated: " << successCount << " files\n";
    std::cout << "Skipped existing: " << skippedCount << " files\n";
    std::cout << "Failed: " << failedCount << " files\n";

    if (failedCount == 0) {
        std::cout << "\nAll config files generated successfully!\n";
        return 0;
    } else {
        std::cerr << "\nSome config files failed to generate. Check the error messages above.\n";
        return 1;
    }
}
