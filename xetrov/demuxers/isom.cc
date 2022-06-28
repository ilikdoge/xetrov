#include "../format.h"
#include "../demuxer.h"
#include "../error.h"
#include "../codecs/aac.h"
#include "isom.h"
#include "xe/log.h"
#include "xe/container/vector.h"

using namespace xetrov;

constexpr int xe_make_fourcc(xe_cstr str){
	return ((uint)str[3] << 24) | ((uint)str[2] << 16) | ((uint)str[1] << 8) | (uint)str[0];
}

enum xe_fourcc{
	FOURCC_ROOT = 0,
	FOURCC_MOOV = xe_make_fourcc("moov"),
	FOURCC_TRAK = xe_make_fourcc("trak"),
	FOURCC_MDHD = xe_make_fourcc("mdhd"),
	FOURCC_HDLR = xe_make_fourcc("hdlr"),
	FOURCC_TKHD = xe_make_fourcc("tkhd"),
	FOURCC_MDIA = xe_make_fourcc("mdia"),
	FOURCC_MINF = xe_make_fourcc("minf"),
	FOURCC_STBL = xe_make_fourcc("stbl"),
	FOURCC_STSD = xe_make_fourcc("stsd"),
	FOURCC_ESDS = xe_make_fourcc("esds"),
	FOURCC_CO64 = xe_make_fourcc("co64"),
	FOURCC_STCO = xe_make_fourcc("stco"),
	FOURCC_STSZ = xe_make_fourcc("stsz"),
	FOURCC_STSC = xe_make_fourcc("stsc"),
	FOURCC_STTS = xe_make_fourcc("stts"),
	FOURCC_STSS = xe_make_fourcc("stss"),
	FOURCC_MVEX = xe_make_fourcc("mvex"),
	FOURCC_TREX = xe_make_fourcc("trex"),
	FOURCC_SIDX = xe_make_fourcc("sidx"),
	FOURCC_TRAF = xe_make_fourcc("traf"),
	FOURCC_TFDT = xe_make_fourcc("tfdt"),
	FOURCC_TFHD = xe_make_fourcc("tfhd"),
	FOURCC_TRUN = xe_make_fourcc("trun"),
	FOURCC_MOOF = xe_make_fourcc("moof"),
	FOURCC_MDAT = xe_make_fourcc("mdat"),

	/* unused top-level boxes */
	FOURCC_FTYP = xe_make_fourcc("ftyp"),
	FOURCC_PDIN = xe_make_fourcc("pdin"),
	FOURCC_BLOC = xe_make_fourcc("bloc"),
	FOURCC_MFRA = xe_make_fourcc("mfra"),
	FOURCC_FREE = xe_make_fourcc("free"),
	FOURCC_SKIP = xe_make_fourcc("skip"),
	FOURCC_META = xe_make_fourcc("meta"),
	FOURCC_MECO = xe_make_fourcc("meco"),
	FOURCC_STYP = xe_make_fourcc("styp"),
	FOURCC_SSIX = xe_make_fourcc("ssix"),
	FOURCC_PRFT = xe_make_fourcc("prft"),
	FOURCC_UUID = xe_make_fourcc("uuid"),
	FOURCC_EMSG = xe_make_fourcc("emsg"),

	ENTRY_MP4A = xe_make_fourcc("mp4a"),
	ENTRY_ENCA = xe_make_fourcc("enca"),
	ENTRY_OPUS = xe_make_fourcc("Opus"),
	ENTRY_FLAC = xe_make_fourcc("fLaC"),

	FOURCC_VIDE = xe_make_fourcc("vide"),
	FOURCC_SOUN = xe_make_fourcc("soun")
};

enum xe_trackflags{
	TRACK_FLAG_ENABLED = 0x1,
	TRACK_FLAG_MOVIE = 0x2,
	TRACK_FLAG_PREVIEW = 0x4
};

enum xe_sample_flags{
	SAMPLE_BITS_LEADING			= 0x0c000000,
	SAMPLE_BITS_DEPENDS			= 0x03000000,
	SAMPLE_BITS_DEPENDED_ON		= 0x00c00000,
	SAMPLE_BITS_REDUNDANT 		= 0x00300000,
	SAMPLE_BITS_PADDING			= 0x000e0000,
	SAMPLE_FLAG_NONSYNC	 		= 0x00010000,
	SAMPLE_BITS_DEGREDATIONPRIO	= 0x0000ffff
};

enum xe_sample_leading{
	SAMPLE_LEADING_UNKNOWN = 0x0,
	SAMPLE_LEADING_DEPENDENT = 0x1,
	SAMPLE_LEADING_NOT = 0x2,
	SAMPLE_LEADING_INDEPENDENT = 0x3
};

enum xe_sample_depends{
	SAMPLE_DEPENDS_UNKNOWN = 0x0,
	SAMPLE_DEPENDS_YES = 0x1,
	SAMPLE_DEPENDS_NO = 0x2,
	SAMPLE_DEPENDS_RESERVED = 0x3
};

enum xe_sample_depended_on{
	SAMPLE_DEPENDED_UNKNOWN = 0x0,
	SAMPLE_DEPENDED_YES = 0x1,
	SAMPLE_DEPENDED_NO = 0x2,
	SAMPLE_DEPENDED_RESERVED = 0x3
};

enum xe_sample_redundant{
	SAMPLE_REDUNDANT_UNKNOWN = 0x0,
	SAMPLE_REDUNDANT_YES = 0x1,
	SAMPLE_REDUNDANT_NO = 0x2,
	SAMPLE_REDUNDANT_RESERVED = 0x3
};

struct xe_isom_track : public xe_track{
	uint id;
	uint default_sample_duration;
	uint default_sample_size;
	uint default_sample_flags;
	uint track_flags;
	uint alternate_group;

	xe_fourcc entry;

	struct xe_stts{
		uint count;
		uint delta;
	};

	struct xe_stsc{
		uint first;
		uint count;
	};

	uint sample_size;

	xe_array<xe_stts> sample_time;
	xe_array<xe_stsc> sample_chunk;
	xe_array<uint> sample_sizes;
	xe_array<ulong> chunk_offset;
	xe_array<uint> sync_sample;

	struct sample_index{
		uint* sample_to_chunk;
		uint* time_to_sample;
		ulong* sample_to_time;
	} index;

