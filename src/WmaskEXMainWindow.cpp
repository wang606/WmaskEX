#include "header.h"

HINSTANCE hInstance = GetModuleHandle(NULL);
HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(240, 240, 240));
std::wstring configFilePath = L".wmaskex";
std::wstring previewImagePath = L"cover.png"; 
std::map<std::wstring, WmaskEXConfig> wmaskEXConfigs;
std::map<HWND, HwndInfo> hwndInfos;

HWND mainWindowHwnd; 
HWND configListHwnd, previewHwnd; 
HWND nameLabelHwnd, nameEditHwnd; 
HWND exePathLabelHwnd, exePathEditHwnd;
HWND assetsPathLabelHwnd, assetsPathEditHwnd;
HWND previewPathLabelHwnd, previewPathEditHwnd;
HWND sizeLabelHwnd, sizeComboHwnd, scaleLabelHwnd, scaleEditHwnd, scaleUpdownHwnd;
HWND horizontalLabelHwnd, horizontalEditHwnd, horizontalUpdownHwnd, xshiftLabelHwnd, xshiftEditHwnd, xshiftUpdownHwnd;
HWND verticalLabelHwnd, verticalEditHwnd, verticalUpdownHwnd, yshiftLabelHwnd, yshiftEditHwnd, yshiftUpdownHwnd;
HWND durationLabelHwnd, durationEditHwnd, durationUpdownHwnd, opacityLabelHwnd, opacityEditHwnd, opacityUpdownHwnd;
HWND deleteButtonHwnd, applyButtonHwnd, enableButtonHwnd; 

/* WmaskEX Tray */

void createWmaskEXTray(HWND hwnd) {
    NOTIFYICONDATA nid = {0}; 
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_WmaskEXTray; 
    nid.uVersion = NOTIFYICON_VERSION; 
    nid.uCallbackMessage = WM_WmaskEXTray;
    nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON));
    wcscpy_s(nid.szTip, L"WmaskEX");
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void destroyWmaskEXTray(HWND hwnd) {
    NOTIFYICONDATA nid = {0}; 
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_WmaskEXTray;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void popupWmaskEXTrayMenu(HWND hwnd) {
    POINT cursor; 
    GetCursorPos(&cursor);
    HMENU trayMenu = CreatePopupMenu();
    UINT_PTR uid = ID_WmaskEXTrayStart + 1;
    for (const auto& [name, config] : wmaskEXConfigs) {
        if (config.active) {
            int num = 0; 
            for (const auto& [hwnd, info] : hwndInfos)
                if (info.childHwnds.contains(name)) num++; 
            AppendMenu(trayMenu, MF_CHECKED | MF_STRING, uid++, (name + L"(" + std::to_wstring(num) + L")").c_str());
        } else AppendMenu(trayMenu, MF_STRING, uid++, name.c_str());
    }
    AppendMenu(trayMenu, MF_SEPARATOR, NULL, NULL);
    AppendMenu(trayMenu, MF_STRING, ID_ShowWmaskEX, L"Show WmaskEX");
    AppendMenu(trayMenu, MF_STRING, ID_QuitWmaskEX, L"Quit WmaskEX");
    SetMenuDefaultItem(trayMenu, ID_ShowWmaskEX, FALSE);
    TrackPopupMenu(trayMenu, TPM_LEFTALIGN, cursor.x, cursor.y, 0, hwnd, NULL);
}

/* WmaskEX Preview */

LRESULT CALLBACK wmaskEXPreviewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        Gdiplus::Image previewImage(previewImagePath.c_str());
        RECT rect; 
        GetClientRect(hwnd, &rect);
        double sw = (double)rect.right / previewImage.GetWidth();
        double sh = (double)rect.bottom / previewImage.GetHeight();
        double scale = (sw + sh) / 2; 
        int width = (int)(previewImage.GetWidth() * scale);
        int height = (int)(previewImage.GetHeight() * scale);
        int x = (rect.right - width) / 2;
        int y = (rect.bottom - height) / 2;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Gdiplus::Graphics graphics(hdc);
        graphics.Clear(Gdiplus::Color(240, 240, 240)); 
        graphics.DrawImage(&previewImage, x, y, width, height);
        EndPaint(hwnd, &ps);
        return 0; 
    } else return DefWindowProc(hwnd, msg, wParam, lParam);
}

