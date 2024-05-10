#ifndef TRACKER_H
#define TRACKER_H

#include "MsgsQueue.h"
#include "NotificationDispatchers.h"


class Tracker final
{
public:
	Tracker(_In_ const std::wstring& path, _In_ std::unique_ptr<INotificationsManager>& manager);

	void beginTrack(_In_ const bool needSubtreesCheck);

private:
	static void CALLBACK waitUserSignal(_Inout_ Tracker& tracker);

	static void CALLBACK dispatchMsgs(_Inout_ Tracker& tracker);

private:
	void startDispatchThread();

	void startWaiterThread();

private:
	void serviceReceivedInfo(_Inout_ BytesBufferT& msgsPool, _Inout_ OVERLAPPED& ov);

	void addMsgsToQueue(_Inout_ BytesBufferT& msgsPool, _In_ const DWORD nBytes);

	void upMsgsCnt(_In_ LONG cnt);

	void raiseUserSignal();

	void addMsg(_In_ LPCVOID pBlob);

	void procCurMsg();

private:
	void terminateBackgroundTasks();

	void terminateAdditionalThreads();

	void cancelFileIoOperations();

private:
	MsgsQueue msgsBuffer_;

	std::thread dispatcherThread_;
	std::thread waiterThread_;

	std::unique_ptr<INotificationsManager> manager_;

	ATL::CHandle dirHandle_{ nullptr };

	ATL::CHandle newMsgsForProcCnt_{ nullptr };
	ATL::CHandle userSignal_{ nullptr };	
	ATL::CHandle threadKill_{ nullptr };

};



#endif		//TRACKER_H
