#pragma once
#include <utility>
#include "buffer.h"
#include "common.h"
#include "error.h"
#include "xe/mem.h"
#include "codec.h"
#include "packet.h"

namespace xetrov{

class xe_packet_buffer{
private:
	xe_buffer_ref buf;
	xe_bptr head;
	size_t left;

	friend class xe_packet_buffer_pool;
public:
	xe_packet_buffer(){
		head = null;
		left = 0;
	}

	size_t has(size_t size) const{
		return left > size + XE_BUFFER_PADDING;
	}

	void alloc(xe_packet& packet, size_t size){
		packet.ref.ref(buf);

		xe_zero(head + size, XE_BUFFER_PADDING);

		packet.buffer = xe_array<byte>(head, size);
		head += size + XE_BUFFER_PADDING;
		left -= size + XE_BUFFER_PADDING;
	}

	void unref(){
		buf.unref();
	}
};

class xe_packet_buffer_pool{
private:
	struct xe_packet_buffer_node{
		xe_packet_buffer_node* next;
		xe_packet_buffer_pool* pool;
		xe_buffer* buffer;
	};

	xe_packet_buffer_node* head;

	static void restore(xe_ptr ptr, xe_buffer* buffer){
		xe_packet_buffer_node& node = *(xe_packet_buffer_node*)ptr;
		xe_packet_buffer_pool& pool = *node.pool;

		node.next = pool.head;
		node.buffer = buffer;
		pool.head = &node;
	}
public:
	enum{
		XE_BUFFER_SIZE = 16384
	};

	xe_packet_buffer_pool(){
		head = null;
	}

	bool get_buffer(xe_packet_buffer& buffer, size_t size){
		buffer.unref();
		size = xe_max<size_t>(size + XE_BUFFER_PADDING, XE_BUFFER_SIZE);

		if(size == XE_BUFFER_SIZE && head){
			xe_packet_buffer_node& node = *head;

			buffer.left = size;
			buffer.buf.ref(node.buffer);
			buffer.head = buffer.buf.data();
			head = node.next;
		}else{
			xe_packet_buffer_node* node = xe_alloc<xe_packet_buffer_node>();

			if(!node)
				return false;
			if(!buffer.buf.create(size, size == XE_BUFFER_SIZE ? restore : null, node)){
				xe_dealloc(node);

				return false;
			}

			buffer.left = size;
			buffer.head = buffer.buf.data();
			node -> pool = this;
		}

		return true;
	}

	~xe_packet_buffer_pool(){
		xe_packet_buffer_node* next;

		while(head){
			next = head -> next;

			xe_dealloc(head -> buffer);
			xe_dealloc(head);

			head = next;
		}
	}
};

}