void registerWmaskEXPreviewClass() {
    WNDCLASS wc = {0}; 
    wc.lpfnWndProc = wmaskEXPreviewProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WmaskEXPreviewClass";
    wc.hbrBackground = hBackgroundBrush;
    RegisterClass(&wc);
}

/* WmaskEX Main Window */

void _loadConfig(const WmaskEXConfig& c) {
    SendMessage(nameEditHwnd, WM_SETTEXT, 0, (LPARAM)c.name.c_str());
    SendMessage(exePathEditHwnd, WM_SETTEXT, 0, (LPARAM)c.exePath.c_str());
    SendMessage(assetsPathEditHwnd, WM_SETTEXT, 0, (LPARAM)c.assetsPath.c_str());
    SendMessage(previewPathEditHwnd, WM_SETTEXT, 0, (LPARAM)c.previewPath.c_str());
    SendMessage(sizeComboHwnd, CB_SETCURSEL, static_cast<WPARAM>(c.sizeType), 0);
    SendMessage(scaleUpdownHwnd, UDM_SETPOS32, 0, c.scale);
    SendMessage(horizontalUpdownHwnd, UDM_SETPOS32, 0, c.horizontal);
    SendMessage(xshiftUpdownHwnd, UDM_SETPOS32, 0, c.xShift);
    SendMessage(verticalUpdownHwnd, UDM_SETPOS32, 0, c.vertical);
    SendMessage(yshiftUpdownHwnd, UDM_SETPOS32, 0, c.yShift);
    SendMessage(durationUpdownHwnd, UDM_SETPOS32, 0, c.duration);
    SendMessage(opacityUpdownHwnd, UDM_SETPOS32, 0, c.opacity);
    previewImagePath = c.previewPath.empty() ? L"cover.png" : c.previewPath;
    InvalidateRect(previewHwnd, NULL, TRUE);
    if (c.active) {
        EnableWindow(deleteButtonHwnd, FALSE);
        EnableWindow(applyButtonHwnd, FALSE);
        SendMessage(enableButtonHwnd, WM_SETTEXT, 0, (LPARAM)L"Disable");
    } else {
        EnableWindow(deleteButtonHwnd, TRUE);
        EnableWindow(applyButtonHwnd, TRUE);
        SendMessage(enableButtonHwnd, WM_SETTEXT, 0, (LPARAM)L"Enable");
    }
}

bool _formConfig(WmaskEXConfig& c) {
    wchar_t text[MAX_PATH] = {0};
    c.active = false;
    SendMessage(nameEditHwnd, WM_GETTEXT, MAX_PATH, (LPARAM)text);
    c.name = text;
    if (c.name.empty()) {
        MessageBox(mainWindowHwnd, L"Name cannot be empty.", L"Error", MB_ICONERROR);
        return false;
    }
    SendMessage(exePathEditHwnd, WM_GETTEXT, MAX_PATH, (LPARAM)text);
    c.exePath = text;
    std::transform(c.exePath.begin(), c.exePath.end(), c.exePath.begin(), ::towlower);
    if (c.exePath.empty()) {
        MessageBox(mainWindowHwnd, L"Executable path cannot be empty.", L"Error", MB_ICONERROR);
        return false;
    }
    SendMessage(assetsPathEditHwnd, WM_GETTEXT, MAX_PATH, (LPARAM)text);
    c.assetsPath = text;
    if (c.assetsPath.empty()) {
        MessageBox(mainWindowHwnd, L"Assets path cannot be empty.", L"Error", MB_ICONERROR);
        return false;
    }
    fs::path assetsPath(c.assetsPath);
    if (!fs::exists(assetsPath) || !fs::is_directory(assetsPath)) {
        MessageBox(mainWindowHwnd, L"Assets path does not exist or is not a directory.", L"Error", MB_ICONERROR);
        return false;
    }
    SendMessage(previewPathEditHwnd, WM_GETTEXT, MAX_PATH, (LPARAM)text);
    c.previewPath = text;
    if (c.previewPath.empty()) {
        MessageBox(mainWindowHwnd, L"Preview path cannot be empty.", L"Error", MB_ICONERROR);
        return false;
    }
    fs::path previewPath(c.previewPath);
    if (!fs::exists(previewPath)) {
        MessageBox(mainWindowHwnd, L"Preview path does not exist.", L"Error", MB_ICONERROR);
        return false;
    }
    c.sizeType = static_cast<WmaskEXConfig::SizeType>(SendMessage(sizeComboHwnd, CB_GETCURSEL, 0, 0)); 
    c.scale = static_cast<int>(SendMessage(scaleUpdownHwnd, UDM_GETPOS32, 0, 0));
    c.horizontal = static_cast<int>(SendMessage(horizontalUpdownHwnd, UDM_GETPOS32, 0, 0));
    c.xShift = static_cast<int>(SendMessage(xshiftUpdownHwnd, UDM_GETPOS32, 0, 0));
    c.vertical = static_cast<int>(SendMessage(verticalUpdownHwnd, UDM_GETPOS32, 0, 0));
    c.yShift = static_cast<int>(SendMessage(yshiftUpdownHwnd, UDM_GETPOS32, 0, 0));
    c.duration = static_cast<int>(SendMessage(durationUpdownHwnd, UDM_GETPOS32, 0, 0));
    c.opacity = static_cast<int>(SendMessage(opacityUpdownHwnd, UDM_GETPOS32, 0, 0));
    return true;
}

