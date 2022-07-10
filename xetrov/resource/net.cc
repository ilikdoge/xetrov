#include "net.h"
#include "xurl/request.h"
#include "../common.h"

using namespace xurl;
using namespace xetrov;

class xetrov::xe_net_stream : public xe_stream{
public:
	static int write_cb(xe_request& request, xe_ptr buf, size_t len){
		xe_net_stream& stream = xe_containerof(request, &xe_net_stream::request);

		int err;

		if(stream.callbacks.write){
			stream.callback = true;
			err = stream.callbacks.write(stream, buf, len);
			stream.callback = false;

			if(!err && stream.stop)
				return XE_ABORTED;
			return err;
		}

		return 0;
	}

	static void done_cb(xe_request& request, int error){
		xe_net_stream& stream = xe_containerof(request, &xe_net_stream::request);

		if(stream.reopen && error == XE_ABORTED){
			error = stream.open(stream.current_start, stream.current_end);

			if(!error)
				return;
		}

		if(stream.closing)
			xe_delete(&stream);
		else if(stream.callbacks.done){
			stream.callback = true;
			stream.callbacks.done(stream, error);
			stream.callback = false;
		}
	}

	xe_request request;
	xurl_ctx* ctx;
	xe_net_resource* resource;
	ulong current_end;
	ulong current_start;

	bool opened: 1;
	bool callback: 1;
	bool reopen: 1;
	bool stop: 1;
	bool closing: 1;

	xe_net_stream(xurl_ctx* ctx_, xe_net_resource* resource_){
		ctx = ctx_;
		resource = resource_;
		request.set_write_cb(write_cb);
		request.set_done_cb(done_cb);
		seekable_ = true;
	}

	int open(ulong start, ulong end){
		int err;

		if(!opened){
			if((err = ctx -> open(request, resource -> url)))
				return err;
			if(resource -> max_redirects)
				request.set_max_redirects(resource -> max_redirects);
			request.set_follow_location(resource -> follow_location);
			request.set_ssl_verify(resource -> ssl_verify);
			request.set_ip_mode(resource -> ip_mode);
			request.set_recvbuf_size(resource -> recvbuf_size);
			opened = true;
		}

		current_end = end;
		stop = false;
		reopen = false;

		if(!request.set_http_header("Accept", "*/*"))
			return XE_ENOMEM;
		if(start || end){
			char range[60];

			if(end)
				snprintf(range, sizeof(range), "bytes=%lu-%lu", start, end - 1);
			else
				snprintf(range, sizeof(range), "bytes=%lu-", start);
			if(!request.set_http_header("Range", range, true))
				return XE_ENOMEM;
		}

		return ctx -> start(request);
	}

	int seek(ulong offset){
		current_start = offset;
		reopen = true;

		if(!callback)
			ctx -> end(request);
		else
			stop = true;
		return 0;
	}

	void pause(bool paused){
		if(ctx -> transferctl(request, paused ? XE_PAUSE_RECV : XE_RESUME_RECV))
			abort();
	}

	void abort(){
		if(!callback)
			ctx -> end(request);
		else{
			stop = true;
			reopen = false;
		}
	}

	void close(){
		abort();

		if(!callback)
			request.close();
		else
			closing = true;
	}
};

xe_net_resource::xe_net_resource(){

}

int xe_net_resource::init(xurl_ctx& ctx_, xe_string url_){
	ctx = &ctx_;
	url = url_;
	max_redirects = 0;
	recvbuf_size = 0;
	follow_location = false;
	ssl_verify = true;
	ip_mode = XE_IP_ANY;

	return 0;
}

void xe_net_resource::set_ssl_verify(bool verify){
	ssl_verify = verify;
}

void xe_net_resource::set_max_redirects(uint max){
	max_redirects = max;
}

void xe_net_resource::set_follow_location(bool follow){
	follow_location = follow;
}

void xe_net_resource::set_ip_mode(xe_ip_mode mode){
	ip_mode = mode;
}

void xe_net_resource::set_recvbuf_size(uint size){
	recvbuf_size = size;
}

xe_stream* xe_net_resource::create(){
	return xe_znew<xe_net_stream>(ctx, this);
}

void xe_net_resource::close(){}