#pragma once
#include <windows.h>



namespace hash {
	namespace dll {
		inline constexpr DWORD kernel32  = 0x7040EE75;
		inline constexpr DWORD ntdll     = 0x22D3B5ED;
		inline constexpr DWORD advapi32  = 0x67208A49;
	}

	namespace win32 {
		inline constexpr DWORD LocalAlloc = 0x73CEBC5B;
		inline constexpr DWORD LocalFree = 0xA66DF372;
		inline constexpr DWORD LocalReAlloc = 0xABAD9DB2;
	}
}
