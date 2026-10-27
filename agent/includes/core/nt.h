#pragma once
#include <windows.h>


typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaxiumLength;
	PWSTR Buffer;
}UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA {
	ULONG Len;
	BOOLEAN Intitalised;
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitialiszationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutDownInProgress;
	HANDLE ShutdownThreadID;
}PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryBase;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
}LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB {
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN BitFields;
	HANDLE Mutant;
	PVOID ImageBase;
	PPEB_LDR_DATA Ldr;
}PEB, * PPEB;