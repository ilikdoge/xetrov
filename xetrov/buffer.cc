#include <atomic>
#include "common.h"
#include "buffer.h"
#include "xe/mem.h"

enum{
	AV_BUFFER_FLAG_NO_FREE = 0x2
};

using namespace xetrov;

static void xe_buffer_default_free(xe_ptr user, xe_buffer* buffer){
	xe_dealloc(buffer);
}

class xetrov::xe_buffer{
public:
	xe_buffer* self;
	size_t size;
	std::atomic<uint> refcount;
	xe_buffer_free_fn free;
	xe_ptr opaque;
	uint flags;
	uint flags_internal;

	xe_buffer(size_t len, xe_buffer_free_fn fn, xe_ptr user): refcount(1){
		self = this;
		size = len;
		free = fn ? fn : xe_buffer_default_free;
		opaque = user;
		flags = 0;
		flags_internal = AV_BUFFER_FLAG_NO_FREE;
	}

	xe_bptr data(){
		return (xe_bptr)self + sizeof(*this);
	}

	void ref(){
		refcount.fetch_add(1, std::memory_order_relaxed);
	}

	void unref(){
		if(refcount.fetch_sub(1, std::memory_order_relaxed) == 1){
			bool self_free = !(flags_internal & AV_BUFFER_FLAG_NO_FREE);

			free(opaque, self);

			if(self_free)
				xe_dealloc(this);
		}
	}
};

xe_buffer_ref::xe_buffer_ref(){
	buffer_ = null;
	data_ = null;
	size_ = 0;
}

xe_buffer_ref& xe_buffer_ref::operator=(AVBufferRef& ref){
	unref();

	ref_ = ref;

	return *this;
}

bool xe_buffer_ref::create(size_t size, xe_buffer_free_fn fn, xe_ptr user){
	size_t total;
	xe_buffer* buffer;

	unref();

	if(xe_overflow_add(total, size, sizeof(xe_buffer)))
		return false;
	buffer = (xe_buffer*)xe_alloc<byte>(total);

	if(!buffer)
		return false;
	xe_construct(buffer, size, fn, user);

	buffer_ = buffer;
	data_ = buffer -> data();
	size_ = size;

	return true;
}

void xe_buffer_ref::ref(xe_buffer* buffer){
	unref();

	buffer_ = buffer;
	data_ = buffer -> data();
	size_ = buffer -> size;
	buffer_ -> ref();
}

void xe_buffer_ref::ref(const xe_buffer_ref& other){
	unref();

	buffer_ = other.buffer_;
	data_ = other.data_;
	size_ = other.size_;
	buffer_ -> ref();
}

void xe_buffer_ref::unref(){
	if(buffer_){
		buffer_ -> unref();
		buffer_ = null;
	}
}

xe_cbptr xe_buffer_ref::data() const{
	return data_;
}

xe_bptr xe_buffer_ref::data(){
	return data_;
}

size_t xe_buffer_ref::size() const{
	return size_;
}

AVBufferRef& xe_buffer_ref::get(){
	return ref_;
}

xe_buffer_ref::~xe_buffer_ref(){
	unref();
}