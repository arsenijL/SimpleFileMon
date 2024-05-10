#ifndef SERVICES_H
#define SERVICES_H

#include "NotificationDispatchers.h"
#include "Tracker.h"

class IService
{
public:
	virtual DWORD run() = 0;

	virtual ~IService() = default;
};


class PrintHelpService : public IService
{
public:
	DWORD run() override;

};


class TrackFileService : public IService
{
public:
	TrackFileService(_In_ const std::shared_ptr<CLI_Utils::CmdLineParser>& opts);

	DWORD run() override;

private:
	static void validateFilePath(_In_ const std::filesystem::path& filePath);

	std::unique_ptr<INotificationsManager> makeNotificationsFilter() const;
	std::unique_ptr<Tracker> makeFileTracker() const;

private:
	std::filesystem::path fileFullPath_;
	READ_DIRECTORY_NOTIFY_INFORMATION_CLASS infoType_{ READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation };
};


class TrackDirService : public IService
{
public:
	TrackDirService(_In_ const std::shared_ptr<CLI_Utils::CmdLineParser>& opts);

	DWORD run() override;

private:
	static void validateDirPath(_In_ const std::filesystem::path& dirPath);

	std::unique_ptr<Tracker> makeDirectoryTracker() const;
	std::unique_ptr<INotificationsManager> makeNotificationsManager() const;

private:
	std::filesystem::path dirFullPath_;
	READ_DIRECTORY_NOTIFY_INFORMATION_CLASS infoType_{ READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation };
	bool needSubtreesCheck_{ false };

};


class WrongRequestService : public IService
{
public:
	DWORD run() override;

};



#endif		//SERVICES_H
