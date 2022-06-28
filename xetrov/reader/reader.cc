#include "../error.h"
#include "../common.h"
#include "reader.h"
#include "xe/loop.h"
#include "xe/assert.h"
#include "xe/log.h"
#include "xe/mem.h"
#include <byteswap.h>

using namespace xetrov;

/* Implementation details
 * ==============================================================
 * to minimize memory copying, the reader will attempt to
 * read directly from the stream's volatile shared io buffer
 * and only copy the data to the reader's buffer if there is data
 * left over when the worker manually suspends
 *
 * "buffer" refers to the reader's internal buffer
 * "input" refers to the stream's volatile shared io buffer
 * "data" refers to the pointer to the read head
 *
 * peek mode allows rewinding the read head back to when peeking was enabled
 *
 * buffer_length is not actually consistent with the buffer's length,
 * only an indication that there is data left in there **to be read**
 */

enum{
	HARD_SEEK_THRESHOLD = 0x80000,
	BUFFER_SIZE = XE_LOOP_IOBUF_SIZE * 2,
	BUFFER_LIMIT = XE_LOOP_IOBUF_SIZE
};

int xe_reader::write_cb(xe_stream& stream, xe_ptr buf, size_t len){
	xe_reader& reader = *(xe_reader*)stream.data;

	return reader.write((xe_bptr)buf, len);
}

void xe_reader::done_cb(xe_stream& stream, int error){
	xe_reader& reader = *(xe_reader*)stream.data;

	reader.done(error);
}

int xe_reader::write(xe_bptr buf, size_t len){
	if(!read_length){
		xe_assertm(false, "bad call to write()");

		/* fatal error, stream status will be set to XE_ABORTED after callback */
		return err = XE_ABORTED;
	}

	if(len > BUFFER_LIMIT){
		/* avoid buffer overflow, even when peeking */
		xe_assertm(false, "buffer too big");

		/* set stream error here */
		return err = stream_status = XE_BUFFER_TOO_SMALL;
	}

	if(peeking)
		peek.input_length = len;
	if(read_to || skipping){
		size_t min = xe_min(read_length, len);

		read_length -= min;

		if(read_to){
			xe_memcpy(read_to, buf, min);

			read_to += min;
		}

		if(read_length){
			/* still need to read some more */
			return 0;
		}

		buf += min;
		len -= min;
	}

	/* this function will only be called when all data is exhausted
	 * it is guaranteed that there is no more data left in the buffer */
	data = buf;
	length = len;

	/* we either completed a read or have data, let the worker do some work */
	if(resume())
		return 0;
	if(peeking){
		if(peek.buffer_offset && peek.buffer_offset + peek.buffer_length + peek.input_length > BUFFER_SIZE){
			/* make room for stream input */
			if(peek.buffer_length)
				xe_memmove(buffer, buffer + peek.buffer_offset, peek.buffer_length);
			peek.buffer_offset = 0;
		}

		if(peek.input_length){
			/* append stream input */
			xe_memcpy(buffer + peek.buffer_length, buf + len - peek.input_length, peek.input_length);

			peek.buffer_length += peek.input_length;
			peek.input_length = 0;
		}
	}

	if(!read_length){
		/* worker suspended without requesting more data so
		 * save the data to our internal buffer and pause the stream */
		stream -> pause(true);
		paused = true;

		/* the next time data will be read, it will be from the internal buffer */
		if(peeking){
			data = buffer + peek.buffer_offset;
			length = peek.buffer_length;
		}else{
			/* buffer length is only non-zero here if manually suspending after a peek rewind */
			if(buffer_length){
				/* we have not exhausted the buffer yet */
				size_t buffer_offset = data - buffer;

				if(buffer_offset + length + len > BUFFER_SIZE){
					/* combined data exceeds, we need to move the buffer */
					xe_memmove(buffer, data, length);

					data = buffer;
				}

				xe_memcpy(data + length, buf, len);

				length += len;
			}else{
				/* no data in the buffer */
				xe_memcpy(buffer, data, length);

				data = buffer;
			}

			buffer_length = length;
		}
	}

	return 0;
}

void xe_reader::done(int error){
	if(!stream_status)
		stream_status = error ? error : XE_EOF;
	if(read_length){
		/* we were waiting for more data, but we have no more */
		if(!err)
			err = stream_status;
		resume();
	}
}

inline void xe_reader::update_buffer(){
	/* we exhausted our current buffer, move data pointer to the stream's input */
	if(!length){
		data = input;
		length = input_length;
		buffer_length = 0;
		input_length = 0;
	}
}

inline bool xe_reader::resume(){
	callback = true;
	worker -> resume();
	callback = false;

	if(closing){
		stream -> close();
		worker -> resume();

		return true;
	}

	return false;
}

inline int xe_reader::wait(){
	if(stream_status)
		/* stream has already ended */
		return err = stream_status;
	if(peeking && peek.buffer_length + peek.input_length > BUFFER_LIMIT){
		/* if we read any more data, it's possible that saving
		 * the input to the buffer will overflow */
		return err = XE_EOF;
	}

	if(paused){
		stream -> pause(false);
		paused = false;
	}

	xe_log_trace(this, "worker suspend");

	worker -> suspend();

	xe_log_trace(this, "worker resume");

	return err;
}

template<size_t size>
ulong xe_reader::read(){
	ulong value;

	off += size;

	if(err)
		return 0;
	if(length < size){
		off -= size;

		if(read(&value, size))
			value = 0;
	}else{
		xe_memcpy(&value, data, size);

		length -= size;
		data += size;

		update_buffer();
	}

	return value;
}

