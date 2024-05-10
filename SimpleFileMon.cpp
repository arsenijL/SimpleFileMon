#include "stdafx.h"
#include "ServiceManager.h"
#include "Common.h"


int wmain(_In_ int argc, _In_ const wchar_t* argv[])
{
	try
	{
		ServiceManager manager;
		const DWORD errorCode = manager.start(argc, argv);

		return static_cast<int>(errorCode);
	}
	catch (const CommonException& er)
	{
		std::wcerr << er.getMsg() << std::endl;
		
		return static_cast<int>(er.getCode());
	}
	catch (const std::exception& er)
	{
		std::wcerr << LPCWSTR(CA2W(er.what())) << std::endl;

		return static_cast<int>(ERROR_INTERNAL_ERROR);
	}
	catch (...)
	{
		std::wcerr << L"An unknown error occured." << std::endl;

		return static_cast<int>(ERROR_INTERNAL_ERROR);
	}
}