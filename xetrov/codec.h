#pragma once
#include "xe/container/vector.h"
#include "xe/overflow.h"
#include "frame.h"
#include "packet.h"

extern "C" {
	#include <libavutil/channel_layout.h>
}

namespace xetrov{

enum{
	XE_BUFFER_PADDING = 64
};

enum xe_codec_id{
	XE_CODEC_NONE = 0,
	XE_CODEC_AAC,
	XE_CODEC_OPUS,
	XE_CODEC_FLAC,
	XE_CODEC_VORBIS,
	XE_CODEC_MP3,
	XE_CODEC_MP2
};

enum xe_codec_mode{
	XE_CODEC_DECODE = 0,
	XE_CODEC_ENCODE = 1
};

enum xe_channels{
	XE_CH_FRONT_LEFT = AV_CH_FRONT_LEFT,
	XE_CH_FRONT_RIGHT = AV_CH_FRONT_RIGHT,
	XE_CH_FRONT_CENTER = AV_CH_FRONT_CENTER,
	XE_CH_LOW_FREQUENCY = AV_CH_LOW_FREQUENCY,
	XE_CH_BACK_LEFT = AV_CH_BACK_LEFT,
	XE_CH_BACK_RIGHT = AV_CH_BACK_RIGHT,
	XE_CH_FRONT_LEFT_OF_CENTER = AV_CH_FRONT_LEFT_OF_CENTER,
	XE_CH_FRONT_RIGHT_OF_CENTER = AV_CH_FRONT_RIGHT_OF_CENTER,
	XE_CH_BACK_CENTER = AV_CH_BACK_CENTER,
	XE_CH_SIDE_LEFT = AV_CH_SIDE_LEFT,
	XE_CH_SIDE_RIGHT = AV_CH_SIDE_RIGHT,
	XE_CH_TOP_CENTER = AV_CH_TOP_CENTER,
	XE_CH_TOP_FRONT_LEFT = AV_CH_TOP_FRONT_LEFT,
	XE_CH_TOP_FRONT_CENTER = AV_CH_TOP_FRONT_CENTER,
	XE_CH_TOP_FRONT_RIGHT = AV_CH_TOP_FRONT_RIGHT,
	XE_CH_TOP_BACK_LEFT = AV_CH_TOP_BACK_LEFT,
	XE_CH_TOP_BACK_CENTER = AV_CH_TOP_BACK_CENTER,
	XE_CH_TOP_BACK_RIGHT = AV_CH_TOP_BACK_RIGHT,
	XE_CH_STEREO_LEFT = AV_CH_STEREO_LEFT,
	XE_CH_STEREO_RIGHT = AV_CH_STEREO_RIGHT,
	XE_CH_WIDE_LEFT = AV_CH_WIDE_LEFT,
	XE_CH_WIDE_RIGHT = AV_CH_WIDE_RIGHT,
	XE_CH_SURROUND_DIRECT_LEFT = AV_CH_SURROUND_DIRECT_LEFT,
	XE_CH_SURROUND_DIRECT_RIGHT = AV_CH_SURROUND_DIRECT_RIGHT,
	XE_CH_LOW_FREQUENCY_2 = AV_CH_LOW_FREQUENCY_2,
	XE_CH_TOP_SIDE_LEFT = AV_CH_TOP_SIDE_LEFT,
	XE_CH_TOP_SIDE_RIGHT = AV_CH_TOP_SIDE_RIGHT,
	XE_CH_BOTTOM_FRONT_CENTER = AV_CH_BOTTOM_FRONT_CENTER,
	XE_CH_BOTTOM_FRONT_LEFT = AV_CH_BOTTOM_FRONT_LEFT,
	XE_CH_BOTTOM_FRONT_RIGHT = AV_CH_BOTTOM_FRONT_RIGHT
};

static ulong xe_default_channel_layout(uint channels){
	return av_get_default_channel_layout(channels);
}

struct xe_codec_parameters{
	xe_codec_id id;
	xe_array<byte> config;

	ulong channel_layout;
	uint sample_rate;
	uint channels;
	uint bits_per_sample;
	uint frame_size;
	uint bit_rate;
	xe_audio_sample_fmt format;

	bool alloc_config(size_t size){
		size_t total;

		if(xe_overflow_add(total, size, (size_t)XE_BUFFER_PADDING))
			return false;
		xe_bptr data = xe_alloc<byte>(total);

		if(!data)
			return false;
		xe_zero(data + size, XE_BUFFER_PADDING);

		config = xe_array<byte>(data, size);

		return true;
	}
};

class xe_codec{
protected:
	xe_codec_id id_;

	xe_codec(xe_codec_id id){
		id_ = id;
	}

	virtual int init(xe_codec_parameters& params) = 0;
public:
	xe_codec_id id() const{
		return id_;
	}

	virtual int send_packet(xe_packet& packet) = 0;
	virtual int send_frame(xe_frame& frame) = 0;
	virtual int receive_frame(xe_frame& frame) = 0;
	virtual int receive_packet(xe_packet& packet) = 0;
	virtual int drain() = 0;
	virtual void flush() = 0;

	virtual ~xe_codec(){}

	static int open(xe_codec** codec, xe_codec_parameters& params, xe_codec_mode mode);
};

class xe_codec_parser{
protected:
	xe_codec_id id_;

	xe_codec_parser(xe_codec_id id){
		id_ = id;
	}

	virtual int init(xe_codec_parameters& params) = 0;
public:
	xe_codec_id id() const{
		return id_;
	}

	virtual int parse(xe_packet& packet) = 0;
	virtual void close(){}

	static int open(xe_codec_parser** parser, xe_codec_parameters& params);
};

}