#include "format.h"
#include "error.h"
#include "demuxer.h"
#include "common.h"
#include "demuxers/isom.h"
#include "demuxers/mkv.h"

using namespace xetrov;

static xe_isom_class isom;
static xe_matroska_class matroska;

static const xe_demuxer_class* formats[] = {
	&isom,
	&matroska
};

xe_format::xe_format(){
	demuxer = null;
	resource = null;
	stream = null;
	worker = null;
}

void xe_format::init(xe_fiber_worker& worker_, xe_resource& resource_, xe_packet_buffer_pool& pool_){
	worker = &worker_;
	resource = &resource_;
	context.pool = &pool_;
}

int xe_format::open(){
	int err, reader_error;

	if(!stream){
		stream = resource -> create();

		if(!stream)
			return XE_ENOMEM;
		if((err = stream -> open()))
			return err;
		if((err = context.reader.init(*worker, *stream)))
			return err;
		bool matches;

		for(size_t i = 0; i < xe_array_size(formats); i++){
			context.reader.peek_mode(true);
			matches = formats[i] -> probe(context.reader);
			context.reader.peek_mode(false);
			err = context.reader.error();

			if(err)
				return err;
			if(!matches)
				continue;
			demuxer = formats[i] -> create(context);

			if(!demuxer)
				return XE_ENOMEM;
			break;
		}

		if(!demuxer)
			return XE_UNKNOWN_FORMAT;
	}

	err = demuxer -> open();

	if(err)
		stream -> abort();
	return err;
}

int xe_format::seek(uint stream, ulong pos){
	return demuxer -> seek(stream, pos);
}

int xe_format::read_packet(xe_packet& packet){
	int err = demuxer -> read_packet(packet);

	if(err)
		return err;
	xe_track* track = context.tracks[packet.track];

	if(track -> parse != XE_PARSE_NONE){
		if(!track -> parser && (err = xe_codec_parser::open(&track -> parser, track -> codec)))
			return err;
		err = track -> parser -> parse(packet);
	}

	return err;
}

void xe_format::close(){
	context.reader.close();

	xe_delete(demuxer);
}

const xe_vector<xe_track*> xe_format::tracks() const{
	return context.tracks;
}