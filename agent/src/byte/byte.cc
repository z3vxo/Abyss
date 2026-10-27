#include "../../includes/byte/byte.h"
#include "../../includes/core/core.h"



BOOL Packager::NewWriter() {
	PBYTE Addr = hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, BASE_WRITER_SIZE);
	if (!Addr) { return FALSE; }
	this->WriteBuf = Addr;
	this->WriteIndex = 0;
	this->WriteBufSize = BASE_WRITER_SIZE;
	return TRUE;
}

BOOL Packager::Write4(INT32 val) {
	if (!this->Write4) { return FALSE; }
	if (this->WriteIndex + sizeof(val) > this->WriteBufSize) {
		this->EnsureBuf();
	}
	memcpy(this->WriteBuf + this->WriteIndex, val, sizeof(val));
	this->WriteIndex += sizeof(val);
	return TRUE;
}

BOOL Packager::WriteBytes(PBYTE Data, INT32 len) {
	if (!this->Write4) { return FALSE; }
	if (this->WriteIndex + len > this->WriteBufSize) {
		this->EnsureBuf();
	}
	memcpy(this->WriteBuf + this->WriteIndex, Data, len);
	this->WriteIndex += len;
	return TRUE;
}

BOOL Packager::DeleteWriter() {
	hollow->Win32.LocalFree(this->WriteBuf);
	return TRUE;
}

PBYTE Packager::WriterPointer() {
	return this->WriteBuf;
}

INT32 Packager::WriterSize() {
	return this->WriteBufSize;
}

Packager* packager = NULL;