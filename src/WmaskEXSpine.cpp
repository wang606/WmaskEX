#include "header.h"

bool wmaskEXSpineOnTimeout(const EventData& e, LRESULT& r) {
    if (e.msg != WM_TIMER) return false; 
    WmaskEXSpine* pData = reinterpret_cast<WmaskEXSpine*>(GetWindowLongPtr(e.hwnd, GWLP_USERDATA));
    if (!pData) return true;

    wglMakeCurrent(pData->hdc, pData->hglrc);
    glbinding::useContext((glbinding::ContextHandle)pData->hglrc);
    
    RECT parentRect; 
    GetClientRect(pData->parentHwnd, &parentRect);
    if (parentRect.right < 0 || parentRect.bottom < 0) return true; 
    if (pData->parentSize.cx != parentRect.right || pData->parentSize.cy != parentRect.bottom) {
        pData->parentSize = SIZE { parentRect.right, parentRect.bottom };
        float s; 
        float ws = pData->parentSize.cx / pData->bounds.width; 
        float hs = pData->parentSize.cy / pData->bounds.height;
        switch (pData->config.sizeType) {
        case WmaskEXConfig::SizeType::ST_Fill:
            s = max(ws, hs);
            break;
        case WmaskEXConfig::SizeType::ST_Fit:
            s = min(ws, hs);
            break;
        case WmaskEXConfig::SizeType::ST_FollowHeight:
            s = hs;
            break;
        case WmaskEXConfig::SizeType::ST_FollowWidth:
            s = ws;
            break;
        case WmaskEXConfig::SizeType::ST_Fix:
            s = 1.0f;
            break;
        }
        s *= pData->config.scale / 100.0f;
        pData->x = (pData->parentSize.cx - pData->bounds.width * s) * pData->config.horizontal / 100.0f
            + pData->config.xShift - pData->bounds.x * s;
        pData->y = (pData->parentSize.cy - pData->bounds.height * s) * pData->config.vertical / 100.0f
            + pData->config.yShift - pData->bounds.y * s;
        SetWindowPos(e.hwnd, HWND_TOP, 0, 0, pData->parentSize.cx, pData->parentSize.cy, SWP_NOACTIVATE | SWP_NOMOVE);
        glViewport(0, 0, pData->parentSize.cx, pData->parentSize.cy);
        if (pData->fboID) {
            glDeleteFramebuffers(1, &pData->fboID);
            glDeleteTextures(1, &pData->textureID);
            pData->fboID = 0;
            pData->textureID = 0;
        }
        glGenFramebuffers(1, &pData->fboID);
        glBindFramebuffer(GL_FRAMEBUFFER, pData->fboID);
        glGenTextures(1, &pData->textureID);
        glBindTexture(GL_TEXTURE_2D, pData->textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pData->parentSize.cx, pData->parentSize.cy, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pData->textureID, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pData->pixels.resize(pData->parentSize.cx * pData->parentSize.cy * 4, 0);
        pData->spineRuntime->setPosition(pData->x, pData->y);
        pData->spineRuntime->setScale(s);
        pData->spineRuntime->setViewportSize(pData->parentSize.cx, pData->parentSize.cy);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, pData->fboID);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float currentTime = getCurrentTimeInSeconds();
    float deltaTime = currentTime - pData->lastUpdateTime;
    pData->lastUpdateTime = currentTime;
    pData->spineRuntime->update(deltaTime);
    pData->spineRuntime->draw(pData->pma);
    glReadPixels(0, 0, pData->parentSize.cx, pData->parentSize.cy, GL_BGRA, GL_UNSIGNED_BYTE, pData->pixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = pData->parentSize.cx;
    bmi.bmiHeader.biHeight = -pData->parentSize.cy; 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    POINT ptDst = { 0, 0 };
    SIZE pSize = { pData->parentSize.cx, pData->parentSize.cy };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, static_cast<BYTE>(pData->config.opacity), AC_SRC_ALPHA };
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP memBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    SetBitmapBits(memBitmap, pData->parentSize.cx * pData->parentSize.cy * 4, pData->pixels.data());
    HGDIOBJ hOld = SelectObject(hdcMem, memBitmap);
    UpdateLayeredWindow(e.hwnd, hdcScreen, &ptDst, &pSize, hdcMem, &ptSrc, NULL, &blend, ULW_ALPHA);
    SelectObject(hdcMem, hOld);
    DeleteObject(memBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    if (currentTime - pData->lastUpdateAnimationTime >= pData->animationDurations[pData->curIdx]) {
        if (pData->multiSkin)
            pData->spineRuntime->setSkin(pData->skinNames[int(getRandomFloat() * pData->skinNames.size())]);
        pData->curIdx = int(getRandomFloat() * pData->animationNames.size());
        pData->spineRuntime->setAnimation(pData->animationNames[pData->curIdx]);
        pData->lastUpdateAnimationTime = currentTime;
    }
    return true; 
}

bool wmaskEXSpineOnDestroy(const EventData& e, LRESULT& r) {
    if (e.msg != WM_DESTROY) return false; 
    KillTimer(e.hwnd, 0);
    WmaskEXSpine* pData = reinterpret_cast<WmaskEXSpine*>(GetWindowLongPtr(e.hwnd, GWLP_USERDATA));
    if (!pData) return true;
    wglMakeCurrent(pData->hdc, pData->hglrc);
    glbinding::useContext((glbinding::ContextHandle)pData->hglrc);
    pData->spineRuntime->dispose();
    delete pData->spineRuntime;
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(pData->hglrc);
    ReleaseDC(e.hwnd, pData->hdc);
    delete pData;
    SetWindowLongPtr(e.hwnd, GWLP_USERDATA, 0);
    return true; 
}

LRESULT CALLBACK wmaskEXSpineProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EventData e = { hwnd, msg, wParam, lParam };
    LRESULT r = 0;
    if (wmaskEXSpineOnTimeout(e, r)) return r;
    if (wmaskEXSpineOnDestroy(e, r)) return r;
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void registerWmaskEXSpineClass() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = wmaskEXSpineProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"WmaskEXSpineClass";
    RegisterClass(&wc);
}

HWND createWmaskEXSpineWindow(const WmaskEXConfig& config, const WmaskEXAssetConfig& assetConfig) {
    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, 
        L"WmaskEXSpineClass", NULL, WS_VISIBLE, 0, 0, 10, 10, NULL, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_CHILD);
    SetParent(hwnd, assetConfig.parentHwnd);
    WmaskEXSpine* pData = new WmaskEXSpine;
    pData->config = config;
    pData->parentHwnd = assetConfig.parentHwnd;
    pData->hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 
        1, 
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 
        PFD_TYPE_RGBA, 
        32, 0, 0, 0, 0, 0, 0, 
        8, 0, 0, 0, 0, 0, 0, 
        24, 8, 0, 
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    }; 
    int pixelFormat = ChoosePixelFormat(pData->hdc, &pfd);
    SetPixelFormat(pData->hdc, pixelFormat, &pfd);
    pData->hglrc = wglCreateContext(pData->hdc);
    wglMakeCurrent(pData->hdc, pData->hglrc);
    glbinding::initialize((glbinding::ContextHandle)pData->hglrc, nullptr, true, false); 
    pData->fboID = 0;
    pData->textureID = 0;
    pData->parentSize = { 0, 0 };

    switch (assetConfig.spineVersion) {
    case WmaskEXAssetConfig::SpineVersion::SV_42:
        pData->spineRuntime = createSpineRuntime42();
        break;
    case WmaskEXAssetConfig::SpineVersion::SV_41:
        pData->spineRuntime = createSpineRuntime41();
        break;
    case WmaskEXAssetConfig::SpineVersion::SV_40:
        pData->spineRuntime = createSpineRuntime40();
        break;
    case WmaskEXAssetConfig::SpineVersion::SV_38:
        pData->spineRuntime = createSpineRuntime38();
        break;
    case WmaskEXAssetConfig::SpineVersion::SV_37:
        pData->spineRuntime = createSpineRuntime37();
        break;
    }
    pData->bounds = assetConfig.bounds;
    pData->pma = assetConfig.pma;
    namespace fs = std::filesystem;
    fs::path assetDir = assetConfig.assetPath;
    std::wstring baseName = assetDir.filename().wstring();
    fs::path atlasPath = assetDir / (baseName + L".atlas");
    fs::path jsonPath = assetDir / (baseName + L".json");
    fs::path skelPath = assetDir / (baseName + L".skel");
    std::string atlasPathString = atlasPath.string();
    std::string skeletonPathString = fs::exists(jsonPath) ? jsonPath.string() : skelPath.string();
    pData->spineRuntime->init(atlasPathString, skeletonPathString);
    pData->skinNames = pData->spineRuntime->getAllSkins();
    pData->multiSkin = pData->skinNames.size() > 1;
    auto allAnimations = pData->spineRuntime->getAllAnimations();
    for (const auto& [name, duration] : allAnimations) {
        if (duration > wmaskEXSpineAnimationMinDuration) {
            pData->animationNames.push_back(name);
            pData->animationDurations.push_back(duration);
        }
    }
    if (pData->animationNames.empty()) {
        pData->animationNames.push_back(allAnimations.begin()->first);
        pData->animationDurations.push_back(allAnimations.begin()->second);
    }
    pData->curIdx = int(getRandomFloat() * pData->animationNames.size());
    float currentTime = getCurrentTimeInSeconds(); 
    pData->lastUpdateTime = currentTime; 
    pData->lastUpdateAnimationTime = currentTime;
    pData->spineRuntime->setDefaultMix(0.2f);
    pData->spineRuntime->createRenderer();
    pData->spineRuntime->setAnimation(pData->animationNames[pData->curIdx]);
    if (pData->multiSkin)
        pData->spineRuntime->setSkin(pData->skinNames[int(getRandomFloat() * pData->skinNames.size())]);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pData));
    UpdateWindow(hwnd);
    SetTimer(hwnd, 0, wmaskEXSpineRefreshDuration, NULL); 
    return hwnd;
}