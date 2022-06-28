#include "mp3.h"
#include "av.h"
#include "../error.h"

using namespace xetrov;

static const AVCodec* mp3_decoder = avcodec_find_decoder(AV_CODEC_ID_MP3);
static const AVCodec* mp3_encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);

class xe_mp3_parser : public xe_codec_parser{
public:
	xe_mp3_parser(): xe_codec_parser(XE_CODEC_MP3){}

	int init(xe_codec_parameters& params){
		return 0;
	}

	int parse(xe_packet& packet){
		return 0;
	}
};

static xe_mp3_parser mp3_parser;

class xe_mp3_encoder : public xe_av_codec{
public:
	xe_mp3_encoder(): xe_av_codec(XE_CODEC_MP3){}

	int init(xe_codec_parameters& params){
		if(!mp3_encoder)
			return XE_ENOSYS;
		return open(mp3_encoder, params);
	}
};

class xe_mp3_decoder : public xe_av_codec{
public:
	xe_mp3_decoder(): xe_av_codec(XE_CODEC_MP3){}

	int init(xe_codec_parameters& params){
		if(!mp3_decoder)
			return XE_ENOSYS;
		return open(mp3_decoder, params);
	}
};

xe_codec* xe_mp3::encoder(){
	return xe_new<xe_mp3_encoder>();
}

xe_codec* xe_mp3::decoder(){
	return xe_new<xe_mp3_decoder>();
}

xe_codec_parser* xe_mp3::parser(){
	return &mp3_parser;
}