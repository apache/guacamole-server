#include "channels/rdpdr/msz-unicode.h"

#include <winpr/crt.h>
#include <winpr/stream.h>
#include <winpr/wtypes.h>

SSIZE_T ConvertMszWCharNToUtf8(const WCHAR* wstr, size_t wlen, char* str, size_t len)
{
	if (wlen == 0)
		return 0;

	WINPR_ASSERT(wstr);

	if ((len > INT32_MAX) || (wlen > INT32_MAX))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	const int iwlen = (int)len;
	const int rc = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)wlen, str, iwlen, NULL, NULL);
	if ((rc <= 0) || ((len > 0) && (rc > iwlen)))
		return -1;

	return rc;
}

char* ConvertMszWCharNToUtf8Alloc(const WCHAR* wstr, size_t wlen, size_t* pUtfCharLength)
{
	char* tmp = NULL;
	const SSIZE_T rc = ConvertMszWCharNToUtf8(wstr, wlen, NULL, 0);

	if (pUtfCharLength)
		*pUtfCharLength = 0;
	if (rc < 0)
		return NULL;
	tmp = calloc((size_t)rc + 1ull, sizeof(char));
	if (!tmp)
		return NULL;
	const SSIZE_T rc2 = ConvertMszWCharNToUtf8(wstr, wlen, tmp, (size_t)rc + 1ull);
	if (rc2 < 0)
	{
		free(tmp);
		return NULL;
	}
	WINPR_ASSERT(rc == rc2);
	if (pUtfCharLength)
		*pUtfCharLength = (size_t)rc2;
	return tmp;
}


SSIZE_T ConvertMszUtf8NToWChar(const char* str, size_t len, WCHAR* wstr, size_t wlen)
{
	if (len == 0)
		return 0;

	WINPR_ASSERT(str);

	if ((len > INT32_MAX) || (wlen > INT32_MAX))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	const int iwlen = (int)wlen;
	const int rc = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, wstr, iwlen);
	if ((rc <= 0) || ((wlen > 0) && (rc > iwlen)))
		return -1;

	return rc;
}

WCHAR* ConvertMszUtf8NToWCharAlloc(const char* str, size_t len, size_t* pSize)
{
	WCHAR* tmp = NULL;
	const SSIZE_T rc = ConvertMszUtf8NToWChar(str, len, NULL, 0);
	if (pSize)
		*pSize = 0;
	if (rc < 0)
		return NULL;
	tmp = calloc((size_t)rc + 1ull, sizeof(WCHAR));
	if (!tmp)
		return NULL;
	const SSIZE_T rc2 = ConvertMszUtf8NToWChar(str, len, tmp, (size_t)rc + 1ull);
	if (rc2 < 0)
	{
		free(tmp);
		return NULL;
	}
	WINPR_ASSERT(rc == rc2);
	if (pSize)
		*pSize = (size_t)rc2;
	return tmp;
}

SSIZE_T ConvertUtf8ToWChar(const char* str, WCHAR* wstr, size_t wlen)
{
	if (!str)
	{
		if (wstr && wlen)
			wstr[0] = 0;
		return 0;
	}

	const size_t len = strlen(str);
	return ConvertUtf8NToWChar(str, len + 1, wstr, wlen);
}

const WCHAR* InitializeConstWCharFromUtf8(const char* str, WCHAR* buffer, size_t len)
{
	WINPR_ASSERT(str);
	WINPR_ASSERT(buffer || (len == 0));
	(void)ConvertUtf8ToWChar(str, buffer, len);
	return buffer;
}

SSIZE_T ConvertUtf8NToWChar(const char* str, size_t len, WCHAR* wstr, size_t wlen)
{
	size_t ilen = strnlen(str, len);
	BOOL isNullTerminated = FALSE;
	if (len == 0)
		return 0;

	WINPR_ASSERT(str);

	if ((len > INT32_MAX) || (wlen > INT32_MAX))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}
	if (ilen < len)
	{
		isNullTerminated = TRUE;
		ilen++;
	}

	const int iwlen = (int)wlen;
	const int rc = MultiByteToWideChar(CP_UTF8, 0, str, (int)ilen, wstr, iwlen);
	if ((rc <= 0) || ((wlen > 0) && (rc > iwlen)))
		return -1;
	if (!isNullTerminated)
	{
		if (wstr && (rc < iwlen))
			wstr[rc] = '\0';
		return rc;
	}
	else if (rc == iwlen)
	{
		if (wstr && (wstr[rc - 1] != '\0'))
			return rc;
	}
	return rc - 1;
}

SSIZE_T ConvertWCharToUtf8(const WCHAR* wstr, char* str, size_t len)
{
	if (!wstr)
	{
		if (str && len)
			str[0] = 0;
		return 0;
	}

	const size_t wlen = _wcslen(wstr);
	return ConvertWCharNToUtf8(wstr, wlen + 1, str, len);
}

SSIZE_T ConvertWCharNToUtf8(const WCHAR* wstr, size_t wlen, char* str, size_t len)
{
	BOOL isNullTerminated = FALSE;
	if (wlen == 0)
		return 0;

	WINPR_ASSERT(wstr);
	size_t iwlen = _wcsnlen(wstr, wlen);

	if ((len > INT32_MAX) || (wlen > INT32_MAX))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	if (iwlen < wlen)
	{
		isNullTerminated = TRUE;
		iwlen++;
	}
	const int rc = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)iwlen, str, (int)len, NULL, NULL);
	if ((rc <= 0) || ((len > 0) && ((size_t)rc > len)))
		return -1;
	else if (!isNullTerminated)
	{
		if (str && ((size_t)rc < len))
			str[rc] = '\0';
		return rc;
	}
	else if ((size_t)rc == len)
	{
		if (str && (str[rc - 1] != '\0'))
			return rc;
	}
	return rc - 1;
}
