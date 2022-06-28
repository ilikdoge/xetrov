#pragma once
#include "resource.h"
#include "xurl/ctx.h"

namespace xetrov{

class xe_net_stream;
class xe_net_resource : public xe_resource{
private:
	xurl::xurl_ctx* ctx;
	xe_string url;

	uint max_redirects;
	uint recvbuf_size;
	bool ssl_verify;
	bool follow_location;
	xurl::xe_ip_mode ip_mode;

	friend class xe_net_stream;
public:
	xe_net_resource();

	int init(xurl::xurl_ctx& ctx, xe_string url);

	void set_max_redirects(uint max);
	void set_ssl_verify(bool verify);
	void set_follow_location(bool follow);
	void set_ip_mode(xurl::xe_ip_mode mode);
	void set_recvbuf_size(uint size);

	xe_stream* create();

	void close();
};

}