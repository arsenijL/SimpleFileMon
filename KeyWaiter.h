#ifndef KEY_WAITER_H
#define KEY_WAITER_H


class KeyWaiter final
{
public:
	KeyWaiter() = default;

	HANDLE operator*();

	~KeyWaiter();

private:
	static void CALLBACK waitForEvent(_Inout_ KeyWaiter& waiter);

private:
	static bool hasKeyEvent(_In_ const std::vector<INPUT_RECORD>& records, _In_ const DWORD cnt);

	void raiseKeyEvent();

private:
	std::thread waiter_;
	ATL::CHandle killThread_{ nullptr };
	ATL::CHandle keyEvent_{ nullptr };

};



#endif		//KEY_WAITER_H