#include "../../includes/byte/byte.h"
#include "../../includes/core/core.h"
#include <string.h>




BOOL Packager::EnsureBuf(SIZE_T Needed) {
	if (!this->WriteBuf) { return FALSE; }
	if (Needed <= this->WriteBufSize) { return TRUE; }

	SIZE_T NewSize = this->WriteBufSize ? this->WriteBufSize : BASE_WRITER_SIZE;
	do {
		if (NewSize > ((SIZE_T)-1) / 2) { return FALSE; }
		NewSize *= 2;
	} while (NewSize < Needed);

	HLOCAL NewBuf = hollow->Win32.LocalReAlloc(this->WriteBuf, NewSize, LMEM_FIXED | LMEM_ZEROINIT);
	if (!NewBuf) { return FALSE; }

	this->WriteBuf = (PBYTE)NewBuf;
	this->WriteBufSize = NewSize;
	return TRUE;
}

BOOL Packager::NewWriter() {
	if (this->WriteBuf) { return FALSE; }

	PBYTE Addr = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BASE_WRITER_SIZE);
	if (!Addr) { return FALSE; }
	this->WriteBuf = Addr;
	this->WriteIndex = 0;
	this->WriteBufSize = BASE_WRITER_SIZE;
	return TRUE;
}

BOOL Packager::Write4(INT32 val) {
	if (!this->WriteBuf) { return FALSE; }

	if (!this->EnsureBuf(this->WriteIndex + sizeof(val))) { return FALSE; }

	memcpy(this->WriteBuf + this->WriteIndex, &val, sizeof(val));

	this->WriteIndex += sizeof(val);
	return TRUE;
}

BOOL Packager::WriteBytes(PBYTE Data, INT32 len) {
	if (!this->WriteBuf || !Data || len < 0) { return FALSE; }

	if (!this->EnsureBuf(this->WriteIndex + (SIZE_T)len)) { return FALSE; }

	memcpy(this->WriteBuf + this->WriteIndex, Data, len);
	this->WriteIndex += len;
	return TRUE;
}

BOOL Packager::DeleteWriter() {
	if (!this->WriteBuf) { return FALSE; }

	hollow->Win32.LocalFree(this->WriteBuf);
	this->WriteBuf = NULL;
	this->WriteIndex = 0;
	this->WriteBufSize = 0;
	return TRUE;
}

PBYTE Packager::WriterPointer() {
	return this->WriteBuf;
}

INT32 Packager::WriterSize() {
	return (INT32)this->WriteIndex;
}

Packager* packager = NULL;