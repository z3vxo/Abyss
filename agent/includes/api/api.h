#pragma once
#include <windows.h>
#include "../../includes/core/nt.h"
//#include "apidefs.h"



namespace api {
	namespace hashs {
		namespace dll {
			inline constexpr DWORD kernel32   = 0x7040EE75;
			inline constexpr DWORD ntdll      = 0x22D3B5ED;
			inline constexpr DWORD advapi32   = 0x67208A49;
		}

		namespace win32 {
			inline constexpr DWORD LocalAlloc 			= 0x73CEBC5B;
			inline constexpr DWORD LocalFree            = 0xA66DF372;
			inline constexpr DWORD LocalReAlloc 		= 0xABAD9DB2;
			inline constexpr DWORD GetComputerNameExA   = 0xD252A5F3;
			inline constexpr DWORD GetUserNameA 		= 0x9BC3AB46;
			inline constexpr DWORD GetModuleFileNameA   = 0x13B8A14D;
			inline constexpr DWORD GetTickCount 		= 0x41AD16B9;
			inline constexpr DWORD CloseHandle 			= 0x3870CA07;
			inline constexpr DWORD LoadLibraryA 		= 0x5FBFF0FB;
			inline constexpr DWORD GetProcAddress 		= 0xCF31BB1F;
			inline constexpr DWORD MoveFileA 			= 0xD834FDBD;
			inline constexpr DWORD GetLastError 		= 0x2082EAE3;
			inline constexpr DWORD CreateFileA 			= 0xEB96C5FA;
			inline constexpr DWORD ReadFile 			= 0x71019921;
			inline constexpr DWORD SetCurrentDirectoryA = 0xBEC3A06A;
			inline constexpr DWORD GetCurrentDirectoryA = 0x2CED73DE;

		}

		namespace winhttp {
			inline constexpr DWORD WinHttpOpen 								= 0x5E4F39E5;
			inline constexpr DWORD WinHttpConnect 							= 0x7242C17D;
			inline constexpr DWORD WinHttpCloseHandle 						= 0x36220CD5;
			inline constexpr DWORD WinHttpOpenRequest 						= 0xEAB7B9CE;
			inline constexpr DWORD WinHttpSendRequest 						= 0xB183FAA6;
			inline constexpr DWORD WinHttpReceiveResponse 					= 0x146C4925;
			inline constexpr DWORD WinHttpQueryDataAvailable 				= 0x34CB8684;
			inline constexpr DWORD WinHttpQueryHeaders 						= 0x389CEFA5;
			inline constexpr DWORD WinHttpReadData 							= 0x7195E4E9;
			inline constexpr DWORD WinHttpSetOption 						= 0xA18B94F8;
			inline constexpr DWORD WinHttpAddRequestHeaders 				= 0xED7FCB41;
			inline constexpr DWORD WinHttpGetIEProxyConfigForCurrentUser 	= 0x09C09B02;
			inline constexpr DWORD WinHttpGetProxyForUrl 					= 0xD2D47A4F;
			inline constexpr DWORD WinHttpDetectAutoProxyConfigUrl 			= 0xE471E230;
			inline constexpr DWORD WinHttpGetDefaultProxyConfiguration 		= 0xBCE2D8B2;
		}

		namespace ntdll {
			inline constexpr DWORD RtlRandomEx                 = 0x7C3439F5;
			inline constexpr DWORD RtlGetNtVersionNumbers 	   = 0x4803847B;
			inline constexpr DWORD NtQueryInformationProcess   = 0xD034FC62;
			inline constexpr DWORD NtOpenProcessToken          = 0x7BD07459;
			inline constexpr DWORD NtQueryInformationToken     = 0x2CE5A244;


		}
	}

	DWORD HasherA(PCHAR str);
	DWORD HasherW(PWCHAR str);
	FARPROC GetProcAddressInternal(HMODULE dll, DWORD hash, INT depth);
	FARPROC GetProcAddress(HMODULE dll, DWORD hash);
	HMODULE GetModule(DWORD HASH);
}