	uint current_chunk;
	uint current_sample;
	uint sample_chunk_index;

	struct moof_ref{
		ulong byte;
		ulong time;
	};

	xe_vector<moof_ref> moof_refs;
};

struct xe_sidx_entry{
	ulong time;
	ulong byte: 48;
	ulong contains_keyframe: 1;
	ulong starts_with_keyframe: 1;
};

struct xe_sidx{
	xe_sidx_entry* entries;
	uint timescale;
	uint track_id;
	uint length;
	uint pad;
};

struct xe_traf{
	ulong start_time;
	ulong offset;

	struct track_run{
		ulong data_offset;
		ulong total_sample_size;
		uint sample_count;
		uint first_sample_flags;
		ulong* sample_duration;
		uint* sample_size;
		uint* sample_flags;
		uint current_sample;
	};

	xe_vector<track_run> runs;
	uint run_index;

	track_run* alloc_run(){
		size_t size = runs.size();

		if(!runs.grow(size + 1))
			return null;
		runs.resize(size + 1);

		xe_zero(&runs[size]);

		return &runs[size];
	}

	uint id;
	uint default_sample_duration;
	uint default_sample_size;
	uint default_sample_flags;

	uint track_index;

	byte explicit_base_offset;
	byte default_base_is_moof;
};

enum xe_tfhd_flags{
	TFHD_NONE = 0,
	TFHD_EXPLICIT_BASE_OFFSET 		= 0x1,
	TFHD_SAMPLE_DESCRIPTION_INDEX 	= 0x2,
	TFHD_DEFAULT_SAMPLE_DURATION 	= 0x8,
	TFHD_DEFAULT_SAMPLE_SIZE 		= 0x10,
	TFHD_DEFAULT_SAMPLE_FLAGS 		= 0x20,
	TFHD_DURATION_EMPTY 			= 0x10000,
	TFHD_DEFAULT_BASE_IS_MOOF 		= 0x20000
};

enum xe_trun_flags{
	TRUN_NONE = 0,
	TRUN_DATA_OFFSET 		= 0x1,
	TRUN_FIRST_SAMPLE_FLAGS = 0x4,
	TRUN_SAMPLE_DURATION 	= 0x100,
	TRUN_SAMPLE_SIZE 		= 0x200,
	TRUN_SAMPLE_FLAGS 		= 0x400,
	TRUN_SAMPLE_COMPOSITION = 0x800
};

class xe_moof{
public:
	xe_vector<xe_traf*> tracks;
	ulong start;
};

enum xe_esds_tag{
	ES_ROOT = FOURCC_ESDS,
	ES_ODESCR = 0x1,
	ES_IODESCR = 0x2,
	ES_DESCR = 0x3,
	ES_DECODER_CONFIG = 0x4,
	ES_DECODER_SPECIFIC_INFO = 0x5,
	ES_SLDESCR = 0x6
};

xe_cstr esds_type_str(xe_esds_tag tag){
	switch(tag){
		case ES_ROOT:
			return "ES_None";
		case ES_ODESCR:
			return "ES_ODescr";
		case ES_IODESCR:
			return "ES_IODescr";
		case ES_DESCR:
			return "ES_Descr";
		case ES_DECODER_CONFIG:
			return "ES_DecoderConfigDescr";
		case ES_DECODER_SPECIFIC_INFO:
			return "ES_DecoderSpecificInfo";
		case ES_SLDESCR:
			return "ES_SLDescr";
	}

	return "ES_Other";
}

enum xe_esds_flag{
	ESDS_OCR_STREAM = 0x20,
	ESDS_URL = 0x40,
	ESDS_STREAM_DEPENDENCE = 0x80
};

class xe_box{
public:
	ulong size;
	ulong offset;

	union{
		xe_fourcc type;
		xe_esds_tag estag;
	};
};

class xe_isom : public xe_demuxer{
public:
	xe_vector<xe_isom_track*> tracks;
	xe_vector<xe_sidx> segments;
	xe_moof moof;

	ulong mdat_start;
	ulong mdat_end;
	ulong mdat_size;
	uint traf_index;
	uint track_index;

	bool found_moov;
	bool found_moof;
	bool found_mdat;
	bool built_index;

	xe_isom(xe_format::xe_context& context): xe_demuxer(context){}

	int open();
	int seek(uint stream, ulong pos);
	int read_packet(xe_packet& packet);

	void reset(){

	}

	~xe_isom();

	int next_run();
	int moov_next_sample(xe_packet& packet);
	int moov_next_chunk();

	xe_isom_track* alloc_track(){
		xe_isom_track* track = xe_zalloc<xe_isom_track>();

		if(!track)
			return null;
		if(tracks.push_back(track))
			return track;
		xe_dealloc(track);

		return null;
	}

	xe_sidx* alloc_sidx(){
		size_t size = segments.size();

		if(!segments.grow(size + 1))
			return null;
		segments.resize(size + 1);

		return &segments[size];
	}

	xe_traf* alloc_traf(){
		xe_traf* traf = xe_zalloc<xe_traf>();

		if(!traf)
			return null;
		if(moof.tracks.push_back(traf))
			return traf;
		xe_dealloc(traf);

		return null;
	}

	void free_moov(){
		for(auto t : tracks)
			xe_dealloc(t);
		tracks.resize(0);

		for(auto t : segments)
			xe_dealloc(t.entries);
		segments.resize(0);
	}

	void free_moof(){
		for(auto t : moof.tracks){
			for(auto& run : t -> runs){
				if(run.sample_duration)
					xe_dealloc(run.sample_duration);
				else if(run.sample_size)
					xe_dealloc(run.sample_size);
				else if(run.sample_flags)
					xe_dealloc(run.sample_flags);
			}

			t -> runs.free();

			xe_dealloc(t);
		}

		moof.tracks.resize(0);
	}
};

template<class name_type>
class xe_isom_reader_base{
public:
	using name = name_type;

	xe_isom& isom;
	xe_reader& reader;
	uint depth;

	xe_isom_reader_base(xe_isom& isom, xe_reader& reader):
		isom(isom),
		reader(reader){
		depth = 0;
	}

	ulong box_left(xe_box& box, ulong offset){
		if(!box.size)
			return 0;
		return box.offset + box.size - offset;
	}

