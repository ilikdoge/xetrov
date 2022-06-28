#pragma once
#include "reader/reader.h"
#include "resource/resource.h"
#include "resource/stream.h"
#include "worker.h"
#include "codec.h"
#include "packet.h"
#include "pool.h"

namespace xetrov{

enum xe_track_type{
	XE_TRACK_TYPE_NONE = 0,
	XE_TRACK_TYPE_VIDEO,
	XE_TRACK_TYPE_AUDIO,

	/* unused */
	XE_TRACK_TYPE_OTHER
};

enum xe_parse{
	XE_PARSE_NONE = 0,
	XE_PARSE_HEADER,
	XE_PARSE_FULL
};

enum xe_discard{
	XE_DISCARD_NONE = 0,
	XE_DISCARD_DEFAULT,
	XE_DISCARD_ALL
};

struct xe_track{
	xe_track_type type;
	xe_codec_parameters codec;

	ulong duration;
	xe_rational timescale;
	xe_discard discard;

	/* private */
	xe_parse parse;
	xe_codec_parser* parser;
};

class xe_demuxer;
class xe_format{
public:
	struct xe_context{
		xe_packet_buffer_pool* pool;
		xe_reader reader;
		xe_reader scan_reader;
		xe_vector<xe_track*> tracks;
	};

	xe_format();

	void init(xe_fiber_worker& worker, xe_resource& resouce, xe_packet_buffer_pool& pool);

	int open();
	void close();

	int read_packet(xe_packet& packet);
	int seek(uint stream, ulong pos);

	const xe_vector<xe_track*> tracks() const;
private:
	xe_context context;
	xe_demuxer* demuxer;
	xe_resource* resource;
	xe_stream* stream;
	xe_fiber_worker* worker;
};

}