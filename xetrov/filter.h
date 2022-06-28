#pragma once
#include "types.h"
#include "frame.h"

namespace xetrov{

class xe_filter{
	xe_array<xe_audio_sample_fmt> formats;
	uint out_samples;
	uint in_samples;

	int init(xe_audio_sample_fmt fmt);
public:
	xe_filter(){}

	const xe_array<xe_audio_sample_fmt>& input_formats() const{
		return formats;
	}

	uint input_samples() const{
		return in_samples;
	}

	uint output_samples() const{
		return out_samples;
	}

	virtual int filter(xe_ptr data, uint samples) = 0;
};

class xe_filter_chain{
private:
	xe_vector<xe_filter*> filters;
	xe_vector<xe_frame> frame_queue;
public:
	xe_filter_chain();

	int add_filter(xe_filter* filter);
	int configure(xe_audio_sample_fmt fmt, uint sample_rate, uint channels, uint channel_layout);

	int send_frame(xe_frame& frame);
	int receive_frame(xe_frame& frame);

	~xe_filter_chain();
};

}