	bool box_has(xe_box& box, ulong bytes){
		if(!box.size)
			return true;
		return box.offset + box.size >= bytes + reader.offset();
	}

	void stack_push(xe_box& box){
		xe_log_debug(this, "%*s%.*s : %lu @ %lu", depth * 2, "", sizeof(box.type), &box.type, box.size, box.offset);

		depth++;
	}

	void stack_pop(xe_box& box){
		if(box.size)
			xe_log_debug(this, "%*s%li bytes remaining", depth * 2, "", box.offset + box.size - reader.offset());
		else
			xe_log_debug(this, "%*send of document", depth * 2, "");
		depth--;
	}

	void esds_stack_push(xe_box& box){
		xe_log_debug(this, "%*s%s : %lu @ %lu", depth * 2, "", esds_type_str(box.estag), box.size, box.offset);

		depth++;
	}

	void esds_stack_pop(xe_box& box){
		xe_log_debug(this, "%*s%li bytes remaining", depth * 2, "", box.offset + box.size - reader.offset());

		depth--;
	}

	int skip_box(xe_box& box){
		reader.skip(box.offset + box.size - reader.offset());

		return reader.error();
	}

	static xe_cstr class_name(){
		return name::value;
	}
};

struct xe_isom_reader_name_type{
	static constexpr xe_cstr value = "xe_isom_reader";
};

#define BOX(box_type, parent_type, parse)	\
	case box_type: 							\
		if(parent.type == parent_type)		\
			err = parse(box);				\
		break;
class xe_isom_reader : public xe_isom_reader_base<xe_isom_reader_name_type>{
public:
	xe_isom_reader(xe_isom& isom, xe_reader& reader): xe_isom_reader_base(isom, reader){}

	union{
		xe_ptr ptr;
		xe_isom_track* track;
		xe_traf* traf;
	};

	union{
		ulong bits;

		struct{
			bool tkhd: 1;
			bool hdlr: 1;
			bool mdhd: 1;
			bool stsd: 1;

			bool stts: 1;
			bool stsc: 1;
			bool stsz: 1;
			bool stco: 1;
			bool stss: 1;

			bool codec: 1;
		};

		struct{
			bool tfhd: 1;
		};
	} found_boxes;

	int read_root(){
		xe_box root;

		root.type = FOURCC_ROOT;
		root.size = 0;
		root.offset = 0;

		return read_children(root);
	}

	int read_children(xe_box& parent){
		xe_box box;

		int err = 0;

		while(box_has(parent, 8)){
			box.offset = reader.offset();
			box.size = reader.r32be();
			box.type = (xe_fourcc)reader.r32le();

			if(box.size == 1){
				if(!box_has(parent, 8))
					return XE_INVALID_DATA;
				box.size = reader.r64be();

				if(box.size > 0 && box.size < 16)
					return XE_INVALID_DATA;
			}else if(box.size > 0 && box.size < 8){
				return XE_INVALID_DATA;
			}

			if((err = reader.error())){
				if(!depth && err == XE_EOF)
					err = 0;
				break;
			}

			if(depth && !box.size)
				box.size = box_left(parent, box.offset);
			stack_push(box);

			switch(box.type){
				BOX(FOURCC_MOOV, FOURCC_ROOT, read_moov)
				BOX(FOURCC_SIDX, FOURCC_ROOT, read_sidx)
				BOX(FOURCC_MOOF, FOURCC_ROOT, read_moof)

				BOX(FOURCC_TRAK, FOURCC_MOOV, read_trak)
				BOX(FOURCC_MVEX, FOURCC_MOOV, read_children)

				BOX(FOURCC_TKHD, FOURCC_TRAK, read_tkhd)
				BOX(FOURCC_MDIA, FOURCC_TRAK, read_children)

				BOX(FOURCC_HDLR, FOURCC_MDIA, read_hdlr)
				BOX(FOURCC_MDHD, FOURCC_MDIA, read_mdhd)
				BOX(FOURCC_MINF, FOURCC_MDIA, read_children)

				BOX(FOURCC_STBL, FOURCC_MINF, read_children)
				BOX(FOURCC_STSD, FOURCC_STBL, read_stsd)
				BOX(FOURCC_STTS, FOURCC_STBL, read_stts)
				BOX(FOURCC_STSC, FOURCC_STBL, read_stsc)
				BOX(FOURCC_STSZ, FOURCC_STBL, read_stsz)
				BOX(FOURCC_STCO, FOURCC_STBL, read_stco)
				BOX(FOURCC_CO64, FOURCC_STBL, read_co64)
				BOX(FOURCC_STSS, FOURCC_STBL, read_stss)

				BOX(FOURCC_ESDS, ENTRY_MP4A, read_esds)

				BOX(FOURCC_TREX, FOURCC_MVEX, read_trex)

				BOX(FOURCC_TRAF, FOURCC_MOOF, read_traf)
				BOX(FOURCC_TFHD, FOURCC_TRAF, read_tfhd)
				BOX(FOURCC_TFDT, FOURCC_TRAF, read_tfdt)
				BOX(FOURCC_TRUN, FOURCC_TRAF, read_trun)

				BOX(FOURCC_MDAT, FOURCC_ROOT, read_mdat)
			}

			stack_pop(box);

			if(!box_has(box, 0))
				err = XE_INVALID_DATA;
			if(err)
				break;
			if((err = reader.error()))
				break;
			if(box.type == FOURCC_MDAT && !depth && isom.found_moov && isom.found_mdat) // TODO fix
				break;
			if(!box.size)
				break; // TODO abort stream
			if((err = skip_box(box)))
				break;
			if(!depth && isom.found_moov && isom.found_mdat)
				break;
		}

		return err;
	}

	int read_moov(xe_box& box){
		isom.free_moov();
		isom.found_moov = true;

		return read_children(box);
	}

	int read_trex(xe_box& box){
		xe_isom_track* track = null;
		uint id;

		reader.skip(4); /* version + flags */
		id = reader.r32be();
		reader.r32be(); /* default_sample_description_index */

		if(!id)
			return XE_INVALID_DATA;
		for(auto t : isom.tracks){
			if(t -> id == id){
				track = t;

				break;
			}
		}

		if(!track){
			track = isom.alloc_track();

			if(!track)
				return XE_ENOMEM;
			track -> id = id;
		}

		track -> default_sample_duration = reader.r32be();
		track -> default_sample_size = reader.r32be();
		track -> default_sample_flags = reader.r32be();

		return 0;
	}

