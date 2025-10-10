#ifndef __NON_LOCK_BUFFER_H__
#define __NON_LOCK_BUFFER_H__
#include <atomic>
#include <iostream>

template<typename Value> 
class ConcurrentBufferNoLock {
public:
	typedef typename std::vector<Value> ValueVec;
public:
	ConcurrentBufferNoLock(int32_t _buffer_size) 
		: buffer_(_buffer_size), buffer_size_(_buffer_size), 
		cur_buffer_index_(0), valid_(false)
	{}

	/***********************************
	 *
	 *		pay attention: 
	 *		1. getWritable and doneWrite is in one thread 
	 *		2. getReadable and get is in another thread
	 *
	 **********************************/
	Value* getWritableBuffer() {
		int32_t index = cur_buffer_index_ + 1;
		if ( index == buffer_size_ )
			index = 0;	
		return &buffer_[index];
	}
	void doneWriteBuffer() {
		if ( ++cur_buffer_index_ == buffer_size_ )
			cur_buffer_index_ = 0;
	}
	int getReadableARBufferIndex() {
		return cur_buffer_index_;
	}
	Value* getBuffer(int _index) {
		if ( _index == buffer_size_ )
			return &buffer_[0];
		return &buffer_[_index];
	}
	void setValid(bool _valid) {
		valid_ = _valid;
	}
	bool isValid() {
		return valid_;
	}
	int32_t getSize() {
	    return buffer_size_;
	}
private:
	ValueVec buffer_;
	int32_t buffer_size_;
	std::atomic_int cur_buffer_index_;
	bool valid_;
};

#endif
