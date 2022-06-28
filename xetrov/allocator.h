#pragma once
#include "types.h"

namespace xetrov{

class xe_allocator{
	size_t peak;
	bool reached_capacity;
};

}