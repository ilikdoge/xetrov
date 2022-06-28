#pragma once
#include "types.h"
#include "rational.h"
#include "buffer.h"
#include "xe/container/vector.h"

namespace xetrov{

enum xe_packet_flags{
	XE_PACKET_FLAG_NONE = 0x0,
	XE_PACKET_FLAG_KEY = 0x1
};

struct xe_packet{
	xe_buffer_ref ref;
	xe_array<byte> buffer;
	ulong duration;
	ulong timestamp;
	uint flags;
	uint track;
	xe_rational timescale;

	xe_cbptr data() const{
		return buffer.data();
	}

	xe_bptr data(){
		return buffer.data();
	}

	size_t size() const{
		return buffer.size();
	}

	void unref(){
		ref.unref();
	}
};

}