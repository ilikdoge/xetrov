#pragma once
#include "../types.h"
#include "xe/assert.h"

namespace xetrov{

class xe_bit_reader{
private:
	xe_bptr buf;
	size_t offset;
	size_t bits;
public:
	xe_bit_reader(xe_bptr buf_, size_t offset_, size_t total_){
		buf = buf_;
		offset = offset_;
		bits = total_;
	}

	byte read(){
		if(offset < bits)
			return (buf[offset >> 3] >> (7 - (offset++ & 0x7))) & 0x1;
		offset++;

		return 0;
	}

	ulong read(size_t len){
		xe_assert(len);

		ulong value = 0;
		uint byte;
		uint read;

		if(!has_bits(len)){
			offset += len;

			return 0;
		}

		while(len){ // TODO optimize read more than one byte at a time
			read = 8 - (offset & 0x7);
			byte = buf[offset >> 3];

			if(read > len){
				byte >>= (read - len);
				byte &= (1 << len) - 1;
				offset += len;
				value = (value << len) | byte;

				break;
			}else{
				byte &= (1 << read) - 1;
				offset += read;
				len -= read;
				value = (value << read) | byte;
			}
		}

		return value;
	}

	bool has_bits(ulong len){
		return len + offset <= bits;
	}
};

}