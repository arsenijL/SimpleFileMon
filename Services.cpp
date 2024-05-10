#include "stdafx.h"
#include "CLI_Utils.h"
#include "CLI_Utils_Extentions.h"
#include "Services.h"
#include "OptNames.h"
#include "Tracker.h"

namespace
{
	namespace fs = std::filesystem;

	void printHelpMenu()
	{
		std::wcout <<
			L"SimpleFileMon - utility for monitoring file system." << std::endl <<
			L"Command line format: <path>\\SimpleFileMon.exe [-opt1] <argument> [-opt2] [-opt3]" << std::endl <<
			L"Note: Options order is not considered." << std::endl << std::endl <<
			L"Basic commands:" << std::endl <<
			L"-d (--dir-path) - directory monitoring (the only arg is directory path)." << std::endl <<
			L"-f (--file-path) - specific file monitoring (the only arg is file path)." << std::endl <<
			L"-h (--help) - print help menu." << std::endl << std::endl <<
			L"Additional common switches:" << std::endl <<
			L"-e (--extended-info) - displaying extended information about monitored objects state." << std::endl << std::endl <<
			L"Additional switches for the -d (--dir-path) option:" << std::endl <<
			L"-s (--subtrees-obs) - tracking in all subdirectories of any depth." << std::endl << std::endl <<
			L"Usage example:" << std::endl <<
			L".\\SimpleFileMon.exe -d \"C:\\\" -s --extended-info" << std::endl << std::endl;
	}

	__if_not_exists(RtlGetVersion)
	{
		extern "C" LONG WINAPI RtlGetVersion(
			_Out_ POSVERSIONINFOW lpVersionInformation
		);		//функция недокументирована
	}

	bool osVerIsCompatible()
	{
		OSVERSIONINFO osver = {};	//можно занулить и с помощью ZeroMemory
		osver.dwOSVersionInfoSize = sizeof(osver);

		const auto status = RtlGetVersion(&osver);
		if (((NTSTATUS)0x00000000L) != status)
		{
			commonError(L"RtlGetVersion failed", status);
		}

		constexpr ULONG buildNumberRS3 = 16299UL;

		return osver.dwBuildNumber >= buildNumberRS3;
	}

}	//namespace



DWORD PrintHelpService::run()
{
	printHelpMenu();

	return ERROR_SUCCESS;
}



DWORD WrongRequestService::run()
{
	std::wcout << L"Entered parameters are wrong. Please see help menu." << std::endl << std::endl;

	printHelpMenu();

	return ERROR_INVALID_PARAMETER;
}



TrackFileService::TrackFileService(_In_ const std::shared_ptr<CLI_Utils::CmdLineParser>& opts)
{
	if (!osVerIsCompatible())
	{
		commonError(L"The minimum OS version for the utility to work correctly is Windows 10, version 1709", ERROR_NOT_SUPPORTED);
	}

	validOptsCheck(	opts,
					OPT_TRACK_FILE,
					OPT_TRACK_FILE_LONG,
					SWITCH_EXTENDED_INFO,
					SWITCH_EXTENDED_INFO_LONG	);

	fileFullPath_ = getOnlyArgForOneOfOptsVariant(opts->getAllOpts(), OPT_TRACK_FILE, OPT_TRACK_FILE_LONG);

	validateFilePath(fileFullPath_);

	std::error_code eCode;
	fileFullPath_ = fs::canonical(fileFullPath_, eCode);
	if (ERROR_SUCCESS != eCode.value())
	{
		commonError(L"Canonical path to the file could not be obtained.", eCode.value());
	}


	infoType_ = oneOfSwitchVariantExists(opts->getAllOpts(), SWITCH_EXTENDED_INFO, SWITCH_EXTENDED_INFO_LONG) ?
		READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyExtendedInformation :
		READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation;
}

void TrackFileService::validateFilePath(_In_ const std::filesystem::path& filePath)
{
	std::wstring errorMessage;
	if (!fs::exists(filePath))
	{
		errorMessage = L"File <" + static_cast<std::wstring>(filePath) +
			L"> is not found in your system. Check that the file path is correct and try again.";
		commonError(errorMessage, ERROR_FILE_NOT_FOUND);
	}

	if (fs::is_directory(filePath))
	{
		errorMessage = L"Option " + std::wstring(OPT_TRACK_FILE) + L" (or " + OPT_TRACK_FILE_LONG +
			L") doesn't accept the directory path. Check that the file path is correct and try again.";
		commonError(errorMessage, ERROR_INVALID_PARAMETER);
	}
}

