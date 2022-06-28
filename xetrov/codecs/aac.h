#pragma once
#include "../codec.h"

namespace xetrov{

class xe_aac{
public:
	static int parse_config(xe_codec_parameters& context);
	static xe_codec* encoder();
	static xe_codec* decoder();
	static xe_codec_parser* parser();
};

}