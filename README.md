# Caffeine

A lightweight, pure Win32 C++ utility that lives in the system tray and prevents Windows from entering sleep states or dimming the display. 

Unlike traditional sleep-prevention utilities that simulate synthetic hardware events (such as fake F15 keystrokes at timed intervals), Caffeine registers a native kernel-level flag directly with the Windows Power Manager. This allows the processor to remain in deep low-power C-states, maximizing battery efficiency.

---

## Performance Metrics

| Metric | Traditional Utilities | Caffeine (Pure Win32) |
| :--- | :--- | :--- |
| **CPU Usage** | Intermittent spikes (polling/simulation) | **0.00%** (Strictly event-driven) |
| **Memory (RAM)** | 12 MB – 50 MB | **~380 KB** |
| **Executable Size** | 150 KB – 5 MB | **~14 KB** (Standalone Installer) |
| **System Wakeups** | Regular intervals (forces CPU wake) | **None** (Passive OS assignment) |
| **Input Interference** | Risks disrupting macros or focus states | **None** |

---

## Key Features

* **Zero CPU Overhead:** Suspends application activity completely when idle, maintaining true 0% CPU usage to maximize laptop battery life.
* **Flexible Session Timers:** Right-click the icon to choose exactly how long your system stays awake: 5, 15, or 30 minutes, 1 or 2 hours, or indefinitely.
* **Live Countdown Tooltips:** Hover your mouse over the system tray icon to view a real-time countdown showing exactly when sleep prevention will turn off.
* **High-Visibility Silhouette Icon:** Features a bold, clean coffee cup design that scales perfectly on high-resolution screens and matches both light and dark Windows 11 themes.
* **Single Instance Protection:** Automatically detects if the application is already open to prevent multiple duplicate copies from running at the same time.

---

## Installation

1. Go to Releases page
2. Download "Setup.exe"
3. Run "Setup.exe"
4. Yay! :D

---

## Uninstallation

Run the provided `Uninstall.bat` script. The terminates the active memory process, unregisters the startup path from the registry hive, and completely purges the local storage directory.

---

## Compilation

Source code will be uploaded in an upcoming update.
