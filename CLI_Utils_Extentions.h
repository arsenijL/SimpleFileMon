#ifndef CLI_UTILS_EXTENTIONS_H
#define CLI_UTILS_EXTENTIONS_H

#include "CLI_Utils.h"
#include "Common.h"


template<typename... T>
void validOptsCheck(_In_ const std::shared_ptr<CLI_Utils::CmdLineParser>& opts, _In_ const T... optNames)		// TODO: пересмотреть название и в целом реализацию данной функции. Выглядит костыльно, если честно.
{
	if (!opts->getGlobalArgs().empty())
	{
		commonError(L"Global arguments are not allowed. Enter <path>\\SimpleFileMon.exe -h to see help menu.", ERROR_INVALID_PARAMETER);
	}

	std::set<std::wstring> validOptNames = { CLI_Utils::CmdLineParser::getGlobalOptName(), optNames...};

	std::for_each(
		opts->getAllOpts().begin(),
		opts->getAllOpts().end(),
		[&validOptNames](const auto& el) {
			if (validOptNames.find(el.first) == validOptNames.end()) 
				commonError(
					L"Unrecognized option: " + el.first + 
					L". Enter <path>\\SimpleFileMon.exe -h to see help menu.",
					ERROR_INVALID_PARAMETER		);
		} );
}

template<typename... T>
CLI_Utils::CmdLineOpts::const_iterator getArgsForOneOfOptsVariant(_In_ const CLI_Utils::CmdLineOpts& opts, _In_ const T... optNames)
{
	std::set<std::wstring> optVariants = { optNames... };

	size_t enteredVariantsCnt = 0;
	auto optIt = opts.begin();
	for (auto it = opts.begin(); it != opts.end(); ++it)
	{
		if (optVariants.find(it->first) != optVariants.end())
		{
			optIt = it;
			++enteredVariantsCnt;
		}
	}

	if (!enteredVariantsCnt)
	{
		commonError(L"Required option missing. Enter <path>\\SimpleFileMon.exe -h to see help menu.", ERROR_INVALID_PARAMETER);
	}

	if (enteredVariantsCnt > 1)
	{
		commonError(
			L"You can use only one variant of option. Enter <path>\\SimpleFileMon.exe -h to see help menu.", 
			ERROR_INVALID_PARAMETER	);
	}

	return optIt;
}

template<typename... T>
std::wstring getOnlyArgForOneOfOptsVariant(_In_ const CLI_Utils::CmdLineOpts& opts, _In_ const T... optNames)
{
	const auto& optArgs = getArgsForOneOfOptsVariant(opts, optNames...);

	if (optArgs->second.size() > 1)
	{
		commonError(
			L"Option " + optArgs->first + L" requre only one argument. Enter <path>\\SimpleFileMon.exe -h to see help menu.",
			ERROR_INVALID_PARAMETER	);
	}

	return *(optArgs->second.begin());
}

template<typename... T>
bool oneOfSwitchVariantExists(_In_ const CLI_Utils::CmdLineOpts& opts, _In_ const T... optNames)
{
	std::set<std::wstring> switchVariants = { optNames... };

	size_t enteredVariantsCnt = 0;
	auto optIt = opts.begin();
	for (auto it = opts.begin(); it != opts.end(); ++it)
	{
		if (switchVariants.find(it->first) != switchVariants.end())
		{
			optIt = it;
			++enteredVariantsCnt;
		}
	}

	if (!enteredVariantsCnt)
	{
		return false;
	}

	if (enteredVariantsCnt > 1)
	{
		commonError(L"You can use only one variant of switch. Enter <path>\\SimpleFileMon.exe -h to see help menu.", ERROR_INVALID_PARAMETER);
	}

	if (!optIt->second.empty())
	{
		commonError(L"Switch " + optIt->first + L" hasn't arguments. Enter <path>\\SimpleFileMon.exe -h to see help menu.", ERROR_INVALID_PARAMETER);
	}

	return true;
}


#endif		//CLI_UTILS_EXTENTIONS_H
