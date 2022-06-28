#pragma once
#include "types.h"

extern "C" {
	#include <libavutil/buffer.h>
}

namespace xetrov{

class xe_buffer;
typedef void (*xe_buffer_free_fn)(xe_ptr user, xe_buffer* buffer);

class xe_buffer_ref{
private:
	union{
		AVBufferRef ref_;

		struct{
			xe_buffer* buffer_;
			xe_bptr data_;
			size_t size_;
		};
	};
public:
	xe_buffer_ref();

	xe_buffer_ref& operator=(AVBufferRef& ref);

	bool create(size_t size, xe_buffer_free_fn fn, xe_ptr user);

	void ref(xe_buffer* buffer);
	void ref(const xe_buffer_ref& other);
	void unref();

	xe_cbptr data() const;
	xe_bptr data();
	size_t size() const;

	AVBufferRef& get();

	~xe_buffer_ref();
};

}