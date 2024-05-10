#include "stdafx.h"
#include "NotificationDispatchers.h"
#include "Common.h"
#include <minwinbase.h> 

namespace
{
	namespace fs = std::filesystem;

	const std::wstring_view prompt{ L"Active wait still works. To end waiting process, press any key: " };

	const int entryLen = 60;
	const std::wstring entryStart(entryLen, L'-');

	const std::wstring clearLine(prompt.size(), L' ');

	inline HANDLE getStdOutputHandle()
	{
		return getValidStdHandle(STD_OUTPUT_HANDLE);
	}

	void setNextMessagePositionInConsole()
	{
		HANDLE hStdOut = getStdOutputHandle();		//не используем RAII-оболочки, т.к. закрытие хэндла на стандартное устр-во приведет к его закрытию на уровне процесса, и при следующем использовании любой кэшированной ссылки возникнет ошибка

		CONSOLE_SCREEN_BUFFER_INFO cbi;
		if (!GetConsoleScreenBufferInfo(hStdOut, &cbi))
		{
			[[maybe_unused]] const DWORD eCode = GetLastError();
			return;
			//все плохо
		}

		COORD crd{ 0, cbi.dwCursorPosition.Y };
		if (!SetConsoleCursorPosition(hStdOut, crd))
		{
			[[maybe_unused]] const DWORD eCode = GetLastError();
			return;
			//все плохо
		}

		//'чистим' текущую строку и переводим каретку в ее начало
		std::wcout << clearLine << L'\r';
	}

	DWORD getActionId(_In_ const BytesBufferT& blob)		// TODO: то, что у FILE_NOTIFY_INFORMATION и FILE_NOTIFY_EXTENDED_INFORMATION есть одинаковые поля не говорит о том, что можно запихивать сюда все, что угодно. Лучше сделать свой вариант этой ф-ии для каждой стр-ры.
	{
		const auto& pNotifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(blob.data());
		return pNotifyInfo->Action;
	}

	inline bool equal(_In_ const std::wstring& str1, _In_ const std::wstring& str2)
	{
		return 0 == _wcsicmp(str1.c_str(), str2.c_str());
	}

	template<typename NotificationT>
	std::wstring getPath(_In_ const BytesBufferT& blob)
	{
		const auto pMsg = reinterpret_cast<const NotificationT*>(blob.data());

		std::wstring filePath(pMsg->FileNameLength + 1, 0);		//делаем запас, т.к. в FILE_NOTIFY_INFORMATION и FILE_NOTIFY_EXTENDED_INFORMATION у пути нет нуля
		std::memcpy(filePath.data(), pMsg->FileName, pMsg->FileNameLength);

		return filePath;
	}

	SYSTEMTIME getSystemTime(_In_ const LARGE_INTEGER& rawTime)
	{
		FILETIME rawFileTime{ static_cast<LONG>(rawTime.LowPart), static_cast<LONG>(rawTime.HighPart) };

		FILETIME localFileTime{ 0, 0 };
		if (!FileTimeToLocalFileTime(&rawFileTime, &localFileTime))
		{
			const DWORD eCode = GetLastError();
			commonError(L"FileTimeToLocalFileTime failed", eCode);
		}


		SYSTEMTIME localSysTime;
		if (!FileTimeToSystemTime(&localFileTime, &localSysTime))
		{
			const DWORD eCode = GetLastError();
			commonError(L"FileTimeToSystemTime failed", eCode);
		}

		return localSysTime;
	}

	std::wstring getFormatTime(_In_ const SYSTEMTIME& sTime)
	{
		constexpr int bufSize = 30;
		std::wstring buf(bufSize, 0);

		[[maybe_unused]] const auto res = swprintf_s(	buf.data(), 
														bufSize, 
														L"%02d-%02d-%04d %02d:%02d:%02d.%03d", 
														sTime.wDay, sTime.wMonth, sTime.wYear, sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds	);

		return buf;
	}

	inline SYSTEMTIME getLastAccessTime(_In_ const BytesBufferT& blob)
	{
		return getSystemTime(reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(blob.data())->LastAccessTime);
	}

	inline SYSTEMTIME getLastModificationTime(_In_ const BytesBufferT& blob)
	{
		return getSystemTime(reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(blob.data())->LastModificationTime);
	}

	inline SYSTEMTIME getLastChangeTime(_In_ const BytesBufferT& blob)
	{
		return getSystemTime(reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(blob.data())->LastChangeTime);
	}

	inline SYSTEMTIME getCreationTime(_In_ const BytesBufferT& blob)
	{
		return getSystemTime(reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(blob.data())->CreationTime);
	}

