#include "mp2.h"
#include "av.h"
#include "../error.h"

using namespace xetrov;

static const AVCodec* mp2_decoder = avcodec_find_decoder(AV_CODEC_ID_MP2);
static const AVCodec* mp2_encoder = avcodec_find_encoder(AV_CODEC_ID_MP2);

class xe_mp2_parser : public xe_codec_parser{
public:
	xe_mp2_parser(): xe_codec_parser(XE_CODEC_MP2){}

	int init(xe_codec_parameters& params){
		return 0;
	}

	int parse(xe_packet& packet){
		return 0;
	}
};

static xe_mp2_parser mp2_parser;

class xe_mp2_encoder : public xe_av_codec{
public:
	xe_mp2_encoder(): xe_av_codec(XE_CODEC_MP2){}

	int init(xe_codec_parameters& params){
		if(!mp2_encoder)
			return XE_ENOSYS;
		return open(mp2_encoder, params);
	}
};

class xe_mp2_decoder : public xe_av_codec{
public:
	xe_mp2_decoder(): xe_av_codec(XE_CODEC_MP2){}

	int init(xe_codec_parameters& params){
		if(!mp2_decoder)
			return XE_ENOSYS;
		return open(mp2_decoder, params);
	}
};

xe_codec* xe_mp2::encoder(){
	return xe_new<xe_mp2_encoder>();
}

xe_codec* xe_mp2::decoder(){
	return xe_new<xe_mp2_decoder>();
}

xe_codec_parser* xe_mp2::parser(){
	return &mp2_parser;
}