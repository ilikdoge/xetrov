#pragma once
#include "../codec.h"

namespace xetrov{

class xe_mp3{
public:
	static xe_codec* encoder();
	static xe_codec* decoder();
	static xe_codec_parser* parser();
};

}