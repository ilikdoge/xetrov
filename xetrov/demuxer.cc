#include "demuxer.h"

using namespace xetrov;

bool xe_demuxer::alloc_packet(xe_packet& packet, size_t size){
	packet.unref();

	if(!buffer.has(size) && !context -> pool -> get_buffer(buffer, size))
		return false;
	buffer.alloc(packet, size);

	return true;
}

xe_demuxer::xe_demuxer(xe_format::xe_context& context_){
	context = &context_;
}