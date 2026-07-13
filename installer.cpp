#include <windows.h>

extern "C" void __cdecl WinMainCRTStartup() {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    wchar_t localAppData[MAX_PATH] = { 0 };
    GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);

    wchar_t targetDir[MAX_PATH] = { 0 };
    lstrcpyW(targetDir, localAppData);
    lstrcatW(targetDir, L"\\Caffeine");

    wchar_t targetExe[MAX_PATH] = { 0 };
    lstrcpyW(targetExe, targetDir);
    lstrcatW(targetExe, L"\\Caffeine.exe");

    CreateDirectoryW(targetDir, NULL);

    // Fixed: Replaced RT_RCDATA with MAKEINTRESOURCEW(10) for proper wide-string matching
    HRSRC hResource = FindResourceW(hInstance, MAKEINTRESOURCEW(100), MAKEINTRESOURCEW(10));
    if (!hResource) {
        MessageBoxW(NULL, L"Installation payload corrupted.", L"Error", MB_ICONERROR);
        ExitProcess(0);
    }

    HGLOBAL hLoadedResource = LoadResource(hInstance, hResource);
    DWORD exeSize = SizeofResource(hInstance, hResource);
    LPVOID pExeBytes = LockResource(hLoadedResource);

    HANDLE hFile = CreateFileW(targetExe, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBoxW(NULL, L"Failed to write application files.", L"Error", MB_ICONERROR);
        ExitProcess(0);
    }

    DWORD bytesWritten;
    WriteFile(hFile, pExeBytes, exeSize, &bytesWritten, NULL);
    CloseHandle(hFile);

    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (lRes == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"Caffeine", 0, REG_SZ, (BYTE*)targetExe, (lstrlenW(targetExe) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(targetExe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    MessageBoxW(NULL, L"Caffeine installed successfully and configured to run at startup.", L"Success", MB_ICONINFORMATION);
    ExitProcess(0);
}