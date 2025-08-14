#include "header.h"

bool wmaskEXImageOnTimeout(const EventData& e, LRESULT& r) {
    if (e.msg != WM_TIMER) return false; 
    WmaskEXImage* pData = reinterpret_cast<WmaskEXImage*>(GetWindowLongPtr(e.hwnd, GWLP_USERDATA));
    if (!pData) return false;
    RECT parentRect;
    GetClientRect(pData->parentHwnd, &parentRect);
    if (pData->parentSize.cx == parentRect.right && pData->parentSize.cy == parentRect.bottom) return true; 
    pData->parentSize = SIZE { parentRect.right, parentRect.bottom }; 
    int x, y, w, h; 
    float s; 
    float ws = float(pData->parentSize.cx) / pData->image->GetWidth(); 
    float hs = float(pData->parentSize.cy) / pData->image->GetHeight();
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
    w = int(pData->image->GetWidth() * s);
    h = int(pData->image->GetHeight() * s);
    x = int((pData->parentSize.cx - w) * pData->config.horizontal / 100.0f
        + pData->config.xShift);
    y = int((pData->parentSize.cy - h) * (100 - pData->config.vertical) / 100.0f
        + pData->config.yShift);
    SetWindowPos(e.hwnd, HWND_TOP, 0, 0, w, h, SWP_NOMOVE);
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, static_cast<BYTE>(pData->config.opacity), AC_SRC_ALPHA };
    HDC hdc = GetDC(e.hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(memDC, memBitmap);
    Gdiplus::Graphics graphics(memDC);
    graphics.DrawImage(pData->image, Gdiplus::RectF(0, 0, w, h));
    POINT ptSrc = { 0, 0 };
    POINT ptDst = { x, y };
    SIZE pSize = { w, h };
    UpdateLayeredWindow(e.hwnd, GetDC(NULL), &ptDst, &pSize, memDC, &ptSrc, NULL, &blend, ULW_ALPHA);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    return true; 
}

bool wmaskEXImageOnDestroy(const EventData& e, LRESULT& r) {
    if (e.msg == WM_DESTROY) {
        HWND hwnd = e.hwnd;
        WmaskEXImage* pData = reinterpret_cast<WmaskEXImage*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pData) {
            delete pData->image; 
            delete pData; 
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        }
        return true; 
    } else return false; 
}

LRESULT CALLBACK wmaskEXImageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EventData e = { hwnd, msg, wParam, lParam };
    LRESULT r = 0;
    if (wmaskEXImageOnTimeout(e, r)) return r;
    if (wmaskEXImageOnDestroy(e, r)) return r;
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void registerWmaskEXImageClass() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = wmaskEXImageProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"WmaskEXImageClass";
    RegisterClass(&wc);
}

HWND createWmaskEXImageWindow(const WmaskEXConfig& config, const WmaskEXAssetConfig& assetConfig) {
    Gdiplus::Image* image = new Gdiplus::Image(assetConfig.assetPath.c_str());
    if (image->GetLastStatus() != Gdiplus::Ok) {
        delete image;
        MessageBox(NULL, (L"Failed to load image: " + assetConfig.assetPath).c_str(), L"Error", MB_ICONERROR | MB_OK);
        return NULL;
    }
    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT, 
        L"WmaskEXImageClass", NULL, WS_VISIBLE, 0, 0, 10, 10, NULL, NULL, GetModuleHandle(NULL), NULL);
    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_CHILD);
    SetParent(hwnd, assetConfig.parentHwnd);
    WmaskEXImage* pData = new WmaskEXImage;
    pData->config = config;
    pData->parentHwnd = assetConfig.parentHwnd;
    pData->image = image;
    pData->parentSize = SIZE { 0, 0 };
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pData));
    UpdateWindow(hwnd); 
    SetTimer(hwnd, 0, wmaskEXImageRefreshDuration, NULL);
    return hwnd;
}