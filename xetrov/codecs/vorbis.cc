#include "vorbis.h"
#include "av.h"
#include "../error.h"

using namespace xetrov;

static const AVCodec* vorbis_decoder = avcodec_find_decoder(AV_CODEC_ID_VORBIS);
static const AVCodec* vorbis_encoder = avcodec_find_encoder(AV_CODEC_ID_VORBIS);

class xe_vorbis_parser : public xe_codec_parser{
public:
	xe_vorbis_parser(): xe_codec_parser(XE_CODEC_VORBIS){}

	int init(xe_codec_parameters& params){
		return 0;
	}

	int parse(xe_packet& packet){
		return 0;
	}
};

static xe_vorbis_parser vorbis_parser;

class xe_vorbis_encoder : public xe_av_codec{
public:
	xe_vorbis_encoder(): xe_av_codec(XE_CODEC_VORBIS){}

	int init(xe_codec_parameters& params){
		if(!vorbis_encoder)
			return XE_ENOSYS;
		return open(vorbis_encoder, params);
	}
};

class xe_vorbis_decoder : public xe_av_codec{
public:
	xe_vorbis_decoder(): xe_av_codec(XE_CODEC_VORBIS){}

	int init(xe_codec_parameters& params){
		if(!vorbis_decoder)
			return XE_ENOSYS;
		return open(vorbis_decoder, params);
	}
};

xe_codec* xe_vorbis::encoder(){
	return xe_new<xe_vorbis_encoder>();
}

xe_codec* xe_vorbis::decoder(){
	return xe_new<xe_vorbis_decoder>();
}

xe_codec_parser* xe_vorbis::parser(){
	return &vorbis_parser;
}