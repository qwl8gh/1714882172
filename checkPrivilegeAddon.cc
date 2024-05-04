#pragma comment(lib, "netapi32.lib")

#include <napi.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <string>
#include <vector>
#include <codecvt>
#include <locale>
#include <lm.h>

static std::string utf16ToUTF8( const std::wstring &s )
{
    const int size = ::WideCharToMultiByte( CP_UTF8, 0, s.c_str(), -1, NULL, 0, 0, NULL );
    std::vector<char> buf( size );
    ::WideCharToMultiByte( CP_UTF8, 0, s.c_str(), -1, &buf[0], size, 0, NULL );
    return std::string( &buf[0] );
}


bool CheckIfUserExists(const std::string& username) {
    DWORD sidSize = 0;
    DWORD domainSize = 0;
    SID_NAME_USE sidType;
    wchar_t* domain = nullptr;
    SID* sid = nullptr;

    if (!LookupAccountName(NULL, const_cast<char *>(username.c_str()), nullptr, &sidSize, nullptr, &domainSize, &sidType)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return false;
        }
    }

    sid = (SID*)malloc(sidSize);
    domain = (wchar_t*)malloc(domainSize * sizeof(wchar_t));
    if (!sid || !domain) {
        free(sid);
        free(domain);
        return false;
    }
    std::string utf8String = utf16ToUTF8( domain );

    if (!LookupAccountName(NULL, const_cast<char *>(username.c_str()), sid, &sidSize, const_cast<char *>(utf8String.c_str()), &domainSize, &sidType)) {
        free(sid);
        free(domain);
        return false;
    }

    free(sid);
    free(domain);
    return true;
}


Napi::String CheckUserPrivilege(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }

    std::string username = info[0].As<Napi::String>().Utf8Value();

    if (!CheckIfUserExists(username)) {
        return Napi::String::New(env, "UserNotFound");
    }

    USER_INFO_1* userInfo;
    DWORD level = 1;
    wchar_t wtext[20];
    mbstowcs(wtext, username.c_str(), username.length());
    LPWSTR ptr = wtext;

    std::string usernameUtf8 = info[0].As<Napi::String>().Utf8Value();
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring username1 = converter.from_bytes(usernameUtf8);

    NET_API_STATUS status = NetUserGetInfo(NULL, username1.c_str(), level, (LPBYTE*)&userInfo);
    if (status != NERR_Success) {
        return Napi::String::New(env, "Error");
    }

    std::string privilege;
    switch (userInfo->usri1_priv) {
        case USER_PRIV_ADMIN:
            privilege = "Administrator";
            break;
        case USER_PRIV_USER:
            privilege = "User";
            break;
        case USER_PRIV_GUEST:
            privilege = "Guest";
            break;
        default:
            privilege = "Unknown";
            break;
    }

    NetApiBufferFree(userInfo);

    return Napi::String::New(env, const_cast<char *>(privilege.c_str()));
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "checkUserPrivilege"), Napi::Function::New(env, CheckUserPrivilege));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)