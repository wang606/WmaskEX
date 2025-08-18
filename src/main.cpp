#include "header.h"

int main() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"WmaskEXMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0;
    }
    WmaskEXLog::init(); 

    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    registerWmaskEXPreviewClass(); 
    registerWmaskEXMainWindowClass();
    registerWmaskEXImageClass();
    registerWmaskEXSpineClass();
    HWND hwnd = createWmaskEXMainWindow();
    ShowWindow(hwnd, SW_HIDE);
    createWmaskEXTray(hwnd);

    MSG msg; 
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    WmaskEXLog::close(); 
    return 0; 
}