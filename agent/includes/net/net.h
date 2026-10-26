#include <winhttp.h>


#define DECL(x) decltype(x) * x

typedef struct {
    DECL(WinHttpOpen);
    DECL(WinHttpConnect);
    DECL(WinHttpCloseHandle);
    DECL(WinHttpOpenRequest);
    DECL(WinHttpSendRequest);
    DECL(WinHttpReceiveResponse);
    DECL(WinHttpQueryDataAvailable);
    DECL(WinHttpQueryHeaders);
    DECL(WinHttpReadData);
    DECL(WinHttpSetOption);
    DECL(WinHttpAddRequestHeaders);
    DECL(WinHttpGetIEProxyConfigForCurrentUser);
    DECL(WinHttpGetProxyForUrl);
    DECL(WinHttpDetectAutoProxyConfigUrl);
    DECL(WinHttpGetDefaultProxyConfiguration);
} WINHTTP;

class NetMgr {
    WINHTTP *httpApis;


    public:
        void Init();
        BOOL DoPost();


};


extern NetMgr* g_NetMgr;
