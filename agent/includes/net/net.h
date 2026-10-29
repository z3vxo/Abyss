#pragma once
#include <windows.h>
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
    WINHTTP* httpApis;
    HMODULE WinHttp;

    PBYTE  recvBuf;
    SIZE_T recvCap;
    SIZE_T recvLen;

    BOOL EnsureRecv(SIZE_T needed);


public:
    BOOL Init();
    BOOL DoPost(PBYTE data, SIZE_T len);

    PBYTE  RecvBuf()  { return this->recvBuf; }
    SIZE_T RecvSize() { return this->recvLen; }

};


extern NetMgr* g_NetMgr;