	int read_trak(xe_box& box){
		int err;

		track = isom.alloc_track();

		if(!track)
			return XE_ENOMEM;
		found_boxes.bits = 0;

		if((err = read_children(box)))
			return err;
		if(!found_boxes.tkhd || !found_boxes.hdlr || !found_boxes.mdhd || !found_boxes.stsd){
			xe_log_error(this, "missing required boxes");

			return XE_INVALID_DATA;
		}

		return 0;
	}

	int read_tkhd(xe_box& box){
		byte version;
		uint flags;
		uint id;

		if(found_boxes.tkhd){
			xe_log_error(this, "multiple tkhd");

			return XE_INVALID_DATA;
		}

		found_boxes.tkhd = true;

		version = reader.r8();
		flags = reader.r24be();
		track -> track_flags = flags;
		reader.skip(version ? 16 : 8); /* creation_time + modification_time */
		id = reader.r32be();
		track -> id = id;

		if(!id)
			return XE_INVALID_DATA;
		for(size_t i = 0; i < isom.tracks.size() - 1; i++){
			if(isom.tracks[i] -> id == id){
				xe_isom_track* trex = isom.tracks[i];

				track -> default_sample_duration = trex -> default_sample_duration;
				track -> default_sample_size = trex -> default_sample_size;
				track -> default_sample_flags = trex -> default_sample_flags;

				isom.tracks[i] = track;
				isom.tracks.pop_back();

				xe_dealloc(trex);

				break;
			}
		}

		reader.skip(4); /* reserved */
		track -> duration = version ? reader.r64be() : reader.r32be();
		reader.skip(10); /* reserved(4) + 2 * layer */
		track -> alternate_group = reader.r16be();

		return 0;
	}

	int read_hdlr(xe_box& box){
		uint handler;

		if(found_boxes.hdlr){
			xe_log_error(this, "duplicate hdlr");

			return XE_INVALID_DATA;
		}

		found_boxes.hdlr = true;

		reader.skip(8); /* version + flags + defined(4) */
		handler = reader.r32le();

		if(handler == FOURCC_VIDE)
			track -> type = XE_TRACK_TYPE_VIDEO;
		else if(handler == FOURCC_SOUN)
			track -> type = XE_TRACK_TYPE_AUDIO;
		else
			track -> type = XE_TRACK_TYPE_OTHER;
		return 0;
	}

	int read_mdhd(xe_box& box){
		byte version;
		uint flags;

		if(found_boxes.mdhd){
			xe_log_error(this, "multiple mdhd");

			return XE_INVALID_DATA;
		}

		found_boxes.mdhd = true;

		version = reader.r8();
		flags = reader.r24be();
		reader.skip(version ? 16 : 8); /* creation + modification time */
		track -> timescale = reader.r32be();
		track -> duration = version ? reader.r64be() : reader.r32be();

		return 0;
	}

	int read_stsd(xe_box& stsd){
		uint entry_count;
		byte version;
		xe_box box;

		int err = 0;

		if(found_boxes.stsd){
			xe_log_error(this, "multiple stsd"); // TODO indent

			return XE_INVALID_DATA;
		}

		found_boxes.stsd = true;

		version = reader.r8();
		reader.skip(3); /* flags */
		entry_count = reader.r32be();

		for(uint i = 0; i < entry_count && box_has(stsd, 8); i++){
			box.offset = reader.offset();
			box.size = reader.r32be();
			box.type = (xe_fourcc)reader.r32le();

			if(box.size < 8)
				return XE_INVALID_DATA;
			stack_push(box);

			if(track -> type == XE_TRACK_TYPE_AUDIO){
				reader.skip(8); /* reserved(6) + data_reference_index(2) */

				if(version){
					reader.skip(8); /* version(2) + reserved(2) * 3 */
					track -> codec.channels = reader.r16be();
					reader.skip(6); /* bit_depth(2) + pre_defined(2) + reserved(2) */
					track -> codec.sample_rate = reader.r32be() / (double)(1 << 16);
				}else{
					reader.skip(8); /* reserved * 2 */
					track -> codec.channels = reader.r16be();
					reader.skip(6); /* bit_depth(2) + pre_defined(2) + reserved(2) */
					track -> codec.sample_rate = reader.r32be() / (double)(1 << 16);
				}

				if(box.type == ENTRY_MP4A || box.type == ENTRY_ENCA)
					err = read_children(box);
				else if(box.type == ENTRY_OPUS)
					track -> codec.id = XE_CODEC_OPUS;
				else if(box.type == ENTRY_FLAC)
					track -> codec.id = XE_CODEC_FLAC;
				if(!box_has(box, 0))
					err = XE_INVALID_DATA;
			}

			stack_pop(box);

			if(err)
				break;
			if((err = reader.error()))
				break;
			if((err = skip_box(box)))
				break;
		}

		return 0;
	}

	int read_es_descr(xe_box& box){
		reader.skip(2); /* ES_ID */

		byte flags = reader.r8();

		if(flags & ESDS_STREAM_DEPENDENCE)
			reader.skip(2);
		if(flags & ESDS_URL)
			reader.skip(reader.r8());
		if(flags & ESDS_OCR_STREAM)
			reader.skip(2);
		return esds_read_children(box);
	}