xe_reader::xe_reader(){
	stream = null;
	worker = null;

	buffer = null;
	buffer_length = 0;
	input = null;
	input_length = 0;
	data = null;
	length = 0;
	read_to = null;
	read_length = 0;

	peek.buffer_length = 0;
	peek.buffer_offset = 0;
	peek.input_length = 0;
	peek.off = 0;

	off = 0;

	stream_status = 0;
	err = 0;

	paused = false;
	peeking = false;
	closing = false;
	skipping = false;
	callback = false;
}

int xe_reader::init(xe_fiber_worker& worker_, xe_stream& stream_){
	xe_bptr buf = xe_alloc<byte>(BUFFER_SIZE);

	if(!buf)
		return XE_ENOMEM;
	buffer = buf;
	stream = &stream_;
	worker = &worker_;
	stream -> set_write_cb(write_cb);
	stream -> set_done_cb(done_cb);
	stream -> data = this;

	return 0;
}

void xe_reader::close(){
	if(!stream)
		return;
	if(callback){
		/* we are in the worker, so we need to defer the close
		 * otherwise invalid memory will be accessed after worker
		 * yields and returns to the write call */
		closing = true;
		worker -> suspend();
	}else{
		stream -> close();
	}
}

void xe_reader::peek_mode(bool enable){
	if(peeking == enable)
		return;
	peeking = enable;

	if(enable){
		peek.off = off;

		if(buffer_length){
			/* we have data in the buffer */
			peek.buffer_offset = data - buffer;
			peek.buffer_length = length;
			peek.input_length = input_length;
		}else{
			/* whatever stream data we have is our current data pointer */
			peek.input_length = length;
		}
	}else{
		if(peek.input_length && !buffer_length){
			/* we ran out of data in the buffer and read into the input
			 * restore the stream input's original position */
			if(peek.buffer_length){
				/* new data pointer points to the buffer */
				input -= peek.input_length - length;
				input_length = peek.input_length;
			}else{
				/* new data pointer points to the stream input */
				data -= peek.input_length - length;
				length = peek.input_length;
			}
		}

		if(peek.buffer_length){
			/* restore the buffer's original position */
			data = buffer + peek.buffer_offset;
			length = peek.buffer_length;
			buffer_length = peek.buffer_length;
		}

		if(length) /* only report an error if we read more data than we have */
			err = 0;
		off = peek.off;
		peek.buffer_length = 0;
		peek.buffer_offset = 0;
		peek.input_length = 0;
	}
}

int xe_reader::seek(ulong offset){
	int res;

	xe_assertm(!peeking, "cannot seek in peek mode");

	if((res = stream -> seek(offset)))
		return stream_status = res;
	off = offset;
	buffer_length = 0;
	input_length = 0;
	stream_status = 0;
	length = 0;
	err = 0;

	return 0;
}

int xe_reader::skip(ulong len){
	off += len;

	if(err)
		return err;
	if(!len)
		return 0;
	size_t available = length + input_length, min;

	if(!peeking && len > available + HARD_SEEK_THRESHOLD && stream -> seekable())
		return err = seek(off);
	min = xe_min(len, length);
	len -= min;
	length -= min;
	data += min;

	update_buffer();

	if(!len)
		return 0;
	if(length){
		min = xe_min(len, length);
		len -= min;
		length -= min;
		data += len;

		if(!len)
			return 0;
	}

	read_length = len;
	skipping = true;

	wait();

	skipping = false;

	return err;
}

int xe_reader::read(xe_ptr buf, size_t len){
	xe_bptr dest;
	size_t min;

	off += len;

	if(err)
		return err;
	if(!len)
		return 0;
	dest = (xe_bptr)buf;
	min = xe_min(len, length);

	xe_memcpy(dest, data, min);

	length -= min;
	len -= min;
	data += min;

	update_buffer();

	if(!len)
		return 0;
	dest += min;

	if(length){
		min = xe_min(len, length);

		xe_memcpy(dest, data, min);

		length -= min;
		len -= min;
		data += min;

		if(!len)
			return 0;
	}

	read_length = len;
	read_to = dest;

	wait();

	read_to = null;

	return err;
}

ulong xe_reader::r64le(){
	return read<8>();
}

ulong xe_reader::r64be(){
	return bswap_64(r64le());
}

uint xe_reader::r32le(){
	return read<4>();
}

uint xe_reader::r32be(){
	return bswap_32(r32le());
}

uint xe_reader::r24le(){
	return read<3>() & 0x00ffffff;
}

uint xe_reader::r24be(){
	return bswap_32(r24le() << 8);
}

ushort xe_reader::r16le(){
	return read<2>();
}

ushort xe_reader::r16be(){
	return bswap_16(r16le());
}

byte xe_reader::r8(){
	return read<1>();
}

float xe_reader::f32be(){
	union{
		uint u;
		float f;
	} c = {r32be()};

	return c.f;
}

float xe_reader::f32le(){
	union{
		uint u;
		float f;
	} c = {r32le()};

	return c.f;
}

double xe_reader::f64be(){
	union{
		ulong u;
		double f;
	} c = {r64be()};

	return c.f;
}

double xe_reader::f64le(){
	union{
		ulong u;
		double f;
	} c = {r64le()};

	return c.f;
}

ulong xe_reader::offset(){
	return off;
}

int xe_reader::error(){
	return err;
}

bool xe_reader::eof(){
	return stream_status && !length && !input_length;
}

xe_cstr xe_reader::class_name(){
	return "xe_reader";
}