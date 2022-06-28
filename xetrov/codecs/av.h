#pragma once
#include "../codec.h"
#include "../error.h"

extern "C" {
	#include <libavcodec/avcodec.h>
}

namespace xetrov{

class xe_av_codec : public xe_codec{
protected:
	AVCodecContext* context;

	xe_av_codec(xe_codec_id id);
public:
	int open(const AVCodec* codec, xe_codec_parameters& params);

	int send_packet(xe_packet& packet);
	int send_frame(xe_frame& frame);

	int receive_frame(xe_frame& frame);
	int receive_packet(xe_packet& packet);

	int drain();
	void flush();

	~xe_av_codec();
};

}