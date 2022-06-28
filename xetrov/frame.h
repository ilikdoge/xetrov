#pragma once
#include "types.h"
#include "xe/container/vector.h"
#include "buffer.h"
#include "common.h"

extern "C" {
	#include <libavutil/frame.h>
}

namespace xetrov{

enum xe_audio_sample_fmt{
	XE_SAMPLE_FMT_NONE = AV_SAMPLE_FMT_NONE,
	XE_SAMPLE_FMT_U8 = AV_SAMPLE_FMT_U8,
	XE_SAMPLE_FMT_S16 = AV_SAMPLE_FMT_S16,
	XE_SAMPLE_FMT_S32 = AV_SAMPLE_FMT_S32,
	XE_SAMPLE_FMT_FLT = AV_SAMPLE_FMT_FLT,
	XE_SAMPLE_FMT_DBL = AV_SAMPLE_FMT_DBL,

	XE_SAMPLE_FMT_U8P = AV_SAMPLE_FMT_U8P,
	XE_SAMPLE_FMT_S16P = AV_SAMPLE_FMT_S16P,
	XE_SAMPLE_FMT_S32P = AV_SAMPLE_FMT_S32P,
	XE_SAMPLE_FMT_FLTP = AV_SAMPLE_FMT_FLTP,
	XE_SAMPLE_FMT_DBLP = AV_SAMPLE_FMT_DBLP,
	XE_SAMPLE_FMT_S64 = AV_SAMPLE_FMT_S64,
	XE_SAMPLE_FMT_S64P = AV_SAMPLE_FMT_S64P
};

class xe_frame{
public:
	struct{
		int linesize[8];
		byte* data[8];
		xe_buffer_ref* bufs[8];
		xe_array<xe_buffer_ref*> extended_buf;
	} internal;

	byte** data;
	ulong duration;
	ulong timestamp;
	uint samples;
	uint channels;
	uint sample_rate;
	uint channel_layout;
	uint flags;

	union{
		xe_audio_sample_fmt audio_format;
		uint format;
	};

	xe_frame& operator=(xe_frame&& other){
		unref();

		data = other.data;
		duration = other.duration;
		timestamp = other.timestamp;
		samples = other.samples;
		channels = other.channels;
		sample_rate = other.sample_rate;
		channel_layout = other.channel_layout;
		flags = other.flags;

		xe_tmemcpy(&internal, &other.internal);

		other.data = null;
		other.duration = 0;
		other.timestamp = 0;
		other.samples = 0;
		other.channels = 0;
		other.sample_rate = 0;
		other.channel_layout = 0;
		other.flags = 0;

		xe_zero(&other.internal.bufs);

		other.internal.extended_buf.clear();

		return *this;
	}

	void unref(){
		for(uint i = 0; i < xe_array_size(internal.bufs); i++){
			xe_delete(internal.bufs[i]);

			internal.bufs[i] = null;
		}

		if(data != internal.data)
			xe_dealloc(data);
		for(uint i = 0; i < internal.extended_buf.size(); i++)
			xe_delete(internal.extended_buf[i]);
		internal.extended_buf.free();
	}
};

}