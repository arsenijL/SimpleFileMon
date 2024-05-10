#include "stdafx.h"
#include "KeyWaiter.h"
#include "Common.h"

namespace
{
	inline HANDLE getStdInputHandle()
	{
		return getValidStdHandle(STD_INPUT_HANDLE);
	}

	void flushConsoleEventsBuf(_In_ HANDLE hStd)
	{
		if (!FlushConsoleInputBuffer(hStd))
		{
			const DWORD eCode = GetLastError();
			commonError(L"FlushConsoleInputBuffer failed.", eCode);
		}
	}

}	//namespace


HANDLE KeyWaiter::operator*()
{
	keyEvent_.Attach(createValidEvent(FALSE, FALSE));

	killThread_.Attach(createValidEvent(TRUE, FALSE));

	//запускаем на ожидание второй поток и возвращаем управление, чтоб не блокировать вызывающий поток
	std::thread t(waitForEvent, std::ref(*this));
	waiter_.swap(t);

	return keyEvent_;
}


#define BUF_SIZE 4096

void CALLBACK KeyWaiter::waitForEvent(_Inout_ KeyWaiter& waiter)
{
	HANDLE hStdIn = getStdInputHandle();	//не используем RAII-оболочки, т.к. закрытие хэндла на стандартное устр-во приведет к его закрытию на уровне процесса, и при следующем использовании любой кэшированной ссылки возникнет ошибка

	HANDLE hEvents[] = { waiter.killThread_, hStdIn };

	std::vector<INPUT_RECORD> irInBuf(BUF_SIZE);
	DWORD eventsCnt = 0;
	while (true)
	{
		switch (WaitForMultipleObjects(2, hEvents, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0:			//экстренное (или нет) завершение потока
			return;

		case WAIT_OBJECT_0 + 1:		//изменения в консоли
			if (!PeekConsoleInput(hStdIn, irInBuf.data(), static_cast<DWORD>(irInBuf.size()), &eventsCnt))
			{
				const DWORD eCode = GetLastError();
				commonError(L"PeekConsoleInput failed.", eCode);
			}

			flushConsoleEventsBuf(hStdIn);

			if (hasKeyEvent(irInBuf, eventsCnt))
			{
				waiter.raiseKeyEvent();
				return;
			}

			break;

		default:		
			commonError(L"Unrecognized status of WaitForMultipleObjects.", ERROR_INVALID_STATE);
			break;

		}
	}
}

#undef BUF_SIZE

KeyWaiter::~KeyWaiter()
{
	if (!SetEvent(killThread_))
	{
		std::terminate();
	}

	if (waiter_.joinable())
	{
		waiter_.join();
	}
}


bool KeyWaiter::hasKeyEvent(_In_ const std::vector<INPUT_RECORD>& records, _In_ const DWORD cnt)
{
	for (auto rec = records.begin(), recEnd = records.cbegin() + cnt; rec != recEnd; ++rec)
	{
		if (KEY_EVENT == rec->EventType &&
			rec->Event.KeyEvent.bKeyDown)
		{
			return true;
		}
	}

	return false;
}

void KeyWaiter::raiseKeyEvent()
{
	if (!SetEvent(keyEvent_))
	{
		const DWORD eCode = GetLastError();
		commonError(L"SetEvent failed.", eCode);
	}
}
