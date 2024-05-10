#ifndef NOTIFICATION_DISPATCHERS_H
#define NOTIFICATION_DISPATCHERS_H

#include "Common.h"
#include "MsgsQueue.h"


struct INotificationsManager
{
	virtual READ_DIRECTORY_NOTIFY_INFORMATION_CLASS getInfoClass() = 0;

	virtual BytesBufferT getPackedMsg(_In_ LPCVOID pBlob) = 0;

	virtual void procMsg(_In_ const BytesBufferT& blob) = 0;

	virtual bool last(_In_ LPCVOID pBlob) = 0;

	virtual LPCVOID next(_In_ LPCVOID pBlob) = 0;

	virtual ~INotificationsManager() {}

};

struct CManager : public INotificationsManager
{
	BytesBufferT getPackedMsg(_In_ LPCVOID pBlob) final;

	READ_DIRECTORY_NOTIFY_INFORMATION_CLASS getInfoClass() final;

	bool last(_In_ LPCVOID pBlob) final;

	LPCVOID next(_In_ LPCVOID pBlob) final;
};

struct CExtendedManager : public INotificationsManager
{
	BytesBufferT getPackedMsg(_In_ LPCVOID pBlob) final;

	READ_DIRECTORY_NOTIFY_INFORMATION_CLASS getInfoClass() final;

	bool last(_In_ LPCVOID pBlob) final;

	LPCVOID next(_In_ LPCVOID pBlob) final;
};

class DefaultManager : public CManager
{
public:
	DefaultManager(_In_ const std::filesystem::path& dirPath) : trackedDirFullPath_(dirPath), localizer_(std::make_unique<Localizer>())
	{
		std::wcout.setf(std::ios::left);		//включаем выравнивание по левому краю для всех последующих записей
	}

	void procMsg(_In_ const BytesBufferT& blob) override;

	const std::filesystem::path& getTrackedDirPath()
	{
		return trackedDirFullPath_;
	}

	~DefaultManager()
	{
		std::wcout.unsetf(std::ios::left);		//отключаем ранее установленное выравнивание по левому краю
	}

private:
	std::filesystem::path trackedDirFullPath_;		//строка пути нужна для получения полного пути файла (ReadDirectoryChanges возвращает путь, относительный введеной директории)
	std::unique_ptr<Localizer> localizer_;
};

class DefaultNotificationsFilter final : public DefaultManager
{
public:
	DefaultNotificationsFilter(_In_ const std::filesystem::path& trackedFileFullPath, _In_ const std::filesystem::path& trackedDirFullPath)
		: DefaultManager(trackedDirFullPath), trackedFileFullPath_(trackedFileFullPath)
	{
	}

	void procMsg(_In_ const BytesBufferT& blob) override;

private:
	std::filesystem::path trackedFileFullPath_;
	
};


class ExtendedManager : public CExtendedManager
{
public:
	ExtendedManager(_In_ const std::filesystem::path& dirPath) : trackedDirFullPath_(dirPath), localizer_(std::make_unique<Localizer>())
	{
		std::wcout.setf(std::ios::left);		//включаем выравнивание по левому краю для всех последующих записей
	}

	void procMsg(_In_ const BytesBufferT& blob) override;

	const std::filesystem::path& getTrackedDirPath()
	{
		return trackedDirFullPath_;
	}

	~ExtendedManager()
	{
		std::wcout.unsetf(std::ios::left);		//отключаем ранее установленное выравнивание по левому краю
	}

private:
	std::filesystem::path trackedDirFullPath_;
	std::unique_ptr<Localizer> localizer_;
};

class ExtendedNotificationsFilter final : public ExtendedManager
{
public:
	ExtendedNotificationsFilter(_In_ const std::filesystem::path& trackedFileFullPath, _In_ const std::filesystem::path& trackedDirFullPath)
		: ExtendedManager(trackedDirFullPath), trackedFileFullPath_(trackedFileFullPath)
	{
	}

	void procMsg(_In_ const BytesBufferT& blob) override;

private:
	std::filesystem::path trackedFileFullPath_;

};



#endif			//NOTIFICATION_DISPATCHERS_H
