#pragma once

#include <winpr/crt.h>
#include <winpr/stream.h>

SSIZE_T ConvertMszWCharNToUtf8(const WCHAR* wstr, size_t wlen, char* str, size_t len);
char* ConvertMszWCharNToUtf8Alloc(const WCHAR* wstr, size_t wlen, size_t* pUtfCharLength);
SSIZE_T ConvertMszUtf8NToWChar(const char* str, size_t len, WCHAR* wstr, size_t wlen);
WCHAR* ConvertMszUtf8NToWCharAlloc(const char* str, size_t len, size_t* pSize);

const WCHAR* InitializeConstWCharFromUtf8(const char* str, WCHAR* buffer, size_t len);
SSIZE_T ConvertUtf8ToWChar(const char* str, WCHAR* wstr, size_t wlen);
SSIZE_T ConvertUtf8NToWChar(const char* str, size_t len, WCHAR* wstr, size_t wlen);

SSIZE_T ConvertWCharToUtf8(const WCHAR* wstr, char* str, size_t len);
SSIZE_T ConvertWCharNToUtf8(const WCHAR* wstr, size_t wlen, char* str, size_t len);
