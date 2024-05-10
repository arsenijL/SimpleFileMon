#include "stdafx.h"
#include "MsgsQueue.h"
#include "Common.h"


bool MsgsQueue::empty() const
{
	return msgs_.empty();
}

const BytesBufferT& MsgsQueue::getMsg()
{
	std::lock_guard<std::mutex> locker(mtx_);
	return msgs_.front();
}

void MsgsQueue::popRecents()
{
	std::lock_guard<std::mutex> locker(mtx_);
	msgs_.pop();
}