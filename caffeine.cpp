#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>

#define WM_TRAYICON (WM_USER + 1)
#define TIMER_ID 1

#define ID_TRAY_DISABLED    1002
#define ID_TRAY_5MIN        1003
#define ID_TRAY_15MIN       1004
#define ID_TRAY_30MIN       1005
#define ID_TRAY_1HOUR       1006
#define ID_TRAY_2HOURS      1007
#define ID_TRAY_INDEFINITE  1008
#define ID_TRAY_EXIT        1001

NOTIFYICONDATAW nid = { 0 };
UINT g_ActiveMode = ID_TRAY_INDEFINITE;
DWORD g_TimeRemainingSeconds = 0;
HANDLE g_hMutex = NULL; 

const wchar_t* GetModeName(UINT id) {
    if (id == ID_TRAY_5MIN) return L"5 Minutes";
    if (id == ID_TRAY_15MIN) return L"15 Minutes";
    if (id == ID_TRAY_30MIN) return L"30 Minutes";
    if (id == ID_TRAY_1HOUR) return L"1 Hour";
    if (id == ID_TRAY_2HOURS) return L"2 Hours";
    return L"";
}

void FormatTimeRemaining(wchar_t* dest, size_t maxCount, DWORD totalSeconds, const wchar_t* modeName) {
    if (maxCount == 0) return;

    DWORD h = totalSeconds / 3600;
    DWORD m = (totalSeconds % 3600) / 60;
    DWORD s = totalSeconds % 60;

    wchar_t* p = dest;
    wchar_t* end = dest + maxCount - 1;

    const wchar_t* prefix1 = L"Remaining: ";
    while (*prefix1 && p < end) *p++ = *prefix1++;
    
    if (h > 0) {
        if (h >= 10 && p < end) *p++ = L'0' + (h / 10);
        if (p < end) *p++ = L'0' + (h % 10);
        if (p < end) *p++ = L'h';
        if (p < end) *p++ = L' ';
    }
    
    if (m >= 10 && p < end) *p++ = L'0' + (m / 10);
    if (p < end) *p++ = L'0' + (m % 10);
    if (p < end) *p++ = L'm';
    if (p < end) *p++ = L' ';
    
    if (s >= 10 && p < end) *p++ = L'0' + (s / 10);
    if (p < end) *p++ = L'0' + (s % 10);
    if (p < end) *p++ = L's';
    
    if (p < end) *p++ = L'\n';
    
    const wchar_t* prefix2 = L"Caffeine: ";
    while (*prefix2 && p < end) *p++ = *prefix2++;
    
    while (*modeName && p < end) *p++ = *modeName++;
    
    *p = L'\0';
}

void SafeStringCopy(wchar_t* dest, const wchar_t* src, size_t maxCount) {
    if (maxCount == 0) return;
    size_t i = 0;
    while (src[i] && i < (maxCount - 1)) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = L'\0';
}

void CombinePath(wchar_t* dest, const wchar_t* folder, const wchar_t* file, size_t maxCount) {
    size_t i = 0;
    while (folder[i] && i < (maxCount - 1)) {
        dest[i] = folder[i];
        i++;
    }
    if (i > 0 && dest[i - 1] != L'\\' && i < (maxCount - 1)) {
        dest[i++] = L'\\';
    }
    size_t j = 0;
    while (file[j] && i < (maxCount - 1)) {
        dest[i++] = file[j++];
    }
    dest[i] = L'\0';
}

