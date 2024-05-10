#ifndef COMMON_H
#define COMMON_H

#ifndef Add2Ptr
#define Add2Ptr(P, I) ((PVOID)((PUCHAR)(P) + (I)))
#endif		//Add2Ptr


class CommonException
{
public:
	CommonException(_In_ const std::wstring& msg, _In_ const DWORD eCode) 
		: msg_(msg), eCode_(eCode)
	{
	}

	DWORD getCode() const
	{
		return eCode_;
	}

	const std::wstring& getMsg() const
	{
		return msg_;
	}

	virtual ~CommonException() {}

private:
	std::wstring msg_;
	DWORD eCode_{ERROR_SUCCESS};

};

class Localizer final
{
public:
	Localizer();

	~Localizer();

private:
	static std::wstring GetOldLocaleVal();

	static std::wstring getUserLocaleName();

	static void setLocale(_In_ const std::wstring& localeName);

private:
	std::wstring oldLocaleName_;

};

void commonError(_In_ const std::wstring& msg, _In_ const DWORD eCode);


HANDLE createValidEvent(_In_ const BOOL bManualReset, _In_ const BOOL bInitialState);

HANDLE getValidStdHandle(_In_ const DWORD hStdHandle);



#endif		//COMMON_H
