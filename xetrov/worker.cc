#include "xe/assert.h"
#include "worker.h"

using namespace xetrov;

void xe_fiber_worker::fiber_start(xe_ptr ptr){
	xe_fiber_worker& worker = *(xe_fiber_worker*)ptr;

	worker.routine.start(worker);
	worker.suspend();

	xe_stop("fiber resumed after completing");
}

xe_fiber_worker::xe_fiber_worker(xe_fiber& main, xe_fiber_routine& routine):
	main(main),
	routine(routine){
}

int xe_fiber_worker::init(){
	return worker.init();
}

void xe_fiber_worker::start(){
	worker.start(main, fiber_start, this);
}

void xe_fiber_worker::suspend(){
	worker.yield(main);
}

void xe_fiber_worker::resume(){
	main.yield(worker);
}

void xe_fiber_worker::destroy(){
	worker.free();
}

