#include "codec.h"
#include "codecs/opus.h"
#include "codecs/aac.h"
#include "codecs/vorbis.h"
#include "codecs/flac.h"
#include "codecs/mp3.h"
#include "codecs/mp2.h"

using namespace xetrov;

int xe_codec::open(xe_codec** out, xe_codec_parameters& params, xe_codec_mode mode){
	xe_codec* codec;

	if(mode == XE_CODEC_DECODE){
		switch(params.id){
			case XE_CODEC_OPUS:
				codec = xe_opus::decoder();

				break;
			case XE_CODEC_AAC:
				codec = xe_aac::decoder();

				break;
			case XE_CODEC_VORBIS:
				codec = xe_vorbis::decoder();

				break;
			case XE_CODEC_FLAC:
				codec = xe_flac::decoder();

				break;
			case XE_CODEC_MP3:
				codec = xe_mp3::decoder();

				break;
			case XE_CODEC_MP2:
				codec = xe_mp2::decoder();

				break;
			default:
				return XE_ENOSYS;
		}
	}else{
		switch(params.id){
			case XE_CODEC_OPUS:
				codec = xe_opus::encoder();

				break;
			case XE_CODEC_AAC:
				codec = xe_aac::encoder();

				break;
			case XE_CODEC_VORBIS:
				codec = xe_vorbis::encoder();

				break;
			case XE_CODEC_FLAC:
				codec = xe_flac::encoder();

				break;
			case XE_CODEC_MP3:
				codec = xe_mp3::encoder();

				break;
			case XE_CODEC_MP2:
				codec = xe_mp2::encoder();

				break;
			default:
				return XE_ENOSYS;
		}
	}

	if(!codec)
		return XE_ENOMEM;
	int err = codec -> init(params);

	if(err){
		xe_delete(codec);

		return err;
	}

	*out = codec;

	return 0;
}

int xe_codec_parser::open(xe_codec_parser** out, xe_codec_parameters& params){
	xe_codec_parser* parser;

	switch(params.id){
		case XE_CODEC_OPUS:
			parser = xe_opus::parser();

			break;
		case XE_CODEC_VORBIS:
			parser = xe_vorbis::parser();

			break;
		default:
			return XE_ENOSYS;
	}

	if(!parser)
		return XE_ENOMEM;
	int err = parser -> init(params);

	if(err){
		parser -> close();

		return err;
	}

	*out = parser;

	return 0;
}