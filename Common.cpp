#include "stdafx.h"
#include "Common.h"

Localizer::Localizer() : oldLocaleName_(GetOldLocaleVal())
{
	const auto userLocaleName = getUserLocaleName();
	setLocale(userLocaleName);
}

Localizer::~Localizer()
{
	setLocale(oldLocaleName_);
}

std::wstring Localizer::GetOldLocaleVal()
{
	auto pOldLoc = _wsetlocale(LC_ALL, nullptr);
	if (!pOldLoc)
	{
		const auto eCode = errno;
		commonError(L"Getting a current locale failed", static_cast<DWORD>(eCode));
	}

	return pOldLoc;
}

std::wstring Localizer::getUserLocaleName()
{
	std::wstring localeName('\n', LOCALE_NAME_MAX_LENGTH);
	int localeSz = GetUserDefaultLocaleName(localeName.data(), static_cast<int>(localeName.size()));
	if (!localeSz)
	{
		const auto eCode = GetLastError();
		commonError(L"Getting user locale name failed", eCode);
	}

	return localeName;
}

void Localizer::setLocale(_In_ const std::wstring& localeName)
{
	auto loc = _wsetlocale(LC_ALL, localeName.c_str());
	if (!loc)
	{
		const auto eCode = errno;
		commonError(L"Setting locale failed", static_cast<DWORD>(eCode));
	}
}


void commonError(_In_ const std::wstring& msg, _In_ const DWORD eCode)
{
	throw CommonException(msg, eCode);
}


HANDLE createValidEvent(_In_ const BOOL bManualReset, _In_ const BOOL bInitialState)
{
	HANDLE hEvent = CreateEvent(nullptr, bManualReset, bInitialState, nullptr);
	if (!hEvent)
	{
		const DWORD eCode = GetLastError();
		commonError(L"CreateEvent failed.", eCode);
	}

	return hEvent;
}

HANDLE getValidStdHandle(_In_ const DWORD hStdHandle)
{
	HANDLE hStdIn = GetStdHandle(hStdHandle);
	if (INVALID_HANDLE_VALUE == hStdIn)
	{
		const DWORD eCode = GetLastError();
		commonError(L"GetStdHandle failed.", eCode);
	}

	return hStdIn;
}
