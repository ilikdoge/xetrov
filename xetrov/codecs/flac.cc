#include "flac.h"
#include "av.h"
#include "../error.h"

using namespace xetrov;

static const AVCodec* flac_decoder = avcodec_find_decoder(AV_CODEC_ID_FLAC);
static const AVCodec* flac_encoder = avcodec_find_encoder(AV_CODEC_ID_FLAC);

class xe_flac_parser : public xe_codec_parser{
public:
	xe_flac_parser(): xe_codec_parser(XE_CODEC_FLAC){}

	int init(xe_codec_parameters& params){
		return 0;
	}

	int parse(xe_packet& packet){
		return 0;
	}
};

static xe_flac_parser flac_parser;

class xe_flac_encoder : public xe_av_codec{
public:
	xe_flac_encoder(): xe_av_codec(XE_CODEC_FLAC){}

	int init(xe_codec_parameters& params){
		if(!flac_encoder)
			return XE_ENOSYS;
		return open(flac_encoder, params);
	}
};

class xe_flac_decoder : public xe_av_codec{
public:
	xe_flac_decoder(): xe_av_codec(XE_CODEC_FLAC){}

	int init(xe_codec_parameters& params){
		if(!flac_decoder)
			return XE_ENOSYS;
		return open(flac_decoder, params);
	}
};

xe_codec* xe_flac::encoder(){
	return xe_new<xe_flac_encoder>();
}

xe_codec* xe_flac::decoder(){
	return xe_new<xe_flac_decoder>();
}

xe_codec_parser* xe_flac::parser(){
	return &flac_parser;
}