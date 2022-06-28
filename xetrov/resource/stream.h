#pragma once
#include "../types.h"

namespace xetrov{

class xe_stream{
public:
	typedef int (*write_cb)(xe_stream& stream, xe_ptr buf, size_t len);
	typedef void (*done_cb)(xe_stream& stream, int error);
	xe_ptr data;
protected:
	struct xe_callbacks{
		write_cb write;
		done_cb done;
	} callbacks;

	bool seekable_;
public:
	xe_stream(){}

	virtual int open(ulong start = 0, ulong end = 0) = 0;
	virtual int seek(ulong offset) = 0;
	virtual void pause(bool paused) = 0;
	virtual void abort() = 0;
	virtual void close() = 0;

	bool seekable() const{
		return seekable_;
	}

	void set_write_cb(write_cb cb){
		callbacks.write = cb;
	}

	void set_done_cb(done_cb cb){
		callbacks.done = cb;
	}
};

}