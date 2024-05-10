#ifndef MSGS_QUEUE_H
#define	MSGS_QUEUE_H


using BytesBufferT = std::vector<BYTE>;

class MsgsQueue final
{
public:
	MsgsQueue() = default;

	bool empty() const;

	template<typename BufT>
	void addMsg(BufT&& buf)
	{
		std::lock_guard<std::mutex> locker(mtx_);
		msgs_.push(std::forward<BufT>(buf));
	}

	const BytesBufferT& getMsg();

	void popRecents();

private:
	std::mutex mtx_;
	std::queue<BytesBufferT> msgs_;

};



#endif		//MSGS_QUEUE_H
