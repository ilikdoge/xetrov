#include <opus/opus.h>
#include "opus.h"
#include "av.h"
#include "../error.h"

using namespace xetrov;

static const AVCodec* opus_decoder = avcodec_find_decoder(AV_CODEC_ID_OPUS);
static const AVCodec* opus_encoder = avcodec_find_encoder(AV_CODEC_ID_OPUS);

enum{
	XE_OPUS_SAMPLE_RATE = 48000
};

class xe_opus_parser : public xe_codec_parser{
public:
	xe_opus_parser(): xe_codec_parser(XE_CODEC_OPUS){}

	int init(xe_codec_parameters& params){
		return 0;
	}

	int parse(xe_packet& packet){
		ulong samples;

		if(!packet.size())
			return XE_INVALID_DATA;
		samples = opus_packet_get_nb_samples(packet.data(), packet.size(), XE_OPUS_SAMPLE_RATE);

		if(samples == OPUS_INVALID_PACKET)
			return XE_INVALID_DATA;
		packet.timescale = XE_OPUS_SAMPLE_RATE;
		packet.duration = samples;

		return 0;
	}
};

static xe_opus_parser opus_parser;

class xe_opus_encoder : public xe_av_codec{
public:
	xe_opus_encoder(): xe_av_codec(XE_CODEC_OPUS){}

	int init(xe_codec_parameters& params){
		if(!opus_encoder)
			return XE_ENOSYS;
		return open(opus_encoder, params);
	}
};

class xe_opus_decoder : public xe_av_codec{
public:
	xe_opus_decoder(): xe_av_codec(XE_CODEC_OPUS){}

	int init(xe_codec_parameters& params){
		if(!opus_decoder)
			return XE_ENOSYS;
		return open(opus_decoder, params);
	}
};

xe_codec* xe_opus::encoder(){
	return xe_new<xe_opus_encoder>();
}

xe_codec* xe_opus::decoder(){
	return xe_new<xe_opus_decoder>();
}

xe_codec_parser* xe_opus::parser(){
	return &opus_parser;
}