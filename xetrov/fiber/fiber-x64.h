#pragma once
#include "../types.h"

namespace xetrov{

struct xe_fiber_ctx{
	typedef void* reg;

	reg r12;
	reg r13;
	reg r14;
	reg r15;
	reg rip;
	reg stack;
	reg rbx;
	reg rbp;
};

}