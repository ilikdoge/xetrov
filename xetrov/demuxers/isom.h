#pragma once
#include "../demuxer.h"

namespace xetrov{

class xe_isom_class : public xe_demuxer_class{
public:
	xe_isom_class(){}

	xe_demuxer* create(xe_format::xe_context& context) const;
	bool probe(xe_reader& reader) const;

	~xe_isom_class(){}
};

}