	int read_decoder_config(xe_box& box){
		byte object_type = reader.r8();

		reader.skip(8); /* stream type(1) + buffersize(3) + maxBitrate(4) */
		track -> codec.bit_rate = reader.r32be();

		switch(object_type){
			case 0x08:
				/* text*/
				break;
			case 0x20:
				/* mpeg4 */
				break;
			case 0x21:
				/* h264 */
				break;
			case 0x23:
				/* hevc */
				break;
			case 0x40:
				track -> codec.id = XE_CODEC_AAC; /* mpeg4 aac */

				break;
			case 0x60:
				/* mp2 simple */
				break;
			case 0x61:
				/* mp2 main */
				break;
			case 0x62:
				/* mp2 snr */
				break;
			case 0x63:
				/* mp2 spatial */
				break;
			case 0x64:
				/* mp2 high */
				break;
			case 0x65:
				/* mp2 422 */
				break;
			case 0x66:
				track -> codec.id = XE_CODEC_AAC; /* mp2 aac main */

				break;
			case 0x67:
				track -> codec.id = XE_CODEC_AAC; /* mp2 aac low-complexity */

				break;
			case 0x68:
				track -> codec.id = XE_CODEC_AAC; /* mp2 aac ssr */

				break;
			case 0x69:
				track -> codec.id = XE_CODEC_MP3;

				return 0;
			case 0x6a:
				/* mp1 video */
				break;
			case 0x6b:
				track -> codec.id = XE_CODEC_MP3;

				return 0;
			case 0x6c:
				/* mjpeg */
				break;
			case 0x6d:
				/* png */
				break;
			case 0x6e:
				/* jpeg2000 */
				break;
			case 0xa3:
				/* vc1 */
				break;
			case 0xa4:
				/* dirac */
				break;
			case 0xa5:
				/* ac3 */
				break;
			case 0xa6:
				/* eac3 */
				break;
			case 0xa9:
				/* dts */
				break;
			case 0xad:
				track -> codec.id = XE_CODEC_OPUS;

				break;
			case 0xb1:
				/* vp9 */
				break;
		}

		if(!track -> codec.id)
			return 0;
		int err;

		if((err = esds_read_children(box)))
			return err;
		if(track -> codec.id == XE_CODEC_AAC)
			err = xe_aac::parse_config(track -> codec);
		return err;
	}

	int read_decoder_info(xe_box& box){
		if(found_boxes.codec)
			return 0;
		found_boxes.codec = true;

		if(!track -> codec.alloc_config(box.size))
			return XE_ENOMEM;
		reader.read(track -> codec.config.data(), box.size);

		return 0;
	}

	int esds_read_children(xe_box& parent){
		#define ESDS_BOX(es_type, parent_type, parse)	\
			case es_type: 								\
				if(parent.estag == parent_type)			\
					err = parse(box);					\
				break;
		int err;

		xe_box box;

		while(box_has(parent, 2)){
			box.estag = (xe_esds_tag)reader.r8();
			box.size = 0;

			for(uint i = 0; i < 4; i++){
				byte size = reader.r8();

				box.size <<= 7;
				box.size |= size & 0x7f;

				if(!(size & 0x80))
					break;
			}

			if(!box_has(parent, 0))
				return XE_INVALID_DATA;
			if((err = reader.error()))
				break;
			box.offset = reader.offset();

			esds_stack_push(box);

			switch(box.estag){
				ESDS_BOX(ES_DESCR, ES_ROOT, read_es_descr)
				ESDS_BOX(ES_DECODER_CONFIG, ES_DESCR, read_decoder_config)
				ESDS_BOX(ES_DECODER_SPECIFIC_INFO, ES_DECODER_CONFIG, read_decoder_info)
			}

			esds_stack_pop(box);

			if(!box_has(box, 0))
				err = XE_INVALID_DATA;
			if(err)
				break;
			if((err = reader.error()))
				break;
			if((err = skip_box(box)))
				break;
		}

		return err;
	}

	int read_esds(xe_box& box){
		reader.skip(4); /* version + flags */

		return esds_read_children(box);
	}

	int read_stts(xe_box& box){
		uint entries;

		if(found_boxes.stts)
			return 0;
		found_boxes.stts = true;

		reader.skip(4); /* version + flags */
		entries = reader.r32be();

		if(!track -> sample_time.resize(entries))
			return XE_ENOMEM;
		for(uint i = 0; i < entries; i++){
			track -> sample_time[i].count = reader.r32be();
			track -> sample_time[i].delta = reader.r32be();

			if(!track -> sample_time[i].delta)
				return XE_INVALID_DATA;
		}

		return 0;
	}

	int read_stsc(xe_box& box){
		uint entries;

		if(found_boxes.stsc)
			return 0;
		found_boxes.stsc = true;

		reader.skip(4); /* version + flags */
		entries = reader.r32be();

		if(!track -> sample_chunk.resize(entries))
			return XE_ENOMEM;
		for(uint i = 0; i < entries; i++){
			track -> sample_chunk[i].first = reader.r32be();
			track -> sample_chunk[i].count = reader.r32be();
			reader.skip(4); /* sample_description_index */
		}

		return 0;
	}

	int read_stsz(xe_box& box){
		uint entries, sample_size;

		if(found_boxes.stsz){
			xe_log_warn(this, "duplicate stsz box, ignoring");

			return 0;
		}

		found_boxes.stsz = true;

		reader.skip(4); /* version + flags */
		sample_size = reader.r32be();
		entries = reader.r32be();
		track -> sample_size = sample_size;

		if(sample_size){
			track -> sample_sizes = xe_array<uint>(null, entries);

			return 0;
		}

		if(!track -> sample_sizes.resize(entries))
			return XE_ENOMEM;
		for(uint i = 0; i < entries; i++)
			track -> sample_sizes[i] = reader.r32be();
		return 0;
	}

	int read_stco(xe_box& box){
		uint entries;

		if(found_boxes.stco)
			return 0;
		found_boxes.stco = true;

		reader.skip(4); /* version + flags */
		entries = reader.r32be();

		if(!track -> chunk_offset.resize(entries))
			return XE_ENOMEM;
		for(uint i = 0; i < entries; i++)
			track -> chunk_offset[i] = reader.r32be();
		return 0;
	}

	int read_co64(xe_box& box){
		uint entries;

		if(found_boxes.stco)
			return 0;
		found_boxes.stco = true;

		reader.skip(4); /* version + flags */
		entries = reader.r32be();

		if(!track -> chunk_offset.resize(entries))
			return XE_ENOMEM;
		for(uint i = 0; i < entries; i++)
			track -> chunk_offset[i] = reader.r64be();
		return 0;
	}

	int read_stss(xe_box& box){
		uint entries;

		if(found_boxes.stss)
			return 0;
		found_boxes.stss = true;

		reader.skip(4); /* version + flags */
		entries = reader.r32be();

		if(!track -> sync_sample.resize(entries))
			return XE_ENOMEM;
		for(uint i = 0; i < entries; i++)
			track -> sync_sample[i] = reader.r32be();
		return 0;
	}