void EnsureStartMenuShortcut() {
    wchar_t startMenuPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, startMenuPath))) {
        return;
    }

    wchar_t shortcutPath[MAX_PATH];
    CombinePath(shortcutPath, startMenuPath, L"Caffeine.lnk", MAX_PATH);

    if (GetFileAttributesW(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
        return;
    }

    wchar_t currentExePath[MAX_PATH];
    GetModuleFileNameW(NULL, currentExePath, MAX_PATH);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    IShellLinkW* pShellLink = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink))) {
        pShellLink->SetPath(currentExePath);
        pShellLink->SetDescription(L"Prevent Windows from sleeping.");
        
        IPersistFile* pPersistFile = NULL;
        if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile))) {
            pPersistFile->Save(shortcutPath, TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    CoUninitialize();
}

void CleanUpAndExit(int exitCode) {
    if (nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }
    if (g_hMutex) {
        CloseHandle(g_hMutex);
    }
    ExitProcess(exitCode);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_TRAYICON) {
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            if (!hMenu) return 0;
            
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_DISABLED ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_DISABLED, L"Disabled (Allow Sleep)");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_5MIN ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_5MIN, L"5 Minutes");
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_15MIN ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_15MIN, L"15 Minutes");
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_30MIN ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_30MIN, L"30 Minutes");
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_1HOUR ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_1HOUR, L"1 Hour");
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_2HOURS ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_2HOURS, L"2 Hours");
            AppendMenuW(hMenu, MF_STRING | (g_ActiveMode == ID_TRAY_INDEFINITE ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_INDEFINITE, L"Indefinitely");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
            
            POINT pt;
            GetCursorPos(&pt);
            
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            PostMessageW(hWnd, WM_NULL, 0, 0); 
            
            DestroyMenu(hMenu);
        }
    }  
    else if (message == WM_COMMAND) {
        UINT id = LOWORD(wParam);
        if (id >= ID_TRAY_DISABLED && id <= ID_TRAY_INDEFINITE) {
            g_ActiveMode = id;
            KillTimer(hWnd, TIMER_ID);

            if (id == ID_TRAY_DISABLED) {
                SetThreadExecutionState(ES_CONTINUOUS);
                SafeStringCopy(nid.szTip, L"Caffeine: Disabled", sizeof(nid.szTip) / sizeof(wchar_t));
            } 
            else if (id == ID_TRAY_INDEFINITE) {
                SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
                SafeStringCopy(nid.szTip, L"Caffeine: Indefinitely", sizeof(nid.szTip) / sizeof(wchar_t));
            } 
            else {
                SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
                
                if (id == ID_TRAY_5MIN)   g_TimeRemainingSeconds = 5 * 60;
                if (id == ID_TRAY_15MIN)  g_TimeRemainingSeconds = 15 * 60;
                if (id == ID_TRAY_30MIN)  g_TimeRemainingSeconds = 30 * 60;
                if (id == ID_TRAY_1HOUR)  g_TimeRemainingSeconds = 60 * 60;
                if (id == ID_TRAY_2HOURS) g_TimeRemainingSeconds = 120 * 60;

                FormatTimeRemaining(nid.szTip, sizeof(nid.szTip) / sizeof(wchar_t), g_TimeRemainingSeconds, GetModeName(id));
                SetTimer(hWnd, TIMER_ID, 1000, NULL);
            }
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }
        else if (id == ID_TRAY_EXIT) {
            CleanUpAndExit(0);
        }
    } 
    else if (message == WM_TIMER) {
        if (wParam == TIMER_ID) {
            if (g_TimeRemainingSeconds > 0) {
                g_TimeRemainingSeconds--;
                FormatTimeRemaining(nid.szTip, sizeof(nid.szTip) / sizeof(wchar_t), g_TimeRemainingSeconds, GetModeName(g_ActiveMode));
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            } else {
                KillTimer(hWnd, TIMER_ID);
                g_ActiveMode = ID_TRAY_DISABLED;
                SetThreadExecutionState(ES_CONTINUOUS);
                SafeStringCopy(nid.szTip, L"Caffeine: Disabled (Time Expired)", sizeof(nid.szTip) / sizeof(wchar_t));
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            }
        }
    }
    else if (message == WM_DESTROY) {
        CleanUpAndExit(0);
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

extern "C" void __cdecl WinMainCRTStartup() {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    g_hMutex = CreateMutexW(NULL, TRUE, L"CaffeineProductionMutex");
    if (g_hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_hMutex) CloseHandle(g_hMutex);
        ExitProcess(0);
    }

    EnsureStartMenuShortcut();

    HICON hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = hIcon;
    wc.lpszClassName = L"Caffeine_Class";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(0, L"Caffeine_Class", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!hWnd) {
        CleanUpAndExit(0);
    }

    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hIcon;
    SafeStringCopy(nid.szTip, L"Caffeine: Active Indefinitely", sizeof(nid.szTip) / sizeof(wchar_t));

    Shell_NotifyIconW(NIM_ADD, &nid);
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CleanUpAndExit(0);
}