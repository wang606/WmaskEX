#ifndef WMASKEXHEADER_H
#define WMASKEXHEADER_H

#include <windows.h>
#include <commctrl.h>
#include <objidl.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <Psapi.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <glbinding/Binding.h>
#include <map>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "res/resource.h"
#include "ISpineRuntime.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace gl;

// WmaskEX HMENU
#define ID_ConfigList 0x1001
#define ID_ExePathEdit 0x1002
#define ID_Delete 0x1003
#define ID_Apply 0x1004
#define ID_Enable 0x1005

#define ID_WmaskEXTray 0x2001
#define ID_ShowWmaskEX 0x2002
#define ID_QuitWmaskEX 0x2003
#define ID_WmaskEXTrayStart 0x2004
#define ID_WmaskEXTrayEnd 0x2FFF

#define WM_WmaskEXTray (WM_USER + 0x0001)

// WmaskEX Constants
const int wmaskEXRefreshDuration = 100; // ms
const int wmaskEXImageRefreshDuration = 100; // ms
const int wmaskEXSpineRefreshDuration = 40; // ms
const double wmaskEXSpineAnimationMinDuration = 0.5; // s
const std::set<std::wstring> validImageExtensions = { L".png", L".jpg", L".jpeg", L".bmp", L".ico", L".tiff", L".exif", L".wmf", L".emf" };
const std::vector<std::string> validSpineVersions = { "3.7", "3.8", "4.0", "4.1", "4.2" };

// WmaskEX Data Structure
struct EventData {
    HWND hwnd; 
    UINT msg; 
    WPARAM wParam; 
    LPARAM lParam; 
};

struct WmaskEXConfig {
    enum class SizeType {
        ST_Fill, 
        ST_Fit, 
        ST_FollowHeight, 
        ST_FollowWidth, 
        ST_Fix 
    };
    bool active; 
    std::wstring name; 
    std::wstring exePath;
    std::wstring assetsPath; 
    std::wstring previewPath; 
    SizeType sizeType; 
    int scale; 
    int horizontal; 
    int xShift; 
    int vertical; 
    int yShift; 
    int duration; 
    int opacity; 
}; 
void to_json(json& j, const WmaskEXConfig& c); 
void from_json(const json& j, WmaskEXConfig& c); 

struct WmaskEXAssetConfig {
    enum class AssetType {
        AT_Image, 
        AT_Spine 
    };
    enum class SpineVersion {
        SV_37,
        SV_38, 
        SV_40, 
        SV_41, 
        SV_42,
        SV_Invalid
    };
    AssetType type;
    HWND parentHwnd;
    std::wstring assetPath;
    SpineVersion spineVersion;
    Bounds bounds;
    bool pma; 
}; 

struct WmaskEXImage {
    WmaskEXConfig config;
    HWND parentHwnd; 
    Gdiplus::Image* image; 
    SIZE parentSize; 
}; 

struct WmaskEXSpine {
    WmaskEXConfig config;
    HWND parentHwnd; 
    ISpineRuntime* spineRuntime;
    Bounds bounds;
    bool pma;
    std::vector<std::string> skinNames;
    bool multiSkin; 
    std::vector<std::string> animationNames;
    std::vector<float> animationDurations;
    int curIdx; 
    float lastUpdateTime;
    float lastUpdateAnimationTime;
    HDC hdc;
    HGLRC hglrc;
    GLuint fboID, textureID;
    std::vector<BYTE> pixels;
    SIZE parentSize; 
    int x, y; 
}; 

struct ChildHwndInfo {
    HWND hwnd; 
    float creationTime; 
}; 

struct HwndInfo {
    std::wstring exePath; 
    std::map<std::wstring, ChildHwndInfo> childHwnds;
}; 

// WmaskEX Function
float getRandomFloat(); 
float getCurrentTimeInSeconds();
bool isValidWmaskEXParentWindow(HWND); 
bool getRandomAsset(const std::wstring& assetsPath, WmaskEXAssetConfig& assetConfig);
bool openConfig(const std::wstring& configFilePath, std::map<std::wstring, WmaskEXConfig>& configs); 
bool saveConfig(const std::wstring& configFilePath, const std::map<std::wstring, WmaskEXConfig>& configs); 

void createWmaskEXTray(HWND hwnd);
void registerWmaskEXPreviewClass();
void registerWmaskEXMainWindowClass(); 
HWND createWmaskEXMainWindow();

void registerWmaskEXImageClass();
HWND createWmaskEXImageWindow(const WmaskEXConfig& config, const WmaskEXAssetConfig& assetConfig);

void registerWmaskEXSpineClass();
HWND createWmaskEXSpineWindow(const WmaskEXConfig& config, const WmaskEXAssetConfig& assetConfig);

// ISpineRuntime Function
extern "C" __declspec(dllimport) ISpineRuntime* createSpineRuntime37(); 
extern "C" __declspec(dllimport) ISpineRuntime* createSpineRuntime38(); 
extern "C" __declspec(dllimport) ISpineRuntime* createSpineRuntime40(); 
extern "C" __declspec(dllimport) ISpineRuntime* createSpineRuntime41(); 
extern "C" __declspec(dllimport) ISpineRuntime* createSpineRuntime42(); 

#endif // WMASKEXHEADER_H