void _resetConfig() {
    SendMessage(nameEditHwnd, WM_SETTEXT, 0, (LPARAM)L"");
    SendMessage(exePathEditHwnd, WM_SETTEXT, 0, (LPARAM)L"");
    SendMessage(assetsPathEditHwnd, WM_SETTEXT, 0, (LPARAM)L"");
    SendMessage(previewPathEditHwnd, WM_SETTEXT, 0, (LPARAM)L"");
    SendMessage(sizeComboHwnd, CB_SETCURSEL, 0, 0);
    SendMessage(scaleUpdownHwnd, UDM_SETPOS32, 0, 100);
    SendMessage(horizontalUpdownHwnd, UDM_SETPOS32, 0, 0);
    SendMessage(xshiftUpdownHwnd, UDM_SETPOS32, 0, 0);
    SendMessage(verticalUpdownHwnd, UDM_SETPOS32, 0, 0);
    SendMessage(yshiftUpdownHwnd, UDM_SETPOS32, 0, 0);
    SendMessage(durationUpdownHwnd, UDM_SETPOS32, 0, 100);
    SendMessage(opacityUpdownHwnd, UDM_SETPOS32, 0, 100);
    previewImagePath = L"cover.png";
    InvalidateRect(previewHwnd, NULL, TRUE);
    EnableWindow(deleteButtonHwnd, TRUE);
    EnableWindow(applyButtonHwnd, TRUE);
    SendMessage(enableButtonHwnd, WM_SETTEXT, 0, (LPARAM)L"Enable");
}

bool configListOnSelChange(const EventData& e, LRESULT& r) {
    if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_ConfigList && HIWORD(e.wParam) == LBN_SELCHANGE) {
        int selIndex = SendMessage(configListHwnd, LB_GETCURSEL, 0, 0);
        if (selIndex != LB_ERR) {
            wchar_t name[MAX_PATH] = {0}; 
            SendMessage(configListHwnd, LB_GETTEXT, selIndex, (LPARAM)name);
            auto it = wmaskEXConfigs.find(name);
            if (it != wmaskEXConfigs.end())
                _loadConfig(it->second);
            else
                _resetConfig();
        }
        r = TRUE; 
        return true; 
    } else return false; 
}

bool exePathEditOnDropdown(const EventData& e, LRESULT& r) {
    if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_ExePathEdit && HIWORD(e.wParam) == CBN_DROPDOWN) {
        long long int remainNum = 1; 
        while (remainNum > 0) remainNum = SendMessage(exePathEditHwnd, CB_DELETESTRING, 0, 0);
        std::set<std::wstring> exePaths;
        for (const auto& [hwnd, info] : hwndInfos) {
            if (info.exePath.empty()) continue; 
            if (!isValidWmaskEXParentWindow(hwnd)) continue;
            exePaths.insert(info.exePath);
        }
        for (const auto& path : exePaths)
            SendMessage(exePathEditHwnd, CB_ADDSTRING, 0, (LPARAM)path.c_str());
        r = TRUE; 
        return true; 
    } else return false; 
}

