#pragma once
#include "fiber/fiber.h"

namespace xetrov{

class xe_fiber_worker;
class xe_fiber_routine{
public:
	virtual void start(xe_fiber_worker& ctx) = 0;
};

class xe_fiber_worker{
private:
	xe_fiber& main;
	xe_fiber worker;
	xe_fiber_routine& routine;

	static void fiber_start(xe_ptr ptr);
public:
	xe_fiber_worker(xe_fiber& main, xe_fiber_routine& routine);

	int init();
	void start();

	void suspend();
	void resume();

	void destroy();
};

}