	int read_sidx(xe_box& box){
		if(!box.size)
			return 0;
		byte version;
		uint flags;

		ulong bytec, timec;
		uint length;

		uint type_and_size;
		uint sap;

		uint starts_with_sap;
		uint sap_type;
		uint nested;

		xe_sidx* sidx = isom.alloc_sidx();
		xe_sidx_entry* entries;

		if(!sidx)
			return XE_ENOMEM;
		version = reader.r8();
		flags = reader.r24be();

		sidx -> track_id = reader.r32be();
		sidx -> timescale = reader.r32be();

		if(!sidx -> timescale)
			goto popback;
		bytec = box.offset + box.size;

		if(version){
			bytec += reader.r64be();
			timec = reader.r64be();
		}else{
			bytec += reader.r32be();
			timec = reader.r32be();
		}

		reader.skip(2); /* reserved */
		length = reader.r16be();
		entries = xe_alloc<xe_sidx_entry>(length + 1);

		if(!entries)
			goto popback; // TODO return ENOMEM
		sidx -> length = length;
		sidx -> entries = entries;

		if(!length)
			goto end;
		for(uint i = 0; i < length; i++){
			xe_sidx_entry& entry = sidx -> entries[i];

			entry.time = timec;
			entry.byte = bytec;

			type_and_size = reader.r32be();
			timec += reader.r32be();
			bytec += type_and_size & 0x7fffffff;

			/*

			sap
			1 bit - starts_with_sap
			3 bits - sap_type
			28 bits - delta time

			*/
			sap = reader.r32be();
			nested = (type_and_size & 0x80000000);

			if(nested)
				return XE_INVALID_DATA; /* unsupported */
			starts_with_sap = sap & 0x80000000;
			sap_type = (sap & 0x70000000) >> 28;

			entry.starts_with_keyframe = starts_with_sap != 0;
			entry.contains_keyframe = sap_type != 0 && sap_type != 7;
		}

		entries[length].time = timec;
		entries[length].byte = bytec;
end:
		return 0;
popback:
		xe_dealloc(sidx);

		isom.segments.pop_back();

		goto end;
	}

	int read_moof(xe_box& box){
		if(!box.size)
			return 0;
		isom.free_moof();
		isom.moof.start = box.offset;
		isom.found_moof = true;

		return read_children(box);
	}

	int read_traf(xe_box& box){
		int err;

		traf = isom.alloc_traf();

		if(!traf)
			return XE_ENOMEM;
		found_boxes.bits = 0;

		if((err = read_children(box)))
			return err;
		if(!found_boxes.tfhd){
			xe_log_error(this, "missing tfhd");

			return XE_INVALID_DATA;
		}

		return 0;
	}

	int read_tfhd(xe_box& box){
		uint flags;

		if(found_boxes.tfhd){
			xe_log_warn(this, "duplicate tfhd box");

			return 0;
		}

		found_boxes.tfhd = true;

		reader.skip(1); /* version */
		flags = reader.r24be();
		traf -> id = reader.r32be();

		if(!traf -> id)
			return XE_INVALID_DATA;
		for(size_t i = 0; i < isom.tracks.size(); i++){
			if(isom.tracks[i] -> id == traf -> id){
				traf -> default_sample_duration = isom.tracks[i] -> default_sample_duration;
				traf -> default_sample_size = isom.tracks[i] -> default_sample_size;
				traf -> default_sample_flags = isom.tracks[i] -> default_sample_flags;
				traf -> track_index = i;

				break;
			}
		}

		if(flags & TFHD_EXPLICIT_BASE_OFFSET){
			traf -> offset = reader.r64be();
			traf -> explicit_base_offset = true;
		}

		if(flags & TFHD_SAMPLE_DESCRIPTION_INDEX)
			reader.r32be();
		if(flags & TFHD_DEFAULT_SAMPLE_DURATION)
			traf -> default_sample_duration = reader.r32be();
		if(flags & TFHD_DEFAULT_SAMPLE_SIZE)
			traf -> default_sample_size = reader.r32be();
		if(flags & TFHD_DEFAULT_SAMPLE_FLAGS)
			traf -> default_sample_flags = reader.r32be();
		if(flags & TFHD_DEFAULT_BASE_IS_MOOF && !(flags & TFHD_EXPLICIT_BASE_OFFSET))
			traf -> default_base_is_moof = true;
		return 0;
	}

	int read_tfdt(xe_box& box){
		byte version = reader.r8();

		reader.skip(3); /* flags */
		traf -> start_time = version ? reader.r64be() : reader.r32be();

		return 0;
	}

	int read_trun(xe_box& box){
		uint flags;
		size_t size;
		xe_traf::track_run* run = traf -> alloc_run();

		if(!run)
			return XE_ENOMEM;
		reader.skip(1); /* version */
		flags = reader.r24be();
		run -> sample_count = reader.r32be();
		size = 0;

		if(flags & TRUN_DATA_OFFSET)
			run -> data_offset = reader.r32be();
		if(flags & TRUN_FIRST_SAMPLE_FLAGS)
			run -> first_sample_flags = reader.r32be();
		if(flags & TRUN_SAMPLE_DURATION)
			size += sizeof(ulong);
		if(flags & TRUN_SAMPLE_SIZE)
			size += sizeof(uint);
		if(flags & TRUN_SAMPLE_FLAGS)
			size += sizeof(uint);
		uint* data = xe_alloc<uint>(size * run -> sample_count);

		if(!data)
			return XE_ENOMEM;
		if(flags & TRUN_SAMPLE_DURATION){
			run -> sample_duration = (ulong*)data;
			data += run -> sample_count * 2l;
		}

		if(flags & TRUN_SAMPLE_SIZE){
			run -> sample_size = data;
			data += run -> sample_count;
		}

		if(flags & TRUN_SAMPLE_FLAGS)
			run -> sample_flags = data;
		ulong total_duration = 0, total_size = 0;

		for(uint i = 0; i < run -> sample_count; i++){
			if(flags & TRUN_SAMPLE_DURATION){
				uint duration = reader.r32be();

				total_duration += duration;
				run -> sample_duration[i] = total_duration;

				if(!duration)
					return XE_INVALID_DATA;
			}

			if(flags & TRUN_SAMPLE_SIZE){
				uint size = reader.r32be();

				run -> sample_size[i] = size;
				total_size += size;
			}

			if(flags & TRUN_SAMPLE_FLAGS)
				run -> sample_flags[i] = reader.r32be();
			if(flags & TRUN_SAMPLE_COMPOSITION)
				reader.r32be();
		}

		run -> total_sample_size = total_size;

		return 0;
	}

