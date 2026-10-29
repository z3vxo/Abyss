#pragma once
#include <windows.h>

#define BASE_WRITER_SIZE 256

class Packager {
private:
	PBYTE WriteBuf;
	SIZE_T WriteIndex;
	SIZE_T WriteBufSize;

	PBYTE ReadBuf;
	SIZE_T ReadIndex;
	SIZE_T ReadBufSize;

	BOOL EnsureBuf(SIZE_T Needed);

public:
	BOOL NewParser(PBYTE data, INT32 size);
	INT32 Parse4();
	BOOL ParseBytes(void* out, INT32 len);

	BOOL NewPacker();
	BOOL Pack4(INT32 val);
	BOOL PackBytes(PBYTE Data, INT32 len);
	BOOL DeletePacker();
	PBYTE PackerPointer();
	INT32 PackerSize();
	void ResetPacker();
};

extern Packager* packager;
