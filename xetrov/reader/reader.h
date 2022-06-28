#pragma once
#include "../types.h"
#include "../resource/stream.h"
#include "../worker.h"

namespace xetrov{

class xe_reader{
private:
	xe_stream* stream;
	xe_fiber_worker* worker;

	xe_bptr buffer;
	size_t buffer_length;

	xe_bptr input;
	size_t input_length;

	xe_bptr data;
	size_t length;

	xe_bptr read_to;
	size_t read_length;

	struct{
		size_t buffer_length;
		size_t buffer_offset;
		size_t input_length;
		size_t off;
	} peek;

	ulong off;

	int stream_status;
	int err;

	bool paused;
	bool peeking;
	bool closing;
	bool skipping;
	bool callback;

	static int write_cb(xe_stream& stream, xe_ptr buf, size_t len);
	static void done_cb(xe_stream& stream, int error);

	int write(xe_bptr buf, size_t len);
	void done(int error);

	void update_buffer();
	bool resume();
	int wait();

	template<size_t size>
	ulong read();
public:
	xe_reader();

	int init(xe_fiber_worker& worker, xe_stream& stream);
	void close();

	void peek_mode(bool enable);

	int seek(ulong offset);
	int skip(ulong len);
	int read(xe_ptr buf, size_t len);

	ulong r64le();
	ulong r64be();

	uint r32le();
	uint r32be();

	uint r24le();
	uint r24be();

	ushort r16le();
	ushort r16be();

	byte r8();

	float f32be();
	float f32le();

	double f64be();
	double f64le();

	ulong offset();

	int error();

	bool eof();

	static xe_cstr class_name();
};

}