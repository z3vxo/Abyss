#pragma once
#include "windows.h"
#include <stdio.h>
#include "nt.h"

#define DECL(x) decltype(x) * x


#ifdef _DEBUG
#define DEBUG_LOGA(msg, ...) printf(msg "\n", ##__VA_ARGS__)
#define DEBUG_LOGW(msg, ...) wprintf(msg L"\n", ##__VA_ARGS__)
#else
#define DEBUG_LOGA(msg, ...)
#define DEBUG_LOGW(msg, ...) 
#endif

#define OurProcess ((HANDLE)(LONG_PTR)-1)
#define TO_DWORD(x) ((DWORD)(ULONG_PTR)(x))

typedef struct {
    struct _MODULES {
        HMODULE k32;
        HMODULE ntdll;
        HMODULE winhttp;
        HMODULE advapi;
    } Modules;

    struct _WINAPIS {
        DECL(GetModuleFileNameA);
        DECL(GetUserNameA);
        DECL(GetComputerNameExA);
        DECL(GetTickCount);
        DECL(CloseHandle);
        DECL(LoadLibraryA);
        DECL(LocalAlloc);
        DECL(LocalFree);
        DECL(LocalReAlloc);
        DECL(GetProcAddress);
    } Win32;

    struct _NTAPIS {
        DECL(NtOpenProcessToken);
        DECL(NtQueryInformationToken);
        DECL(RtlGetNtVersionNumbers);
        DECL(RtlRandomEx);
        DECL(NtQueryInformationProcess);
    } ntapi;

}Hollow;

extern Hollow* hollow;


void HollowMain();
BOOL HollowInit();
BOOL RegisterAgent();
