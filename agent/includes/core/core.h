#include "windows.h"

#define DECL(x) decltype(x) * x

typedef struct  {
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

        DECL(LocalAlloc);
        DECL(LocalFree);
        DECL(LocalReAlloc);
    } Win32;

    struct _NTAPIS {
        DECL(NtOpenProcessToken);
        DECL(NtQueryInformationToken);
        DECL(RtlGetVersionNumbers);
        DECL(RtlRandomEx);
    } ntapi;

}Hollow;

extern Hollow* hollow;


void HollowMain();
