#include "stdafx.h"
#include "Tracker.h"
#include "Common.h"
#include "MsgsQueue.h"
#include "KeyWaiter.h"


namespace
{
	HANDLE createValidSemaphore()
	{
		constexpr DWORD maxSemaphoreVal = 200000UL;		// TODO: magic?

		HANDLE hSemaphore = CreateSemaphore(nullptr, 0, maxSemaphoreVal, nullptr);
		if (!hSemaphore)
		{
			const DWORD eCode = GetLastError();
			commonError(L"CreateSemaphore failed.", eCode);
		}

		return hSemaphore;
	}

}	//namespace


Tracker::Tracker(_In_ const std::wstring& path, _In_ std::unique_ptr<INotificationsManager>& manager)
	: newMsgsForProcCnt_(createValidSemaphore()), userSignal_(createValidEvent(FALSE, FALSE)), threadKill_(createValidEvent(TRUE, FALSE))
{
	HANDLE hDir = CreateFile(
		path.c_str(),
		SYNCHRONIZE | FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS,
		nullptr	);

	if (INVALID_HANDLE_VALUE == hDir)
	{
		const DWORD eCode = GetLastError();
		commonError(L"An error occured while opening tracked directory.", eCode);
	}

	dirHandle_.Attach(hDir);

	std::swap(manager_, manager);
}


#define BUF_SIZE_ALIGNED 32768		//DWORD-aligned

#define NOTIFY_FILTER_ALL   FILE_NOTIFY_CHANGE_FILE_NAME       \
							| FILE_NOTIFY_CHANGE_DIR_NAME      \
							| FILE_NOTIFY_CHANGE_ATTRIBUTES    \
							| FILE_NOTIFY_CHANGE_SIZE          \
							| FILE_NOTIFY_CHANGE_LAST_WRITE    \
							| FILE_NOTIFY_CHANGE_LAST_ACCESS   \
							| FILE_NOTIFY_CHANGE_CREATION      \
							| FILE_NOTIFY_CHANGE_SECURITY 


void Tracker::beginTrack(_In_ const bool needSubtreesCheck)
{
	ATL::CHandle changeSignal{ createValidEvent(FALSE, FALSE) };
	OVERLAPPED ov = {};
	ov.hEvent = changeSignal;

	HANDLE hEvents[] = { userSignal_, changeSignal };

	BytesBufferT tmpBuf(BUF_SIZE_ALIGNED);		//буфер для хранения сообщений

	auto eInfo = manager_->getInfoClass();		//тип получаемых сообщений

	startWaiterThread();
	startDispatchThread();

	try		// засовываем в try / catch, что бы при возникновении исключений мы могли корректно завершить фоновые потоки и асинхронные API-шные функции.
	{
		while (true)
		{
			if (!ReadDirectoryChangesExW(dirHandle_, tmpBuf.data(), static_cast<DWORD>(tmpBuf.size()), static_cast<BOOL>(needSubtreesCheck), NOTIFY_FILTER_ALL, NULL, &ov, NULL, eInfo))
			{
				const DWORD eCode = GetLastError();
				commonError(L"An error occured while reading changes in tracked directory.", eCode);
			}

			switch (WaitForMultipleObjects(2, hEvents, FALSE, INFINITE))
			{
			case WAIT_OBJECT_0:			//сигнал от пользователя
				terminateBackgroundTasks();
				return;

			case WAIT_OBJECT_0 + 1:		//новые сообщения об изменениях
				serviceReceivedInfo(tmpBuf, ov);
				break;

			default:
				commonError(L"Unrecognized status of WaitForMultipleObjects.", ERROR_INVALID_STATE);
				break;
			}
		}
	}
	catch(...)
	{
		terminateBackgroundTasks();
		throw;
	}
}


#undef NOTIFY_FILTER_ALL

#undef BUF_SIZE_ALIGNED


