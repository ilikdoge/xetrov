#pragma once
#include "types.h"
#include "packet.h"
#include "reader/reader.h"
#include "pool.h"

namespace xetrov{

class xe_demuxer;

}

#include "format.h"

namespace xetrov{

class xe_demuxer{
protected:
	bool alloc_packet(xe_packet& packet, size_t size);

	xe_packet_buffer buffer;
	xe_format::xe_context* context;
public:
	xe_demuxer(xe_format::xe_context& context);

	virtual int open() = 0;
	virtual int seek(uint stream, ulong pos) = 0;
	virtual int read_packet(xe_packet& packet) = 0;
	virtual void reset() = 0;

	virtual ~xe_demuxer(){}
};

class xe_demuxer_class{
public:
	xe_demuxer_class(){}

	virtual xe_demuxer* create(xe_format::xe_context& context) const = 0;
	virtual bool probe(xe_reader& reader) const = 0;

	virtual ~xe_demuxer_class(){}
};

}