DWORD TrackFileService::run()
{
	auto tracker = makeFileTracker();

	std::wcout << L"Request to track file <" << static_cast<std::wstring>(fileFullPath_) <<
		L"> has been approved." << std::endl <<
		L"You can interrupt the wait process by pressing any key." << std::endl;

	tracker->beginTrack(false);

	return ERROR_SUCCESS;
}

std::unique_ptr<INotificationsManager> TrackFileService::makeNotificationsFilter() const
{
	switch (infoType_)
	{
	case READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation:
		return std::make_unique<DefaultNotificationsFilter>(fileFullPath_, fileFullPath_.parent_path());

	case READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyExtendedInformation:
		return std::make_unique<ExtendedNotificationsFilter>(fileFullPath_, fileFullPath_.parent_path());

	default:
		commonError(L"Undefined type of receiving information.", ERROR_INVALID_PARAMETER);
		break;

	}

	return nullptr;		//до сюда выполнение никогда не дойдет (добавил для статических анализаторов)
}

std::unique_ptr<Tracker> TrackFileService::makeFileTracker() const
{
	auto notificationsFilter = makeNotificationsFilter();
	return std::make_unique<Tracker>(static_cast<std::wstring>(fileFullPath_.parent_path()), notificationsFilter);
}



TrackDirService::TrackDirService(_In_ const std::shared_ptr<CLI_Utils::CmdLineParser>& opts)
{
	if (!osVerIsCompatible())
	{
		commonError(L"The minimum OS version for the utility to work correctly is Windows 10, version 1709", ERROR_NOT_SUPPORTED);
	}

	validOptsCheck(opts,
		OPT_TRACK_DIR,
		OPT_TRACK_DIR_LONG,
		SWITCH_EXTENDED_INFO,
		SWITCH_EXTENDED_INFO_LONG,
		SWITCH_SUBTREE_TRACK,
		SWITCH_SUBTREE_TRACK_LONG);

	dirFullPath_ = getOnlyArgForOneOfOptsVariant(opts->getAllOpts(), OPT_TRACK_DIR, OPT_TRACK_DIR_LONG);

	validateDirPath(dirFullPath_);

	std::error_code eCode;
	dirFullPath_ = fs::canonical(dirFullPath_, eCode);
	if (ERROR_SUCCESS != eCode.value())
	{
		commonError(L"Canonical path to the directory could not be obtained.", eCode.value());
	}

	needSubtreesCheck_ = oneOfSwitchVariantExists(opts->getAllOpts(), SWITCH_SUBTREE_TRACK, SWITCH_SUBTREE_TRACK_LONG);

	infoType_ = oneOfSwitchVariantExists(opts->getAllOpts(), SWITCH_EXTENDED_INFO, SWITCH_EXTENDED_INFO_LONG) ?
		READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyExtendedInformation :
		READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation;
}


DWORD TrackDirService::run()
{
	auto tracker = makeDirectoryTracker();

	std::wcout << L"Request to track objects in the directory <" << static_cast<std::wstring>(dirFullPath_) <<
		L"> has been approved." << std::endl <<
		L"You can interrupt the wait process by pressing any key." << std::endl;

	tracker->beginTrack(needSubtreesCheck_);

	return ERROR_SUCCESS;
}

std::unique_ptr<INotificationsManager> TrackDirService::makeNotificationsManager() const
{
	switch (infoType_)
	{
	case READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation:
		return std::make_unique<DefaultManager>(dirFullPath_);

	case READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyExtendedInformation:
		return std::make_unique<ExtendedManager>(dirFullPath_);

	default:
		commonError(L"Undefined class of receiving information.", ERROR_INVALID_PARAMETER);
	}

	return nullptr;		//до сюда выполнение никогда не дойдет (добавил для статических анализаторов)
}

void TrackDirService::validateDirPath(_In_ const std::filesystem::path& dirPath)
{
	std::wstring errorMessage;
	if (!fs::exists(dirPath))
	{
		errorMessage = L"Directory <" + static_cast<std::wstring>(dirPath) +
			L"> is not found in your system. Check that the directory path is correct and try again.";
		commonError(errorMessage, ERROR_FILE_NOT_FOUND);
	}

	if (!fs::is_directory(dirPath))
	{
		errorMessage = L"Option " + std::wstring(OPT_TRACK_DIR) + L" (or " + OPT_TRACK_DIR_LONG +
			L") accepts a directory path only. Check that the directory path is correct and try again.";
		commonError(errorMessage, ERROR_INVALID_PARAMETER);
	}
}

std::unique_ptr<Tracker> TrackDirService::makeDirectoryTracker() const
{
	auto notificationsManager = makeNotificationsManager();
	return std::make_unique<Tracker>(static_cast<std::wstring>(dirFullPath_), notificationsManager);
}