void CALLBACK Tracker::waitUserSignal(_Inout_ Tracker& tracker)
{
	KeyWaiter kw;

	HANDLE hEvents[] = { *kw, tracker.threadKill_ };
	switch (WaitForMultipleObjects(2, hEvents, FALSE, INFINITE))
	{
	case WAIT_OBJECT_0:			//сигнал о нажатии на клавишу
		tracker.raiseUserSignal();
		break;

	case WAIT_OBJECT_0 + 1:		//сигнал на закрытие потока
		break;

	default:
		commonError(L"Unrecognized status of WaitForMultipleObjects.", ERROR_INVALID_STATE);
		break;
	}
}

void CALLBACK Tracker::dispatchMsgs(_Inout_ Tracker& tracker)	
{
	HANDLE hEvents[] = { tracker.threadKill_, tracker.newMsgsForProcCnt_ };

	while (true)
	{
		switch (WaitForMultipleObjects(2, hEvents, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0:			//сигнал на закрытие потока
			return;

		case WAIT_OBJECT_0 + 1:		//сигнал о поступлении данных об изменениях
			tracker.procCurMsg();
			break;

		default:
			commonError(L"Unrecognized status of WaitForMultipleObjects.", ERROR_INVALID_STATE);
			break;
		}
	}
}


void Tracker::serviceReceivedInfo(_Inout_ BytesBufferT& msgsPool, _Inout_ OVERLAPPED& ov)
{
	DWORD readedBytes = 0;
	if (!GetOverlappedResult(dirHandle_, &ov, &readedBytes, FALSE))		// bAlertable == FALSE, т.к. если мы попали в эту функцию, то асинхронная операция завершилась, и количество возвращенных байтов мы должны получить немедленно.
	{
		const DWORD eCode = GetLastError();
		commonError(L"GetOverlappedResult failed.", eCode);
	}

	addMsgsToQueue(msgsPool, readedBytes);
}

void Tracker::addMsgsToQueue(_Inout_ BytesBufferT& msgsPool, _In_ const DWORD nBytes)
{
	if (!nBytes)
	{
		return;
	}

	size_t msgsCnt = 0;
	auto pCurEntry = reinterpret_cast<LPCVOID>(msgsPool.data());
	while (true)
	{
		++msgsCnt;
		addMsg(pCurEntry);

		if (manager_->last(pCurEntry))
		{
			break;
		}

		pCurEntry = manager_->next(pCurEntry);
	}
	
	//будим поток - обработчик
	upMsgsCnt(static_cast<LONG>(msgsCnt));
}


void Tracker::terminateBackgroundTasks()
{
	terminateAdditionalThreads();
	cancelFileIoOperations();
}

void Tracker::terminateAdditionalThreads()
{
	if (!SetEvent(threadKill_))
	{
		std::terminate();
	}

	if (dispatcherThread_.joinable())
	{
		dispatcherThread_.join();
	}

	if (waiterThread_.joinable())
	{
		waiterThread_.join();
	}
}

void Tracker::cancelFileIoOperations()
{
	if (!CancelIo(dirHandle_))
	{
		std::terminate();
	}
}

void Tracker::startDispatchThread()
{
	std::thread t(dispatchMsgs, std::ref(*this));
	dispatcherThread_.swap(t);
}

void Tracker::startWaiterThread()
{
	std::thread t(waitUserSignal, std::ref(*this));
	waiterThread_.swap(t);
}

void Tracker::addMsg(_In_ LPCVOID pBlob)
{
	msgsBuffer_.addMsg(manager_->getPackedMsg(pBlob));
}

void Tracker::procCurMsg()
{
	manager_->procMsg(msgsBuffer_.getMsg());
	msgsBuffer_.popRecents();
}


void Tracker::upMsgsCnt(_In_ LONG cnt)
{
	if (!ReleaseSemaphore(newMsgsForProcCnt_, cnt, nullptr))
	{
		const DWORD eCode = GetLastError();
		commonError(L"ReleaseSemaphore failed.", eCode);
	}
}

void Tracker::raiseUserSignal()
{
	if (!SetEvent(userSignal_))
	{
		const DWORD eCode = GetLastError();
		commonError(L"SetEvent failed.", eCode);
	}
}
