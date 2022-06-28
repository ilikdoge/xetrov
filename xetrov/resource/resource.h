#pragma once
#include "stream.h"

namespace xetrov{

class xe_resource{
public:
	xe_resource(){}

	virtual xe_stream* create() = 0;
	virtual void close() = 0;
};

}