#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define TIMER_ID 1

#define ID_TRAY_EXIT        1001
#define ID_TRAY_DISABLED    1002
#define ID_TRAY_5MIN        1003
#define ID_TRAY_15MIN       1004
#define ID_TRAY_30MIN       1005
#define ID_TRAY_1HOUR       1006
#define ID_TRAY_2HOURS      1007
#define ID_TRAY_INDEFINITE  1008

NOTIFYICONDATAW nid = { 0 };
UINT g_ActiveMode = ID_TRAY_INDEFINITE;
DWORD g_TimeRemainingSeconds = 0;

const wchar_t* GetModeName(UINT id) {
    if (id == ID_TRAY_5MIN) return L"5 Minutes";
    if (id == ID_TRAY_15MIN) return L"15 Minutes";
    if (id == ID_TRAY_30MIN) return L"30 Minutes";
    if (id == ID_TRAY_1HOUR) return L"1 Hour";
    if (id == ID_TRAY_2HOURS) return L"2 Hours";
    return L"";
}

void FormatTimeRemaining(wchar_t* dest, DWORD totalSeconds, const wchar_t* modeName) {
    DWORD h = totalSeconds / 3600;
    DWORD m = (totalSeconds % 3600) / 60;
    DWORD s = totalSeconds % 60;

    wchar_t* p = dest;
    
    const wchar_t* prefix1 = L"Remaining: ";
    while (*prefix1) *p++ = *prefix1++;
    
    if (h > 0) {
        if (h >= 10) *p++ = L'0' + (h / 10);
        *p++ = L'0' + (h % 10);
        *p++ = L'h';
        *p++ = L' ';
    }
    
    if (m >= 10) *p++ = L'0' + (m / 10);
    *p++ = L'0' + (m % 10);
    *p++ = L'm';
    *p++ = L' ';
    
    if (s >= 10) *p++ = L'0' + (s / 10);
    *p++ = L'0' + (s % 10);
    *p++ = L's';
    
    *p++ = L'\n';
    
    const wchar_t* prefix2 = L"Caffeine: ";
    while (*prefix2) *p++ = *prefix2++;
    
    while (*modeName) *p++ = *modeName++;
    
    *p = L'\0';
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_TRAYICON) {
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            
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
                lstrcpyW(nid.szTip, L"Caffeine: Disabled");
            } 
            else if (id == ID_TRAY_INDEFINITE) {
                SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
                lstrcpyW(nid.szTip, L"Caffeine: Indefinitely");
            } 
            else {
                SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
                
                if (id == ID_TRAY_5MIN)   g_TimeRemainingSeconds = 5 * 60;
                if (id == ID_TRAY_15MIN)  g_TimeRemainingSeconds = 15 * 60;
                if (id == ID_TRAY_30MIN)  g_TimeRemainingSeconds = 30 * 60;
                if (id == ID_TRAY_1HOUR)  g_TimeRemainingSeconds = 60 * 60;
                if (id == ID_TRAY_2HOURS) g_TimeRemainingSeconds = 120 * 60;

                FormatTimeRemaining(nid.szTip, g_TimeRemainingSeconds, GetModeName(id));
                SetTimer(hWnd, TIMER_ID, 1000, NULL);
            }
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }
        else if (id == ID_TRAY_EXIT) {
            Shell_NotifyIconW(NIM_DELETE, &nid);
            ExitProcess(0);
        }
    } 
    else if (message == WM_TIMER) {
        if (wParam == TIMER_ID) {
            if (g_TimeRemainingSeconds > 0) {
                g_TimeRemainingSeconds--;
                FormatTimeRemaining(nid.szTip, g_TimeRemainingSeconds, GetModeName(g_ActiveMode));
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            } else {
                KillTimer(hWnd, TIMER_ID);
                g_ActiveMode = ID_TRAY_DISABLED;
                SetThreadExecutionState(ES_CONTINUOUS);
                lstrcpyW(nid.szTip, L"Caffeine: Disabled (Time Expired)");
                Shell_NotifyIconW(NIM_MODIFY, &nid);
            }
        }
    }
    else if (message == WM_DESTROY) {
        Shell_NotifyIconW(NIM_DELETE, &nid);
        ExitProcess(0);
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

extern "C" void __cdecl WinMainCRTStartup() {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"CaffeineProductionMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ExitProcess(0);
    }

    HICON hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = hIcon;
    wc.lpszClassName = L"Caffeine_Class";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(0, L"Caffeine_Class", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!hWnd) {
        ExitProcess(0);
    }

    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hIcon;
    lstrcpyW(nid.szTip, L"Caffeine: Active Indefinitely");

    Shell_NotifyIconW(NIM_ADD, &nid);
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    ExitProcess(0);
}