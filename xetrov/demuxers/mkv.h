#pragma once

namespace xetrov{

class xe_matroska_class : public xe_demuxer_class{
public:
	xe_matroska_class(){}

	xe_demuxer* create(xe_format::xe_context& context) const;
	bool probe(xe_reader& reader) const;

	~xe_matroska_class(){}
};

}