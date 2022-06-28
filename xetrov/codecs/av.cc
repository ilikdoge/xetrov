#include "av.h"

using namespace xetrov;

xe_av_codec::xe_av_codec(xe_codec_id id): xe_codec(id){}

int xe_av_codec::open(const AVCodec* codec, xe_codec_parameters& params){
	int err;

	context = avcodec_alloc_context3(codec);

	if(!context)
		return XE_ENOMEM;
	context -> time_base = {1, (int)params.sample_rate};
	context -> bit_rate = params.bit_rate;
	context -> sample_rate = params.sample_rate;
	context -> channels = params.channels;
	context -> channel_layout = params.channel_layout;
	context -> sample_fmt = (AVSampleFormat)params.format;
	context -> extradata = params.config.data();
	context -> extradata_size = params.config.size();
	err = avcodec_open2(context, null, null);
	context -> extradata = null;
	context -> extradata_size = 0;

	switch(err){
		case 0:
			break;
		case AVERROR(ENOSYS):
			return XE_ENOSYS;
		case AVERROR(ENOMEM):
			return XE_ENOMEM;
		case AVERROR(EINVAL):
			return XE_EINVAL;
		case AVERROR_INVALIDDATA:
			return XE_INVALID_DATA;
		case AVERROR_PATCHWELCOME:
			return XE_ENOSYS;
		default:
			return XE_EXTERNAL;
	}

	return 0;
}

int xe_av_codec::send_packet(xe_packet& packet){
	AVPacket avpacket;
	int err;

	xe_zero(&avpacket);

	avpacket.buf = &packet.ref.get();
	avpacket.data = packet.data();
	avpacket.size = packet.size();
	avpacket.dts = packet.timestamp;
	avpacket.pts = packet.timestamp;
	avpacket.duration = packet.duration;

	switch(avcodec_send_packet(context, &avpacket)){
		case 0:
			break;
		case AVERROR(ENOMEM):
			return XE_ENOMEM;
		case AVERROR(EAGAIN):
			return XE_EAGAIN;
		case AVERROR(EINVAL):
			return XE_EINVAL;
		case AVERROR_EOF:
			return XE_EOF;
		case AVERROR_INVALIDDATA:
			return XE_INVALID_DATA;
		case AVERROR_PATCHWELCOME:
			return XE_ENOSYS;
		default:
			return XE_EXTERNAL;
	}

	return 0;
}

int xe_av_codec::send_frame(xe_frame& frame){
	AVFrame avframe;

	xe_zero(&avframe);

	avframe.nb_samples = frame.samples;
	avframe.sample_rate = frame.sample_rate;
	avframe.channels = frame.channels;
	avframe.channel_layout = frame.channel_layout;
	avframe.pkt_duration = frame.duration;
	avframe.best_effort_timestamp = frame.timestamp;
	avframe.format = (AVSampleFormat)frame.format;
	avframe.extended_buf = (AVBufferRef**)frame.internal.extended_buf.data();
	avframe.nb_extended_buf = frame.internal.extended_buf.size();

	if(frame.data != frame.internal.data)
		avframe.extended_data = frame.data;
	else
		avframe.extended_data = avframe.data;
	xe_tmemcpy(&avframe.data, frame.internal.data);
	xe_tmemcpy(&avframe.buf, frame.internal.bufs);
	xe_tmemcpy(&avframe.linesize, frame.internal.linesize);

	switch(avcodec_send_frame(context, &avframe)){
		case 0:
			break;
		case AVERROR(EAGAIN):
			return XE_EAGAIN;
		case AVERROR_EOF:
			return XE_EOF;
		case AVERROR(EINVAL):
			return XE_EINVAL;
		case AVERROR(ENOMEM):
			return XE_ENOMEM;
		default:
			return XE_EXTERNAL;
	}

	return 0;
}

int xe_av_codec::receive_frame(xe_frame& frame){
	AVFrame avframe;

	xe_zero(&avframe);

	switch(avcodec_receive_frame(context, &avframe)){
		case 0:
			break;
		case AVERROR(EAGAIN):
			return XE_EAGAIN;
		case AVERROR_EOF:
			return XE_EOF;
		case AVERROR(EINVAL):
			return XE_EINVAL;
		default:
			return XE_EXTERNAL;
	}

	frame.samples = avframe.nb_samples;
	frame.sample_rate = avframe.sample_rate;
	frame.channels = avframe.channels;
	frame.channel_layout = avframe.channel_layout;
	frame.duration = avframe.pkt_duration;
	frame.timestamp = avframe.best_effort_timestamp;
	frame.internal.extended_buf = xe_array<xe_buffer_ref*>((xe_buffer_ref**)avframe.extended_buf, avframe.nb_extended_buf);
	frame.format = (xe_audio_sample_fmt)avframe.format;
	avframe.nb_extended_buf = 0;
	avframe.extended_buf = null;

	if(avframe.extended_data != avframe.data)
		frame.data = avframe.extended_data;
	else{
		frame.data = frame.internal.data;

		xe_tmemcpy(&frame.internal.data, avframe.data);
	}

	xe_tmemcpy(&frame.internal.bufs, avframe.buf);
	xe_zero(&avframe.buf);

	xe_tmemcpy(&frame.internal.linesize, avframe.linesize);
	av_frame_unref(&avframe);

	return 0;
}

int xe_av_codec::receive_packet(xe_packet& packet){
	AVPacket avpacket;

	xe_zero(&avpacket);

	switch(avcodec_receive_packet(context, &avpacket)){
		case 0:
			break;
		case AVERROR(EAGAIN):
			return XE_EAGAIN;
		case AVERROR_EOF:
			return XE_EOF;
		case AVERROR(EINVAL):
			return XE_EINVAL;
		default:
			return XE_EXTERNAL;
	}

	packet.ref = *avpacket.buf;
	packet.buffer = xe_array<byte>(avpacket.data, avpacket.size);
	packet.timestamp = avpacket.dts;
	packet.duration = avpacket.duration;
	packet.flags = avpacket.flags;
	avpacket.buf = null;

	av_packet_unref(&avpacket);

	return 0;
}

int xe_av_codec::drain(){
	switch(avcodec_send_packet(context, null)){
		case 0:
			break;
		case AVERROR(ENOMEM):
			return XE_ENOMEM;
		case AVERROR_INVALIDDATA:
			return XE_INVALID_DATA;
		default:
			return XE_EXTERNAL;
	}

	return 0;
}

void xe_av_codec::flush(){
	avcodec_flush_buffers(context);
}

xe_av_codec::~xe_av_codec(){
	avcodec_free_context(&context);
}