	int read_mdat(xe_box& box){
		isom.mdat_start = reader.offset();
		isom.mdat_size = box.size;
		isom.mdat_end = box.offset + box.size;
		isom.found_mdat = true;

		return 0;
	}
};

struct xe_isom_scan_reader_name_type{
	static constexpr xe_cstr value = "xe_isom_scan_reader";
};

class xe_isom_scan_reader : xe_isom_reader_base<xe_isom_scan_reader_name_type>{
	xe_isom_scan_reader(xe_isom& isom, xe_reader& reader): xe_isom_reader_base(isom, reader){}

	union{
		xe_ptr ptr;
		xe_traf* traf;
	};

	union{
		ulong bits;

		struct{
			ulong tkhd: 1;
		};
	} found_boxes;

	int read_root(){
		xe_box root;

		root.type = FOURCC_ROOT;
		root.size = 0;
		root.offset = 0;

		return read_children(root);
	}

	int read_children(xe_box& parent){
		xe_box box;

		int err = 0;

		while(box_has(parent, 8)){
			box.offset = reader.offset();
			box.size = reader.r32be();
			box.type = (xe_fourcc)reader.r32le();

			if(box.size == 1){
				if(!box_has(parent, 8))
					return XE_INVALID_DATA;
				box.size = reader.r64be();

				if(box.size > 0 && box.size < 16)
					return XE_INVALID_DATA;
			}else if(box.size > 0 && box.size < 8){
				return XE_INVALID_DATA;
			}

			if((err = reader.error())){
				if(!depth && err == XE_EOF)
					err = 0;
				break;
			}

			if(depth && !box.size)
				box.size = box_left(parent, box.offset);
			stack_push(box);

			switch(box.type){
				BOX(FOURCC_SIDX, FOURCC_ROOT, read_sidx)
				BOX(FOURCC_MOOF, FOURCC_ROOT, read_moof)

				BOX(FOURCC_TRAF, FOURCC_MOOF, read_traf)
				BOX(FOURCC_TFDT, FOURCC_TRAF, read_tfdt)
				BOX(FOURCC_TRUN, FOURCC_TRAF, read_trun)
			}

			stack_pop(box);

			if(!box_has(box, 0))
				err = XE_INVALID_DATA;
			if(err)
				break;
			if((err = reader.error()))
				break;
			if(box.type == FOURCC_MDAT && !depth && isom.found_moov && isom.found_mdat) // TODO fix
				break;
			if(!box.size)
				break; // TODO abort stream
			if((err = skip_box(box)))
				break;
			if(!depth && isom.found_moov && isom.found_mdat)
				break;
		}

		return err;
	}

	int read_sidx(xe_box& box){
		// TODO
		return 0;
	}

	int read_moof(xe_box& box){
		if(!box.size)
			return 0;
		isom.free_moof();
		isom.moof.start = box.offset;
		isom.found_moof = true;

		return read_children(box);
	}

	int read_traf(xe_box& box){
		traf = isom.alloc_traf();

		if(!traf)
			return XE_ENOMEM;
		return read_children(box);
	}

	int read_tfdt(xe_box& box){
		byte version = reader.r8();

		reader.skip(3); /* flags */
		traf -> start_time = version ? reader.r64be() : reader.r32be();

		return 0;
	}

	int read_trun(xe_box& box){
		return 0;
	}
};

int xe_isom::open(){
	xe_isom_reader reader(*this, context -> reader);

	int err;

	if((err = reader.read_root()))
		return err;
	if(!context -> tracks.resize(tracks.size()))
		return XE_ENOMEM;
	for(uint i = 0; i < tracks.size(); i++)
		context -> tracks[i] = tracks[i];
	return 0;
}

int xe_isom::seek(uint stream, ulong pos){
	return 0;
}

int xe_isom::next_run(){
	xe_reader& reader = context -> reader;
	ulong min_offset = ULONG_MAX;

	while(!reader.error()){
		if(found_moof && found_mdat){
			ulong last_offset;

			found_moof = false;
			found_mdat = false;
			last_offset = moof.start;

			for(auto traf : moof.tracks){
				if(!traf -> explicit_base_offset){
					if(traf -> default_base_is_moof)
						traf -> offset = moof.start;
					else
						traf -> offset = last_offset;
				}

				for(auto& run : traf -> runs){
					int offset = run.data_offset & 0xffffffff;

					if(offset < 0 && -offset > traf -> offset)
						return XE_INVALID_DATA;
					run.data_offset = traf -> offset + offset;

					if(run.data_offset < mdat_start)
						return XE_INVALID_DATA;
					if(!run.sample_size && !run.total_sample_size)
						run.total_sample_size = (ulong)run.sample_count * traf -> default_sample_size;
					last_offset = run.data_offset + run.total_sample_size;

					if(mdat_size != 0 && last_offset > mdat_end)
						return XE_INVALID_DATA;
				}
			}
		}

		bool found = false;

		for(uint i = 0; i < moof.tracks.size(); i++){
			auto traf = moof.tracks[i];

			if(traf -> run_index >= traf -> runs.size())
				continue;
			auto& run = traf -> runs[traf -> run_index];

			if(run.data_offset <= min_offset){
				min_offset = run.data_offset;
				traf_index = i;
				found = true;
			}
		}

		if(found){
			if(min_offset < reader.offset())
				return XE_INVALID_DATA;
			reader.skip(min_offset - reader.offset());
		}else{
			reader.skip(mdat_end - reader.offset());

			xe_isom_reader ireader(*this, reader);

			ireader.read_root();

			continue;
		}

		return reader.error();
	}

	return reader.error();
}

