#pragma once
#include "fiber-x64.h"
#include "../types.h"

namespace xetrov{

typedef void (*xe_fiber_startfn)(xe_ptr arg);

enum{
	XE_FIBER_STACK_SIZE = 0x800000 /* 8 MB */
};

class xe_fiber{
	xe_fiber_ctx context;
	xe_ptr stackp;
public:
	xe_fiber();

	int init(size_t stack_size = XE_FIBER_STACK_SIZE);
	void start(xe_fiber& from, xe_fiber_startfn fn, xe_ptr start_arg);
	void yield(xe_fiber& to);
	void free();
};

}