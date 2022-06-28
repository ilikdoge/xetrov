#include "filter.h"
#include "error.h"

using namespace xetrov;

xe_filter_chain::xe_filter_chain(){

}

int xe_filter_chain::add_filter(xe_filter* filter){
	if(filters.push_back(filter))
		return 0;
	return XE_ENOMEM;
}

int xe_filter_chain::configure(xe_audio_sample_fmt fmt, uint sample_rate, uint channels, uint channel_layout){

}

int xe_filter_chain::send_frame(xe_frame& frame){
	if(!frame_queue.push_back(std::move(frame)))
		return XE_ENOMEM;
}

int xe_filter_chain::receive_frame(xe_frame& frame){

}

xe_filter_chain::~xe_filter_chain(){

}