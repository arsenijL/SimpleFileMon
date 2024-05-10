#include "stdafx.h"
#include "ServiceManager.h"
#include "OptNames.h"


DWORD ServiceManager::start(_In_ const int argc, _In_ const wchar_t* argv[])
{
	opts_ = std::make_shared<CLI_Utils::CmdLineParser>(argc, argv);

	serviceType_ = getServiceType();

	auto serviceToRun = createService();
	return serviceToRun->run();
}


std::unique_ptr<IService> ServiceManager::createService()
{
	static const std::map<Services, std::function<std::unique_ptr<IService>()>> services =
	{
		{Services::printHelp, []() { return std::make_unique<PrintHelpService>(); }},
		{Services::trackFile, [&]() { return std::make_unique<TrackFileService>(opts_); }},
		{Services::trackDir, [&]() { return std::make_unique<TrackDirService>(opts_); }},
		{Services::undefined, []() { return std::make_unique<WrongRequestService>(); }} };

	auto it = services.find(serviceType_);
	if (services.end() == it)
	{
		commonError(L"Unrecognized service type", ERROR_INVALID_PARAMETER);
	}

	return it->second();
}


ServiceManager::Services ServiceManager::getServiceType() const
{
	static const std::map<std::wstring, Services> serviceTags =
	{
		{ OPT_PRINT_HELP,		Services::printHelp },
		{ OPT_PRINT_HELP_LONG,	Services::printHelp },
		{ OPT_TRACK_DIR,		Services::trackDir },
		{ OPT_TRACK_DIR_LONG,	Services::trackDir },
		{ OPT_TRACK_FILE,		Services::trackFile },
		{ OPT_TRACK_FILE_LONG,	Services::trackFile } };

	Services service = Services::printHelp;
	size_t cmdTagsReaded = 0;
	std::for_each(
		serviceTags.begin(), 
		serviceTags.end(), 
		[&](const auto& tag) { if (opts_->optExists(tag.first)) { ++cmdTagsReaded; service = tag.second; }} );

	return (1 == cmdTagsReaded) ? 
		service : 
		Services::undefined;
}
