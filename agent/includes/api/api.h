#pragma once
#include <windows.h>
#include "../../includes/core/nt.h"
#include "apidefs.h"




class API {
private:
	DWORD HasherA(PCHAR str);
	DWORD HasherW(PWCHAR str);
	PPEB GetPeb();
public:
	FARPROC GetProcAddress(HMODULE dll, DWORD hash);
	HMODULE GetModule(DWORD HASH);
};