static int build_index(xe_vector<xe_isom_track*>& tracks){
	for(auto track : tracks){
		track -> index.sample_to_chunk = xe_alloc<uint>(track -> sample_chunk.size());
		track -> index.sample_to_chunk[0] = 0;

		for(uint i = 1; i < track -> sample_chunk.size(); i++)
			track -> index.sample_to_chunk[i] = track -> index.sample_to_chunk[i - 1] + (track -> sample_chunk[i].first - track -> sample_chunk[i - 1].first) * track -> sample_chunk[i - 1].count;
		track -> index.sample_to_time = xe_alloc<ulong>(track -> sample_time.size());

		ulong time = 0;

		for(uint i = 0; i < track -> sample_time.size(); i++){
			time += track -> sample_time[i].count * track -> sample_time[i].delta;
			track -> index.sample_to_time[i] = time;
		}

		track -> index.time_to_sample = xe_alloc<uint>(track -> sample_time.size());
		track -> index.time_to_sample[0] = 0;

		for(uint i = 1; i < track -> sample_time.size(); i++)
			track -> index.time_to_sample[i] = track -> index.time_to_sample[i - 1] + track -> sample_time[i - 1].count;
	}

	return 0;
}

int xe_isom::moov_next_chunk(){
	xe_reader& reader = context -> reader;

	bool found = false;

	ulong min_offset = ULONG_MAX;

	for(uint i = 0; i < tracks.size(); i++){
		auto track = tracks[i];

		if(track -> current_chunk >= track -> chunk_offset.size())
			continue;
		ulong offset = track -> chunk_offset[track -> current_chunk];

		if(offset <= min_offset){
			min_offset = offset;
			track_index = i;
			found = true;
		}
	}

	if(!found)
		return XE_EOF;
	if(min_offset < reader.offset()){
		reader.seek(min_offset);

		return 0;
	}

	reader.skip(min_offset - reader.offset());

	return reader.error();
}

static long bsearch(uint* array, uint size, uint value){
	uint low = 0;
	uint high = size - 1;

	while(low < high){
		uint mid = (low + high + 1) >> 1;

		if(array[mid] > value)
			high = mid - 1;
		else if(array[mid] > value)
			low = mid;
	}

	return low;
}

int xe_isom::moov_next_sample(xe_packet& packet){
	xe_reader& reader = context -> reader;

	int err;

	if(!built_index){
		built_index = true;

		build_index(tracks);

		if((err = moov_next_chunk()))
			return err;
	}

	auto track = tracks[track_index];

	if(track -> current_chunk >= track -> chunk_offset.size())
		return XE_EOF;
	uint sample_chunk_index = track -> sample_chunk_index;
	uint samples_per_chunk = track -> sample_chunk[sample_chunk_index].count;
	uint sample_start = (track -> current_chunk - track -> sample_chunk[sample_chunk_index].first + 1) * samples_per_chunk + track -> index.sample_to_chunk[sample_chunk_index];
	uint sample_end = sample_start + samples_per_chunk;

	if(track -> current_sample >= sample_end){
		track -> current_chunk++;

		if(sample_chunk_index + 1 < track -> sample_chunk.size() && track -> current_chunk + 1 >= track -> sample_chunk[sample_chunk_index + 1].first)
			track -> sample_chunk_index++;
		if((err = moov_next_chunk()))
			return err;
		track = tracks[track_index];

		sample_chunk_index = track -> sample_chunk_index;
		samples_per_chunk = track -> sample_chunk[sample_chunk_index].count;
		sample_start = (track -> current_chunk - track -> sample_chunk[sample_chunk_index].first) * samples_per_chunk + track -> index.sample_to_chunk[sample_chunk_index];
		sample_end = sample_start + samples_per_chunk;
	}

	uint size = track -> sample_size ? track -> sample_size : track -> sample_sizes[track -> current_sample];
	uint tts_index = bsearch(track -> index.time_to_sample, track -> sample_time.size(), track -> current_sample);

	packet.timestamp = (track -> current_sample - track -> index.time_to_sample[tts_index]) * track -> sample_time[tts_index].delta + (tts_index > 0 ? track -> index.sample_to_time[tts_index - 1] : 0);
	packet.duration = track -> sample_time[tts_index].delta;
	track -> current_sample++;

	if(!alloc_packet(packet, size))
		return XE_ENOMEM;
	reader.read(packet.data(), size);

	return reader.error();
}

int xe_isom::read_packet(xe_packet& packet){
	xe_reader& reader = context -> reader; // TODO ensure runs dont overlap

	int err;

	if(reader.error())
		return reader.error();
	if((err = moov_next_sample(packet)) != XE_EOF)
		return err;
	if(found_moof && found_mdat){
		if((err = next_run()))
			return err;
	}

	if(!moof.tracks.size())
		return XE_EOF;
	auto traf = moof.tracks[traf_index];
	auto run = &traf -> runs[traf -> run_index];

	if(run -> current_sample >= run -> sample_count){
		traf -> run_index++;

		if((err = next_run()))
			return err;
		traf = moof.tracks[traf_index];
		run = &traf -> runs[traf -> run_index];
	}

	uint size = run -> sample_size ? run -> sample_size[run -> current_sample] : traf -> default_sample_size;
	ulong timestamp = traf -> start_time;

	if(run -> sample_duration)
		packet.duration = run -> sample_duration[run -> current_sample] - (run -> current_sample ? run -> sample_duration[run -> current_sample - 1] : 0);
	else
		packet.duration = traf -> default_sample_duration;
	if(run -> current_sample > 0)
		timestamp += run -> sample_duration ? run -> sample_duration[run -> current_sample - 1] : traf -> default_sample_duration * (run -> current_sample);
	run -> current_sample++;

	if(!alloc_packet(packet, size))
		return XE_ENOMEM;
	packet.timestamp = timestamp;
	reader.read(packet.data(), size);

	return reader.error();
}

xe_isom::~xe_isom(){
	free_moov();
	free_moof();
}

xe_demuxer* xe_isom_class::create(xe_format::xe_context& context) const{
	return xe_znew<xe_isom>(context);
}

bool xe_isom_class::probe(xe_reader& reader) const{
	xe_fourcc box;

	reader.r32be();
	box = (xe_fourcc)reader.r32le();

	switch(box){
		case FOURCC_FTYP:
		case FOURCC_MOOV:
		case FOURCC_MOOF:
		case FOURCC_MDAT:
		case FOURCC_SIDX:
			return true;
	}

	return false;
}