bool deleteButtonOnClicked(const EventData& e, LRESULT& r) {
    if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_Delete && HIWORD(e.wParam) == BN_CLICKED) {
        int selIndex = SendMessage(configListHwnd, LB_GETCURSEL, 0, 0);
        if (selIndex != LB_ERR) {
            wchar_t selectedName[MAX_PATH] = {0};
            SendMessage(configListHwnd, LB_GETTEXT, selIndex, (LPARAM)selectedName);
            wchar_t editName[MAX_PATH] = {0};
            SendMessage(nameEditHwnd, WM_GETTEXT, MAX_PATH, (LPARAM)editName);
            if (wcscmp(selectedName, editName) == 0) {
                if (MessageBox(mainWindowHwnd, L"Delete this config?", L"Confirm Delete", MB_ICONQUESTION | MB_YESNO) == IDYES) {
                    wmaskEXConfigs.erase(selectedName);
                    SendMessage(configListHwnd, LB_DELETESTRING, selIndex, 0);
                }
            }
        }
        _resetConfig();
        r = TRUE; 
        return true; 
    } else return false; 
}

bool applyButtonOnClicked(const EventData& e, LRESULT& r) {
    if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_Apply && HIWORD(e.wParam) == BN_CLICKED) {
        WmaskEXConfig c; 
        if (_formConfig(c)) {
            if (!wmaskEXConfigs.contains(c.name))
                SendMessage(configListHwnd, LB_ADDSTRING, 0, (LPARAM)c.name.c_str());
            wmaskEXConfigs[c.name] = c;
        }
        r = TRUE; 
        return true; 
    } else return false; 
}

bool enableButtonOnClicked(const EventData& e, LRESULT& r) {
    if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_Enable && HIWORD(e.wParam) == BN_CLICKED) {
        int selIndex = SendMessage(configListHwnd, LB_GETCURSEL, 0, 0);
        if (selIndex != LB_ERR) {
            wchar_t selectedName[MAX_PATH] = {0};
            SendMessage(configListHwnd, LB_GETTEXT, selIndex, (LPARAM)selectedName);
            wchar_t editName[MAX_PATH] = {0};
            SendMessage(nameEditHwnd, WM_GETTEXT, MAX_PATH, (LPARAM)editName);
            if (wcscmp(selectedName, editName) == 0) {
                auto it = wmaskEXConfigs.find(selectedName);
                if (it != wmaskEXConfigs.end()) {
                    it->second.active = !it->second.active;
                    _loadConfig(it->second);
                }
            }
        }
        r = TRUE; 
        return true; 
    } else return false; 
}

bool traySlot(const EventData& e, LRESULT& r) {
    if (e.msg == WM_WmaskEXTray) {
        switch (e.lParam) {
        case WM_LBUTTONDBLCLK:
            ShowWindow(mainWindowHwnd, SW_SHOW);
            SetForegroundWindow(mainWindowHwnd);
            break; 
        case WM_RBUTTONUP:
            SetForegroundWindow(e.hwnd);
            popupWmaskEXTrayMenu(e.hwnd);
            break;
        }
        r = TRUE;
        return true;
    } else if (e.msg == WM_COMMAND && LOWORD(e.wParam) > ID_WmaskEXTrayStart && LOWORD(e.wParam) < ID_WmaskEXTrayEnd) {
        int uid = ID_WmaskEXTrayStart + 1; 
        for (auto it = wmaskEXConfigs.begin(); it != wmaskEXConfigs.end(); it++, uid++) {
            if (uid == LOWORD(e.wParam)) {
                it->second.active = !it->second.active;
                _loadConfig(it->second);
                break; 
            }
        }
        r = TRUE; 
        return true;
    } else if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_ShowWmaskEX) {
        ShowWindow(e.hwnd, SW_RESTORE); 
        r = TRUE; 
        return true;
    } else if (e.msg == WM_COMMAND && LOWORD(e.wParam) == ID_QuitWmaskEX) {
        destroyWmaskEXTray(e.hwnd);
        DestroyWindow(e.hwnd);
        r = TRUE; 
        return true;
    } else return false;
}

