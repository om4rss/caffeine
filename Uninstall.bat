@echo off
title Caffeine Uninstaller

echo [1/3] Terminating any active Caffeine processes...
tasklist /fi "imagename eq Caffeine.exe" 2>nul | find /i "Caffeine.exe" >nul
if %errorlevel% equ 0 (
    taskkill /f /im Caffeine.exe >nul 2>&1
    echo Status: Active process terminated successfully.
) else (
    echo Status: No active process found running.
)
echo.

echo [2/3] Removing system startup registration...
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "Caffeine" >nul 2>&1
if %errorlevel% equ 0 (
    reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "Caffeine" /f >nul 2>&1
    echo Status: Registry startup key removed successfully.
) else (
    echo Status: No startup entry found in registry.
)
echo.

echo [3/3] Purging local application directory...
if exist "%LOCALAPPDATA%\Caffeine" (
    rmdir /s /q "%LOCALAPPDATA%\Caffeine"
    echo Status: Application directory deleted from LocalAppData.
) else (
    echo Status: Application directory not found on disk.
)
echo.

echo --------------------------------------------------
echo Caffeine has been completely uninstalled.
echo --------------------------------------------------
echo.
pause