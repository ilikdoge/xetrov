#pragma once
#include "xe/error.h"

namespace xetrov{

enum xetrov_error{
	XETROV_FIRST = -2100,
	XE_INVALID_DATA,
	XE_BUFFER_TOO_SMALL,
	XE_EOF,
	XE_UNKNOWN_FORMAT,
	XE_EXTERNAL,
	XETROV_LAST
};

static xe_cstr xetrov_strerror(int err){
	switch(err){
		case XE_INVALID_DATA:
			return "Invalid data in input file";
		case XE_BUFFER_TOO_SMALL:
			return "Buffer too small";
		case XE_EOF:
			return "End of file";
		case XE_UNKNOWN_FORMAT:
			return "Unknown format";
		case XE_EXTERNAL:
			return "Error in external library";
	}

	return xe_strerror(err);
}

}