BOOL CALLBACK _enumWindowsCallback(HWND hwnd, LPARAM) {
	if (hwndInfos.contains(hwnd)) return TRUE; 
    if (IsWindow(hwnd)) {
        HwndInfo info;
        info.exePath = L"";
        info.childHwnds.clear();
        wchar_t exePath[MAX_PATH] = {0};
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (hProcess) {
                if (GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH) > 0) {
                    wchar_t* filename = wcsrchr(exePath, L'\\');
                    if (filename)
                        filename++;
                    else
                        filename = exePath;
                    std::wstring exeName(filename);
                    std::transform(exeName.begin(), exeName.end(), exeName.begin(), ::towlower);
                    info.exePath = exeName;
                }
                CloseHandle(hProcess);
            }
        }
        hwndInfos[hwnd] = info;
    }
    return TRUE; 
}


bool mainWindowonTimeout(const EventData& e, LRESULT& r) {
    if (e.msg == WM_TIMER) {
        EnumWindows(_enumWindowsCallback, 0);
        for (auto it = hwndInfos.begin(); it != hwndInfos.end();) {
            if (!IsWindow(it->first)) {
                for (auto child : it->second.childHwnds)
                    DestroyWindow(child.second.hwnd);
                it = hwndInfos.erase(it);
            } else {
                if (isValidWmaskEXParentWindow(it->first)) {
                    for (auto& [name, config] : wmaskEXConfigs) {
                        if (config.active && it->second.exePath == config.exePath) {
                            float currentTime = getCurrentTimeInSeconds();
                            if (it->second.childHwnds.contains(name)) {
                                if (currentTime - it->second.childHwnds[name].creationTime > config.duration) {
                                    DestroyWindow(it->second.childHwnds[name].hwnd);
                                    it->second.childHwnds.erase(name);
                                }
                            } else {
                                WmaskEXAssetConfig assetConfig;
                                if (getRandomAsset(config.assetsPath, assetConfig)) {
                                    assetConfig.parentHwnd = it->first;
                                    HWND hwnd = NULL; 
                                    switch (assetConfig.type) {
                                    case WmaskEXAssetConfig::AssetType::AT_Image:
                                        hwnd = createWmaskEXImageWindow(config, assetConfig);
                                        break;
                                    case WmaskEXAssetConfig::AssetType::AT_Spine:
                                        hwnd = createWmaskEXSpineWindow(config, assetConfig);
                                        break;
                                    }
                                    it->second.childHwnds[name].hwnd = hwnd;
                                    it->second.childHwnds[name].creationTime = currentTime;
                                } else it->second.childHwnds[name] = { NULL, currentTime };
                            }
                        }
                        if (!config.active && it->second.exePath == config.exePath && it->second.childHwnds.contains(name)) {
                            DestroyWindow(it->second.childHwnds[name].hwnd);
                            it->second.childHwnds.erase(name);
                        }
                    }
                }
                it++;
            }
        }
        r = TRUE; 
        return true; 
    } else return false; 
}

bool mainWindowOnClose(const EventData& e, LRESULT& r) {
    if (e.msg == WM_CLOSE) {
        ShowWindow(e.hwnd, SW_HIDE);
        r = TRUE; 
        return true; 
    } else return false; 
}

bool mainWindowOnDestroy(const EventData& e, LRESULT& r) {
    if (e.msg == WM_DESTROY) {
        saveConfig(configFilePath, wmaskEXConfigs);
        PostQuitMessage(0);
        r = TRUE; 
        return true; 
    } else return false; 
}

