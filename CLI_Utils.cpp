#include "stdafx.h"
#include "CLI_Utils.h"
#include "Common.h"

#define OPTS_SIGN L'-'

const std::wstring CLI_Utils::CmdLineParser::globalArgsOptName_ = L"--global-args";


CLI_Utils::CmdLineParser::CmdLineParser(_In_ const int argc, _In_ const wchar_t* argv[])
{
	if (nullptr == argv || 0 >= argc)
	{
		commonError(L"Command line is incorrect.", ERROR_INVALID_PARAMETER);
	}

	parseCmdLineArgs(argc, argv);
}

template<typename StrType>
inline bool isOpt(StrType str)
{
	//предусловие: в случае, если str - указатель, то на вход подается валидный указатель (проверка указателя на командную строку проводится до вызова этой функции)

	return OPTS_SIGN == str[0];
}

CLI_Utils::CmdLineOpts::iterator getIterToNewOpt(_Inout_ CLI_Utils::CmdLineOpts& cmdLineOpts, _In_ const std::wstring& optName);

void CLI_Utils::CmdLineParser::parseCmdLineArgs(_In_ const int argc, _In_ const wchar_t* argv[])
{
	//Предусловие: проверка на валидность переменных argc и argv проходит перед вызовом данной функции (на данный момент в конструкторе)

	auto curOpt = getIterToNewOpt(cmdLineOpts_, globalArgsOptName_);		//аргументы, введеные после пути программы, считаем "глобальными"

	int indexToRead = 1;
	while (indexToRead != argc)
	{
		if (isOpt(argv[indexToRead])) 
		{
			curOpt = getIterToNewOpt(cmdLineOpts_, argv[indexToRead]);
		} 
		else 
		{
			curOpt->second.emplace_back(argv[indexToRead]);
		}

		++indexToRead;
	}
}

const CLI_Utils::CmdLineArgs& CLI_Utils::CmdLineParser::getArgs(_In_ const std::wstring& optName) const
{
	const auto& it = cmdLineOpts_.find(optName);
	if (cmdLineOpts_.cend() == it)
	{
		commonError(L"Undefined option " + optName + L".", ERROR_INVALID_PARAMETER);
	}

	return it->second;
}

const CLI_Utils::CmdLineArgs& CLI_Utils::CmdLineParser::getGlobalArgs() const
{
	return getArgs(globalArgsOptName_);
}

const CLI_Utils::CmdLineOpts& CLI_Utils::CmdLineParser::getAllOpts() const
{
	return cmdLineOpts_;
}

bool CLI_Utils::CmdLineParser::optExists(_In_ const std::wstring& optName) const
{
	return cmdLineOpts_.end() != cmdLineOpts_.find(optName);
}

const std::wstring& CLI_Utils::CmdLineParser::getOnlyArg(_In_ const std::wstring& optName) const
{
	const auto& argsList = getArgs(optName);
	if (argsList.size() != 1)
	{
		commonError(L"Option " + optName + L" requires only one option.", ERROR_INVALID_PARAMETER);
	}

	return *argsList.begin();
}

CLI_Utils::CmdLineOpts::iterator getIterToNewOpt(_Inout_ CLI_Utils::CmdLineOpts& cmdLineOpts, _In_ const std::wstring& optName)
{
	if (cmdLineOpts.find(optName) != cmdLineOpts.end())
	{
		commonError(L"Option " + optName + L" has already been added.", ERROR_INVALID_PARAMETER);
	}

	cmdLineOpts.emplace(optName, CLI_Utils::CmdLineArgs());		
	const auto& it = cmdLineOpts.find(optName);
	if (cmdLineOpts.end() == it)
	{
		commonError(L"Option " + optName + L" have not found.", ERROR_INVALID_PARAMETER);
	}

	return it;
}

const std::wstring& CLI_Utils::CmdLineParser::getGlobalOptName()
{
	return globalArgsOptName_;
}
