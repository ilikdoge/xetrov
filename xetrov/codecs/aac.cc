#include "../reader/bitreader.h"
#include "../error.h"
#include "aac.h"
#include "av.h"

using namespace xetrov;

static const AVCodec* aac_decoder = avcodec_find_decoder(AV_CODEC_ID_AAC);
static const AVCodec* aac_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);

enum{
	AOT_ESCAPE = 0x1f
};

static uint sample_rate_table[] = {
	96000,	88200,	64000,	48000,	44100,	32000,	24000,	22050,	16000,	12000,	11025,
	8000,	7350,	0,		0,		57600,	51200,	40000,	38400,	34150,	28800,	25600,
	20000,	19200,	17075,	14400,	12800,	9600,	0,		0,		0,		0
};

static uint channel_table[] = {
	0, 1, 2, 3, 4, 5, 6, 8, 0, 0, 0, 7, 8, 0, 8, 0
};

int xe_aac::parse_config(xe_codec_parameters& context){
	xe_bit_reader reader(context.config.data(), 0, context.config.size() << 3);
	uint aot, sample_rate, channels;

	aot = reader.read(5);

	if(aot == AOT_ESCAPE)
		aot = 32 + reader.read(6);
	sample_rate = reader.read(4);

	if(sample_rate == 0xf)
		sample_rate = reader.read(24);
	else
		sample_rate = sample_rate_table[sample_rate];
	channels = reader.read(4);
	context.channels = channel_table[channels];
	context.sample_rate = sample_rate;

	return reader.has_bits(0) ? 0 : XE_INVALID_DATA;
}

class xe_aac_encoder : public xe_av_codec{
public:
	xe_aac_encoder(): xe_av_codec(XE_CODEC_AAC){}

	int init(xe_codec_parameters& params){
		if(!aac_encoder)
			return XE_ENOSYS;
		return open(aac_encoder, params);
	}
};

class xe_aac_decoder : public xe_av_codec{
public:
	xe_aac_decoder(): xe_av_codec(XE_CODEC_AAC){}

	int init(xe_codec_parameters& params){
		if(!aac_decoder)
			return XE_ENOSYS;
		return open(aac_decoder, params);
	}
};

xe_codec* xe_aac::encoder(){
	return xe_new<xe_aac_encoder>();
}

xe_codec* xe_aac::decoder(){
	return xe_new<xe_aac_decoder>();
}

xe_codec_parser* xe_aac::parser(){
	return null;
}