LRESULT CALLBACK wmaskEXMainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EventData e = { hwnd, msg, wParam, lParam };
    LRESULT r = 0;
    if (configListOnSelChange(e, r)) return r;
    if (exePathEditOnDropdown(e, r)) return r;
    if (deleteButtonOnClicked(e, r)) return r;
    if (applyButtonOnClicked(e, r)) return r;
    if (enableButtonOnClicked(e, r)) return r;
    if (traySlot(e, r)) return r;
    if (mainWindowonTimeout(e, r)) return r;
    if (mainWindowOnClose(e, r)) return r;
    if (mainWindowOnDestroy(e, r)) return r;
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void registerWmaskEXMainWindowClass() {
    WNDCLASS wc = {0}; 
    wc.lpfnWndProc = wmaskEXMainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WmaskEXMainWindowClass";
    wc.hbrBackground = hBackgroundBrush;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    RegisterClass(&wc);
}

HWND createWmaskEXMainWindow() {
    int width = 670; 
    int height = 374;
    int padding = 10;
    int row_height = 26; 
    int column_width[] = { 150, 70, 70, 70, 70, 70, 70 }; 
    auto CW = [padding, row_height, column_width](LPCWSTR type, LPCWSTR title, DWORD style, 
        int row_idx, int col_idx, int row_num = 1, int col_num = 1, HMENU menu = NULL) {
        int w = column_width[col_idx]; 
        for (int i = 1; i < col_num; i++)
            w += column_width[col_idx + i] + padding;
        int h = row_height * row_num + padding * (row_num - 1);
        int x = padding; 
        for (int i = 0; i < col_idx; i++)
            x += column_width[i] + padding;
        int y = padding + row_height * row_idx + padding * row_idx;
        return CreateWindowEx(NULL, type, title, WS_VISIBLE | WS_CHILD | style, 
            x, y, w, h, mainWindowHwnd, menu, hInstance, NULL);
    }; 

    mainWindowHwnd = CreateWindowEx(NULL, L"WmaskEXMainWindowClass", L"WmaskEX", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 
        width, height, NULL, NULL, hInstance, NULL);
    
    configListHwnd = CW(L"LISTBOX", NULL, LBS_STANDARD | LBS_DISABLENOSCROLL, 0, 0, 4, 1, (HMENU)ID_ConfigList); 
    previewHwnd = CW(L"WmaskEXPreviewClass", NULL, WS_BORDER, 4, 0, 5, 1);
    nameLabelHwnd = CW(L"STATIC", L"name", NULL, 0, 1);
    nameEditHwnd = CW(L"EDIT", NULL, ES_AUTOHSCROLL | WS_BORDER, 0, 2, 1, 5);
    exePathLabelHwnd = CW(L"STATIC", L"parent", NULL, 1, 1);
    exePathEditHwnd = CW(L"COMBOBOX", NULL, CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_BORDER, 1, 2, 1, 5, (HMENU)ID_ExePathEdit);
    assetsPathLabelHwnd = CW(L"STATIC", L"assets", NULL, 2, 1);
    assetsPathEditHwnd = CW(L"EDIT", NULL, ES_AUTOHSCROLL | WS_BORDER, 2, 2, 1, 5);
    previewPathLabelHwnd = CW(L"STATIC", L"preview", NULL, 3, 1);
    previewPathEditHwnd = CW(L"EDIT", NULL, ES_AUTOHSCROLL | WS_BORDER, 3, 2, 1, 5);
    sizeLabelHwnd = CW(L"STATIC", L"size", NULL, 4, 1);
    sizeComboHwnd = CW(L"COMBOBOX", NULL, CBS_DROPDOWNLIST, 4, 2, 1, 2);
    scaleLabelHwnd = CW(L"STATIC", L"scale", NULL, 4, 4);
    scaleEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 4, 5, 1, 2);
    horizontalLabelHwnd = CW(L"STATIC", L"horizontal", NULL, 5, 1);
    horizontalEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 5, 2, 1, 2);
    xshiftLabelHwnd = CW(L"STATIC", L"x shift", NULL, 5, 4);
    xshiftEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 5, 5, 1, 2);
    verticalLabelHwnd = CW(L"STATIC", L"vertical", NULL, 6, 1);
    verticalEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 6, 2, 1, 2);
    yshiftLabelHwnd = CW(L"STATIC", L"y shift", NULL, 6, 4);
    yshiftEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 6, 5, 1, 2);
    durationLabelHwnd = CW(L"STATIC", L"duration", NULL, 7, 1);
    durationEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 7, 2, 1, 2);
    opacityLabelHwnd = CW(L"STATIC", L"opacity", NULL, 7, 4);
    opacityEditHwnd = CW(L"EDIT", NULL, ES_NUMBER | WS_BORDER, 7, 5, 1, 2);
    deleteButtonHwnd = CW(L"BUTTON", L"Delete", NULL, 8, 1, 1, 2, (HMENU)ID_Delete);
    applyButtonHwnd = CW(L"BUTTON", L"Apply", NULL, 8, 3, 1, 2, (HMENU)ID_Apply);
    enableButtonHwnd = CW(L"BUTTON", L"Enable", NULL, 8, 5, 1, 2, (HMENU)ID_Enable);
    
    SendMessage(nameEditHwnd, EM_SETLIMITTEXT, MAX_PATH - 10, 0); 
    SendMessage(exePathEditHwnd, EM_SETLIMITTEXT, MAX_PATH - 10, 0);
    SendMessage(assetsPathEditHwnd, EM_SETLIMITTEXT, MAX_PATH - 10, 0);
    SendMessage(previewPathEditHwnd, EM_SETLIMITTEXT, MAX_PATH - 10, 0);

    HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    HWND hwnds[] = {
        configListHwnd, previewHwnd, 
        nameLabelHwnd, nameEditHwnd,
        exePathLabelHwnd, exePathEditHwnd, 
        previewPathLabelHwnd, previewPathEditHwnd, 
        assetsPathLabelHwnd, assetsPathEditHwnd,
        sizeLabelHwnd, sizeComboHwnd, scaleLabelHwnd, scaleEditHwnd, 
        horizontalLabelHwnd, horizontalEditHwnd, xshiftLabelHwnd, xshiftEditHwnd, 
        verticalLabelHwnd, verticalEditHwnd, yshiftLabelHwnd, yshiftEditHwnd,
        durationLabelHwnd, durationEditHwnd, opacityLabelHwnd, opacityEditHwnd,
        deleteButtonHwnd, applyButtonHwnd, enableButtonHwnd
    };
    for (HWND hwnd : hwnds)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(sizeComboHwnd, CB_ADDSTRING, 0, (LPARAM)L"Fill");
    SendMessage(sizeComboHwnd, CB_ADDSTRING, 0, (LPARAM)L"Fit");
    SendMessage(sizeComboHwnd, CB_ADDSTRING, 0, (LPARAM)L"Follow Height");
    SendMessage(sizeComboHwnd, CB_ADDSTRING, 0, (LPARAM)L"Follow Width");
    SendMessage(sizeComboHwnd, CB_ADDSTRING, 0, (LPARAM)L"Fix");
    SendMessage(sizeComboHwnd, CB_SETCURSEL, 0, 0);

    #define CW_UPDOWN(component, min, max, pos) \
    component##UpdownHwnd = CreateWindowEx(NULL, UPDOWN_CLASS, NULL, \
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_NOTHOUSANDS | UDS_ALIGNRIGHT, \
        0, 0, 0, 0, mainWindowHwnd, NULL, GetModuleHandle(NULL), NULL); \
    SendMessage(component##UpdownHwnd, UDM_SETBUDDY, (WPARAM)component##EditHwnd, 0); \
    SendMessage(component##UpdownHwnd, UDM_SETRANGE32, min, max); \
    SendMessage(component##UpdownHwnd, UDM_SETPOS32, 0, pos);
    CW_UPDOWN(scale, 1, 10000, 100);
    CW_UPDOWN(horizontal, -100, 200, 0);
    CW_UPDOWN(xshift, -10000, 10000, 0);
    CW_UPDOWN(vertical, -100, 200, 0);
    CW_UPDOWN(yshift, -10000, 10000, 0);
    CW_UPDOWN(duration, 0, 365*24*60*60, 120);
    CW_UPDOWN(opacity, 0, 255, 255);

    openConfig(configFilePath, wmaskEXConfigs);
    for (const auto& [name, config] : wmaskEXConfigs)
        SendMessage(configListHwnd, LB_ADDSTRING, 0, (LPARAM)name.c_str());
    SetTimer(mainWindowHwnd, 0, wmaskEXRefreshDuration, NULL);

    return mainWindowHwnd;
}