	std::wstring getActionDesc(_In_ const BytesBufferT& blob)
	{
		static const std::map <DWORD, std::wstring> descCreators{
			{FILE_ACTION_ADDED,				L"Added"},
			{FILE_ACTION_MODIFIED,			L"Modified"},
			{FILE_ACTION_REMOVED,			L"Removed" },
			{FILE_ACTION_RENAMED_OLD_NAME,	L"Renamed, old name shown"},
			{FILE_ACTION_RENAMED_NEW_NAME,	L"Renamed, new name shown"} };

		const auto& descCreator = descCreators.find(getActionId(blob));
		if (descCreator == descCreators.end())
		{
			return L"Undefined";
		}

		return descCreator->second;
	}

	inline LONGLONG getFileSize(_In_ const BytesBufferT& blob)
	{
		return reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(blob.data())->AllocatedLength.QuadPart;
	}

	inline DWORD getFileAttributes(_In_ const BytesBufferT& blob)
	{
		return reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(blob.data())->FileAttributes;
	}

	std::vector<std::wstring_view> getFileAttributesNames(_In_ const DWORD fileAttributes)
	{
		static const std::map<DWORD, std::wstring_view> attributeNames =
		{
			{FILE_ATTRIBUTE_READONLY,				L"Read-only"},
			{FILE_ATTRIBUTE_HIDDEN,					L"Hidden"},
			{FILE_ATTRIBUTE_SYSTEM,					L"System-used"},
			{FILE_ATTRIBUTE_DIRECTORY,				L"Directory"},
			{FILE_ATTRIBUTE_ARCHIVE,				L"Archive"},
			{FILE_ATTRIBUTE_DEVICE,					L"Device (system-reserved)"},
			{FILE_ATTRIBUTE_NORMAL,					L"Normal"},
			{FILE_ATTRIBUTE_TEMPORARY,				L"Temporary"},
			{FILE_ATTRIBUTE_SPARSE_FILE,            L"Sparse"},
			{FILE_ATTRIBUTE_REPARSE_POINT,          L"Reparse point"},
			{FILE_ATTRIBUTE_COMPRESSED,             L"Compressed"},
			{FILE_ATTRIBUTE_OFFLINE,                L"Non-available data"},
			{FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,    L"Non-indexed"},
			{FILE_ATTRIBUTE_ENCRYPTED,              L"Encrypted"},
			{FILE_ATTRIBUTE_INTEGRITY_STREAM,       L"Integrity"},
			{FILE_ATTRIBUTE_VIRTUAL,                L"Virtual (system-reserved)"},
			{FILE_ATTRIBUTE_NO_SCRUB_DATA,			L"Non-readable for AKA scrubber"},
			{FILE_ATTRIBUTE_EA,						L"Extended attributes"},
			{FILE_ATTRIBUTE_PINNED,					L"Pinned"},
			{FILE_ATTRIBUTE_UNPINNED,				L"Unpinned"},
			{FILE_ATTRIBUTE_RECALL_ON_OPEN,			L"Recall on open"},
			{FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS,	L"Recall on data access"}
		};

		std::vector<std::wstring_view> fileAttributesNames;

		for (const auto& el : attributeNames)
		{
			if (fileAttributes & el.first) {
				fileAttributesNames.emplace_back(el.second);
			}
		}

		return fileAttributesNames;
	}

	template<class AttrNamesListT>
	std::wstring getFileAttributesFormatInfo(_In_ const AttrNamesListT& attrNames)
	{
		constexpr auto messageNoAttributes = L"No attributes";

		if (attrNames.empty())
		{
			return messageNoAttributes;
		}

		std::wstring fileAttributesFormatInfo;		
		for (const auto& el : attrNames)
		{
			(fileAttributesFormatInfo += el.data()) += L", ";
		}

		fileAttributesFormatInfo.erase(fileAttributesFormatInfo.end() - 2);		//затираем последнюю запятую

		return fileAttributesFormatInfo;
	}

	inline std::wstring getFileAttributesFormatInfo(_In_ const BytesBufferT& blob)
	{
		return getFileAttributesFormatInfo(getFileAttributesNames(getFileAttributes(blob)));
	}

	template<typename FileNotifyInfoT>
	void copy(_Inout_ BytesBufferT& buf, _In_ const FileNotifyInfoT* pInfo)
	{
		buf.resize(sizeof(FileNotifyInfoT) + pInfo->FileNameLength);
		auto pDstMsg = reinterpret_cast<FileNotifyInfoT*>(buf.data());

		*pDstMsg = *pInfo;

		std::memcpy(pDstMsg->FileName, pInfo->FileName, pInfo->FileNameLength);
	}

	inline std::filesystem::path getFullPath(_In_ const std::filesystem::path& fileName, _In_ const std::filesystem::path& base)
	{
		std::filesystem::path tmpPath = base;
		tmpPath.append(static_cast<std::wstring>(fileName));
		return tmpPath;
	}


