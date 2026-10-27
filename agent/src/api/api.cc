#include "../../includes/api/api.h"


#define HASH_SEED 5381

DWORD API::HasherA(PCHAR str) {
	UINT32 hash = HASH_SEED;
	while (*str) {
		hash = ((hash << 5) + hash) + (unsigned char)*str++;
	}
	return hash;
}

DWORD API::HasherW(PWCHAR str) {
	UINT32 hash = HASH_SEED;
	while (*str) {
		unsigned char c = (unsigned char)*str++;
		if (c >= 'A' && c <= 'Z')[
			c += 0x20;
		]
			hash = ((hash << 5) + hash) + c;
	}
	return hash;
}


PPEB API::GetPeb() {
#if defined(_WIN64) || defined(__x86_64__)
	return (PPEB)__readgsqword(0x60);
#elif defined(_M_IX86)|| defined(__i386__)
	return (PPEB)__readfsdword(0x30);
#endif
}

FARPROC API::GetProcAddress(HMODULE dll, DWORD Hash) {


	return NULL;
}


HMODULE API::GetModule(DWORD Hash) {
	PPEB p = this->GetPeb();
	PEB_LDR_DATA* Ldr = p->Ldr;
	LIST_ENTRY* modules = &Ldr->InMemoryOrderModuleList;
	LIST_ENTRY* start = modules->Flink;

	for (LIST_ENTRY* List = start; List != modules; List = List->Flink) {
		LDR_DATA_TABLE_ENTRY* entry = (LDR_DATA_TABLE_ENTRY*)((BYTE*)List - sizeof(LIST_ENTRY));
		if (this->HasherW(entry->BaseDllName.Buffer) == Hash) {
			return (HMODULE)entry->DllBase;
		}
	}
	
	return NULL;
}