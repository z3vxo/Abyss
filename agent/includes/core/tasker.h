#pragma once
#include <windows.h>


#define TASK_FAILED 5
#define OUTPUT_RAW  4
#define OUTPUT_NEEDS_PARSE 3

#define PARSE_WHOAMI 0xac


// server holds success code -> string mappings
#define MV_SUCCESS  6

BOOL Dispatch();
void do_whoami();
void do_mv();