	void printDefaultInfo(_In_ const std::filesystem::path& filePath, _In_ const BytesBufferT& notifyInfo)
	{
		std::wcout << entryStart << std::endl << std::endl;

		constexpr int intendLen = 16;
		static auto intendMaker = std::setw(intendLen);

		std::wcout << intendMaker << L"File path: " << static_cast<std::wstring>(filePath) << std::endl;
		std::wcout << std::endl;

		std::wcout << intendMaker << L"Action: " << getActionDesc(notifyInfo) << std::endl;
		std::wcout << std::endl;
	}

	void printExtendedInfo(_In_ const std::filesystem::path& filePath, _In_ const BytesBufferT& notifyInfo)
	{
		std::wcout << entryStart << std::endl << std::endl;

		constexpr int intendLen = 28;
		static auto intendMaker = std::setw(intendLen);

		std::wcout << intendMaker << L"File path: " << static_cast<std::wstring>(filePath) << std::endl;
		std::wcout << std::endl;

		std::wcout << intendMaker << L"Action: " << getActionDesc(notifyInfo) << std::endl;
		std::wcout << std::endl;

		std::wcout << intendMaker << L"Last access time: " << getFormatTime(getLastAccessTime(notifyInfo)) << std::endl;
		std::wcout << intendMaker << L"Last change time: " << getFormatTime(getLastChangeTime(notifyInfo)) << std::endl;
		std::wcout << intendMaker << L"Last modification time: " << getFormatTime(getLastModificationTime(notifyInfo)) << std::endl;
		std::wcout << std::endl;

		std::wcout << intendMaker << L"Attributes: " << getFileAttributesFormatInfo(notifyInfo) << std::endl;
		std::wcout << intendMaker << L"Creation time: " << getFormatTime(getCreationTime(notifyInfo)) << std::endl;
		std::wcout << intendMaker << L"File size: " << std::dec << getFileSize(notifyInfo) << L" bytes" << std::endl;
		std::wcout << std::endl;
	}

}	//namespace



//------------- Реализация методов класса CManager ----------------------------

READ_DIRECTORY_NOTIFY_INFORMATION_CLASS CManager::getInfoClass()
{
	return READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyInformation;
}

bool CManager::last(_In_ LPCVOID pBlob)
{
	return 0L == reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(pBlob)->NextEntryOffset;
}

LPCVOID CManager::next(_In_ LPCVOID pBlob)
{
	return Add2Ptr(pBlob, reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(pBlob)->NextEntryOffset);
}

BytesBufferT CManager::getPackedMsg(_In_ LPCVOID pBlob)
{
	BytesBufferT buf;
	copy(buf, reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(pBlob));

	return buf;
}



//------------- Реализация методов класса CExtendedManager --------------------

READ_DIRECTORY_NOTIFY_INFORMATION_CLASS CExtendedManager::getInfoClass()
{
	return READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyExtendedInformation;
}

bool CExtendedManager::last(_In_ LPCVOID pBlob)
{
	return 0L == reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(pBlob)->NextEntryOffset;
}

LPCVOID CExtendedManager::next(_In_ LPCVOID pBlob)
{
	return Add2Ptr(pBlob, reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(pBlob)->NextEntryOffset);
}

BytesBufferT CExtendedManager::getPackedMsg(_In_ LPCVOID pBlob)
{
	BytesBufferT buf;
	copy(buf, reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(pBlob));

	return buf;
}



void DefaultManager::procMsg(_In_ const BytesBufferT& blob)
{
	setNextMessagePositionInConsole();

	const auto fileFullPath = getFullPath(getPath<FILE_NOTIFY_INFORMATION>(blob), getTrackedDirPath());

	printDefaultInfo(fileFullPath, blob);

	std::wcout << prompt;
}


void ExtendedManager::procMsg(_In_ const BytesBufferT& blob)
{
	setNextMessagePositionInConsole();

	const auto fileFullPath = getFullPath(getPath<FILE_NOTIFY_EXTENDED_INFORMATION>(blob), getTrackedDirPath());

	printExtendedInfo(fileFullPath, blob);

	std::wcout << prompt;
}


void DefaultNotificationsFilter::procMsg(_In_ const BytesBufferT& blob)
{
	const auto fileFullPath = getFullPath(getPath<FILE_NOTIFY_INFORMATION>(blob), getTrackedDirPath());
	
	if (!equal(fileFullPath, trackedFileFullPath_))
	{
		return;
	}

	setNextMessagePositionInConsole();

	printDefaultInfo(fileFullPath, blob);

	std::wcout << prompt;
}


void ExtendedNotificationsFilter::procMsg(_In_ const BytesBufferT& blob)
{
	const auto fileFullPath = getFullPath(getPath<FILE_NOTIFY_EXTENDED_INFORMATION>(blob), getTrackedDirPath());

	if (!equal(fileFullPath, trackedFileFullPath_))
	{
		return;
	}

	setNextMessagePositionInConsole();

	printExtendedInfo(fileFullPath, blob);

	std::wcout << prompt;
}

