#pragma once
#include <Windows.h>

#define BASE_WRITER_SIZE 256

class Packager {
private:
	PBYTE WriteBuf;
	SIZE_T WriteIndex;
	SIZE_T WriteBufSize;

	PBYTE ReadBuf;
	SIZE_T ReadIndex;
	SIZE_T ReadBufSize;

public:
	/*BOOL NewReader(PBYTE buf, SIZE_T bufSize);
	INT32 Read4();
	PBYTE ReadBytes(INT32 len);*/

	BOOL NewWriter();
	BOOL Write4(INT32 val);
    BOOL WriteBytes(PBYTE Data, INT32 len);
	BOOL DeleteWriter();
	PBYTE WriterPointer();
	INT32 WriterSize();

};

extern Packager* packager;