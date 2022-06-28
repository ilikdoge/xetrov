#include "../common.h"
#include "../error.h"
#include "xe/mem.h"
#include "fiber.h"

using namespace xetrov;

extern "C" {

void xe_fiber_start(xe_fiber_ctx* from, xe_fiber_ctx* to, xe_fiber_startfn start, xe_ptr arg);
void xe_fiber_transfer(xe_fiber_ctx* from, xe_fiber_ctx* to);

}

xe_fiber::xe_fiber(){
	xe_zero(&context);

	stackp = null;
}

int xe_fiber::init(size_t stack_size){
	xe_bptr stack = xe_alloc_aligned<byte>(0, stack_size);

	if(!stack)
		return XE_ENOMEM;
	context.stack = stack + stack_size;
	stackp = stack;

	return 0;
}

void xe_fiber::start(xe_fiber& from, xe_fiber_startfn fn, xe_ptr start_arg){
	xe_fiber_start(&from.context, &context, fn, start_arg);
}

void xe_fiber::yield(xe_fiber& to){
	xe_fiber_transfer(&context, &to.context);
}

void xe_fiber::free(){
	xe_dealloc(stackp);
}