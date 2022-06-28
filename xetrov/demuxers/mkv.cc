#include "../format.h"
#include "../demuxer.h"
#include "../error.h"
#include "../common.h"
#include "mkv.h"
#include "xe/log.h"
#include "xe/container/vector.h"
#include "xe/arch.h"
#include "xe/string.h"

using namespace xetrov;

static constexpr int xe_make_matroska_id(ulong id){
	return id ^ 1 << xe_log2(id);
}

enum xe_matroska_id : ulong{
	EBML_ROOT = 1,

	EBML_HEADER = xe_make_matroska_id(0x1a45dfa3),
	EBML_VERSION = xe_make_matroska_id(0x4286),
	EBML_READER_VERSION = xe_make_matroska_id(0x42f7),
	EBML_MAX_ID_LENGTH = xe_make_matroska_id(0x42f2),
	EBML_MAX_SIZE_LENGTH = xe_make_matroska_id(0x42f3),
	EBML_DOCTYPE = xe_make_matroska_id(0x4282),
	EBML_DOCTYPE_VERSION = xe_make_matroska_id(0x4287),
	EBML_DOCTYPE_READER_VERSION = xe_make_matroska_id(0x4285),
	EBML_DOCTYPE_EXTENSION = xe_make_matroska_id(0x4281),
	EBML_DOCTYPE_EXTENSION_NAME = xe_make_matroska_id(0x4283),
	EBML_DOCTYPE_EXTENSION_VERSION = xe_make_matroska_id(0x4284),

	CRC32 = xe_make_matroska_id(0xbf),
	VOID = xe_make_matroska_id(0xec),

	MKV_SEGMENT = xe_make_matroska_id(0x18538067),

	MKV_SEGMENT_INFO = xe_make_matroska_id(0x1549a966),
	MKV_SEGMENT_TIMECODE_SCALE = xe_make_matroska_id(0x2ad7b1),
	MKV_SEGMENT_DURATION = xe_make_matroska_id(0x4489),
	MKV_SEGMENT_UID = xe_make_matroska_id(0x73a4),
	MKV_SEGMENT_FILENAME = xe_make_matroska_id(0x7384),
	MKV_SEGMENT_PREV_UID = xe_make_matroska_id(0x3cb923),
	MKV_SEGMENT_PREV_FILENAME = xe_make_matroska_id(0x3c83ab),
	MKV_SEGMENT_NEXT_UID = xe_make_matroska_id(0x3eb923),
	MKV_SEGMENT_NEXT_FILENAME = xe_make_matroska_id(0x3e83bb),
	MKV_SEGMENT_FAMILY = xe_make_matroska_id(0x4444),
	MKV_SEGMENT_CHAPTER_TRANSLATE = xe_make_matroska_id(0x6924),
	MKV_SEGMENT_CHAPTER_TRANSLATE_ID = xe_make_matroska_id(0x69a5),
	MKV_SEGMENT_CHAPTER_TRANSLATE_CODEC = xe_make_matroska_id(0x69bf),
	MKV_SEGMENT_CHAPTER_TRANSLATE_EDITION_UID = xe_make_matroska_id(0x69fc),
	MKV_SEGMENT_DATE_UTC = xe_make_matroska_id(0x4461),
	MKV_SEGMENT_TITLE = xe_make_matroska_id(0x7ba9),
	MKV_SEGMENT_MUXING_APP = xe_make_matroska_id(0x4d80),
	MKV_SEGMENT_WRITING_APP = xe_make_matroska_id(0x5741),

	MKV_SEEK_HEAD = xe_make_matroska_id(0x114d9b74),
	MKV_SEEK = xe_make_matroska_id(0x4dbb),
	MKV_SEEK_ID = xe_make_matroska_id(0x53ab),
	MKV_SEEK_POSITION = xe_make_matroska_id(0x53ac),

	MKV_TRACKS = xe_make_matroska_id(0x1654ae6b),
	MKV_TRACK = xe_make_matroska_id(0xae),
	MKV_TRACK_NUMBER = xe_make_matroska_id(0xd7),
	MKV_TRACK_UID = xe_make_matroska_id(0x73c5),
	MKV_TRACK_TYPE = xe_make_matroska_id(0x83),
	MKV_TRACK_FLAG_ENABLED = xe_make_matroska_id(0xb9),
	MKV_TRACK_FLAG_DEFAULT = xe_make_matroska_id(0x88),
	MKV_TRACK_DEFAULT_DURATION = xe_make_matroska_id(0x23e383),
	MKV_TRACK_TIMECODE_SCALE = xe_make_matroska_id(0x23314f),
	MKV_TRACK_CODEC_ID = xe_make_matroska_id(0x86),
	MKV_TRACK_CODEC_PRIVATE = xe_make_matroska_id(0x63A2),
	MKV_TRACK_CODEC_NAME = xe_make_matroska_id(0x258688),
	MKV_TRACK_FLAG_LACING = xe_make_matroska_id(0x9c),
	MKV_TRACK_LANGUAGE = xe_make_matroska_id(0x22b59c),
	MKV_TRACK_CODEC_DELAY = xe_make_matroska_id(0x56aa),
	MKV_TRACK_SEEK_PREROLL = xe_make_matroska_id(0x56bb),
	MKV_TRACK_FLAG_FORCED = xe_make_matroska_id(0x55aa),
	MKV_TRACK_FLAG_HEARING_IMPAIRED = xe_make_matroska_id(0x55ab),
	MKV_TRACK_FLAG_VISUAL_IMPAIRED = xe_make_matroska_id(0x55ac),
	MKV_TRACK_FLAG_TEXT_DESCRIPTIONS = xe_make_matroska_id(0x55ad),
	MKV_TRACK_FLAG_ORIGINAL = xe_make_matroska_id(0x55ae),
	MKV_TRACK_FLAG_COMMENTARY = xe_make_matroska_id(0x55af),
	MKV_TRACK_MIN_CACHE = xe_make_matroska_id(0x6de7),
	MKV_TRACK_MAX_CACHE = xe_make_matroska_id(0x6df8),
	MKV_TRACK_DEFAULT_DECODED_FIELD_DURATION = xe_make_matroska_id(0x234e7a),
	MKV_TRACK_OFFSET = xe_make_matroska_id(0x537f),
	MKV_TRACK_MAX_BLOCK_ADDITION_ID = xe_make_matroska_id(0x55ee),
	MKV_TRACK_BLOCK_ADDITION_MAPPING = xe_make_matroska_id(0x41e4),
	MKV_TRACK_BLOCK_ADD_ID_VALUE = xe_make_matroska_id(0x41f0),
	MKV_TRACK_BLOCK_ADD_ID_NAME = xe_make_matroska_id(0x41a4),
	MKV_TRACK_BLOCK_ADD_ID_TYPE = xe_make_matroska_id(0x41e7),
	MKV_TRACK_BLOCK_ADD_ID_EXTRA_DATA = xe_make_matroska_id(0x41ed),
	MKV_TRACK_NAME = xe_make_matroska_id(0x536e),
	MKV_TRACK_LANGUAGE_IETF = xe_make_matroska_id(0x22b59d),
	MKV_TRACK_ATTACHMENT_LINK = xe_make_matroska_id(0x7446),
	MKV_TRACK_CODEC_SETTINGS = xe_make_matroska_id(0x3a9697),
	MKV_TRACK_CODEC_INFO_URL = xe_make_matroska_id(0x3b4040),
	MKV_TRACK_CODEC_DOWNLOAD_URL = xe_make_matroska_id(0x26b240),
	MKV_TRACK_CODEC_DECODE_ALL = xe_make_matroska_id(0xaa),
	MKV_TRACK_OVERLAY = xe_make_matroska_id(0x6fab),
	MKV_TRACK_TRANSLATE = xe_make_matroska_id(0x6624),
	MKV_TRACK_TRANSLATE_TRACK_ID = xe_make_matroska_id(0x66a5),
	MKV_TRACK_TRANSLATE_CODEC = xe_make_matroska_id(0x66bf),
	MKV_TRACK_TRANSLATE_EDITION_UID = xe_make_matroska_id(0x66fc),
	MKV_TRACK_VIDEO = xe_make_matroska_id(0xe0),
	MKV_TRACK_FLAG_INTERLACED = xe_make_matroska_id(0x9a),
	MKV_TRACK_FIELD_ORDER = xe_make_matroska_id(0x9d),
	MKV_TRACK_STEREO_MODE = xe_make_matroska_id(0x53b8),
	MKV_TRACK_ALPHA_MODE = xe_make_matroska_id(0x53c0),
	MKV_TRACK_OLD_STEREO_MODE = xe_make_matroska_id(0x53b9),
	MKV_TRACK_PIXEL_WIDTH = xe_make_matroska_id(0xb0),
	MKV_TRACK_PIXEL_HEIGHT = xe_make_matroska_id(0xba),
	MKV_TRACK_PIXEL_CROP_BOTTOM = xe_make_matroska_id(0x54aa),
	MKV_TRACK_PIXEL_CROP_TOP = xe_make_matroska_id(0x54bb),
	MKV_TRACK_PIXEL_CROP_LEFT = xe_make_matroska_id(0x54cc),
	MKV_TRACK_PIXEL_CROP_RIGHT = xe_make_matroska_id(0x54dd),
	MKV_TRACK_DISPLAY_WIDTH = xe_make_matroska_id(0x54b0),
	MKV_TRACK_DISPLAY_HEIGHT = xe_make_matroska_id(0x54ba),
	MKV_TRACK_DISPLAY_UNIT = xe_make_matroska_id(0x54b2),
	MKV_TRACK_ASPECT_RATIO_TYPE = xe_make_matroska_id(0x54b3),
	MKV_TRACK_UNCOMPRESSED_FOUR_CC = xe_make_matroska_id(0x2eb524),
	MKV_TRACK_GAMMA_VALUE = xe_make_matroska_id(0x2fb523),
	MKV_TRACK_FRAME_RATE = xe_make_matroska_id(0x2383e3),
	MKV_TRACK_COLOUR = xe_make_matroska_id(0x55b0),
	MKV_TRACK_MATRIX_COEFFICIENTS = xe_make_matroska_id(0x55b1),
	MKV_TRACK_BITS_PER_CHANNEL = xe_make_matroska_id(0x55b2),
	MKV_TRACK_CHROMA_SUBSAMPLING_HORZ = xe_make_matroska_id(0x55b3),
	MKV_TRACK_CHROMA_SUBSAMPLING_VERT = xe_make_matroska_id(0x55b4),
	MKV_TRACK_CB_SUBSAMPLING_HORZ = xe_make_matroska_id(0x55b5),
	MKV_TRACK_CB_SUBSAMPLING_VERT = xe_make_matroska_id(0x55b6),
	MKV_TRACK_CHROMA_SITING_HORZ = xe_make_matroska_id(0x55b7),
	MKV_TRACK_CHROMA_SITING_VERT = xe_make_matroska_id(0x55b8),
	MKV_TRACK_RANGE = xe_make_matroska_id(0x55b9),
	MKV_TRACK_TRANSFER_CHARACTERISTICS = xe_make_matroska_id(0x55ba),
	MKV_TRACK_PRIMARIES = xe_make_matroska_id(0x55bb),
	MKV_TRACK_MAX_CLL = xe_make_matroska_id(0x55bc),
	MKV_TRACK_MAX_FALL = xe_make_matroska_id(0x55bd),
	MKV_TRACK_MASTERING_METADATA = xe_make_matroska_id(0x55d0),
	MKV_TRACK_PRIMARY_RCHROMATICITY_X = xe_make_matroska_id(0x55d1),
	MKV_TRACK_PRIMARY_RCHROMATICITY_Y = xe_make_matroska_id(0x55d2),
	MKV_TRACK_PRIMARY_GCHROMATICITY_X = xe_make_matroska_id(0x55d3),
	MKV_TRACK_PRIMARY_GCHROMATICITY_Y = xe_make_matroska_id(0x55d4),
	MKV_TRACK_PRIMARY_BCHROMATICITY_X = xe_make_matroska_id(0x55d5),
	MKV_TRACK_PRIMARY_BCHROMATICITY_Y = xe_make_matroska_id(0x55d6),
	MKV_TRACK_WHITE_POINT_CHROMATICITY_X = xe_make_matroska_id(0x55d7),
	MKV_TRACK_WHITE_POINT_CHROMATICITY_Y = xe_make_matroska_id(0x55d8),
	MKV_TRACK_LUMINANCE_MAX = xe_make_matroska_id(0x55d9),
	MKV_TRACK_LUMINANCE_MIN = xe_make_matroska_id(0x55da),
	MKV_TRACK_PROJECTION = xe_make_matroska_id(0x7670),
	MKV_TRACK_PROJECTION_TYPE = xe_make_matroska_id(0x7671),
	MKV_TRACK_PROJECTION_PRIVATE = xe_make_matroska_id(0x7672),
	MKV_TRACK_PROJECTION_POSE_YAW = xe_make_matroska_id(0x7673),
	MKV_TRACK_PROJECTION_POSE_PITCH = xe_make_matroska_id(0x7674),
	MKV_TRACK_PROJECTION_POSE_ROLL = xe_make_matroska_id(0x7675),
	MKV_TRACK_AUDIO = xe_make_matroska_id(0xe1),
	MKV_TRACK_AUDIO_SAMPLING_FREQUENCY = xe_make_matroska_id(0xb5),
	MKV_TRACK_AUDIO_OUTPUT_SAMPLING_FREQUENCY = xe_make_matroska_id(0x78b5),
	MKV_TRACK_AUDIO_CHANNELS = xe_make_matroska_id(0x9f),
	MKV_TRACK_AUDIO_CHANNEL_POSITIONS = xe_make_matroska_id(0x7d7b),
	MKV_TRACK_AUDIO_BIT_DEPTH = xe_make_matroska_id(0x6264),
	MKV_TRACK_OPERATION = xe_make_matroska_id(0xe2),
	MKV_TRACK_COMBINE_PLANES = xe_make_matroska_id(0xe3),
	MKV_TRACK_PLANE = xe_make_matroska_id(0xe4),
	MKV_TRACK_PLANE_UID = xe_make_matroska_id(0xe5),
	MKV_TRACK_PLANE_TYPE = xe_make_matroska_id(0xe6),
	MKV_TRACK_JOIN_BLOCKS = xe_make_matroska_id(0xe9),
	MKV_TRACK_JOIN_UID = xe_make_matroska_id(0xed),
	MKV_TRACK_TRICK_TRACK_UID = xe_make_matroska_id(0xc0),
	MKV_TRACK_TRICK_TRACK_SEGMENT_UID = xe_make_matroska_id(0xc1),
	MKV_TRACK_TRICK_TRACK_FLAG = xe_make_matroska_id(0xc6),
	MKV_TRACK_TRICK_MASTER_TRACK_UID = xe_make_matroska_id(0xc7),
	MKV_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID = xe_make_matroska_id(0xc4),
	MKV_TRACK_CONTENT_ENCODINGS = xe_make_matroska_id(0x6d80),
	MKV_TRACK_CONTENT_ENCODING = xe_make_matroska_id(0x6240),
	MKV_TRACK_CONTENT_ENCODING_ORDER = xe_make_matroska_id(0x5031),
	MKV_TRACK_CONTENT_ENCODING_SCOPE = xe_make_matroska_id(0x5032),
	MKV_TRACK_CONTENT_ENCODING_TYPE = xe_make_matroska_id(0x5033),
	MKV_TRACK_CONTENT_COMPRESSION = xe_make_matroska_id(0x5034),
	MKV_TRACK_CONTENT_COMP_ALGO = xe_make_matroska_id(0x4254),
	MKV_TRACK_CONTENT_COMP_SETTINGS = xe_make_matroska_id(0x4255),
	MKV_TRACK_CONTENT_ENCRYPTION = xe_make_matroska_id(0x5035),
	MKV_TRACK_CONTENT_ENC_ALGO = xe_make_matroska_id(0x47e1),
	MKV_TRACK_CONTENT_ENC_KEY_ID = xe_make_matroska_id(0x47e2),
	MKV_TRACK_CONTENT_ENC_AESSETTINGS = xe_make_matroska_id(0x47e7),
	MKV_TRACK_AESSETTINGS_CIPHER_MODE = xe_make_matroska_id(0x47e8),
	MKV_TRACK_CONTENT_SIGNATURE = xe_make_matroska_id(0x47e3),
	MKV_TRACK_CONTENT_SIG_KEY_ID = xe_make_matroska_id(0x47e4),
	MKV_TRACK_CONTENT_SIG_ALGO = xe_make_matroska_id(0x47e5),
	MKV_TRACK_CONTENT_SIG_HASH_ALGO = xe_make_matroska_id(0x47e6),

	MKV_CUES = xe_make_matroska_id(0x1c53bb6b),
	MKV_CUE_POINT = xe_make_matroska_id(0xbb),
	MKV_CUE_TIME = xe_make_matroska_id(0xb3),
	MKV_CUE_TRACK_POSITION = xe_make_matroska_id(0xb7),
	MKV_CUE_TRACK = xe_make_matroska_id(0xf7),
	MKV_CUE_CLUSTER_POSITION = xe_make_matroska_id(0xf1),
	MKV_CUE_BLOCK_NUMBER = xe_make_matroska_id(0x5378),
	MKV_CUE_RELATIVE_POSITION = xe_make_matroska_id(0xf0),
	MKV_CUE_DURATION = xe_make_matroska_id(0xb2),
	MKV_CUE_CODEC_STATE = xe_make_matroska_id(0xea),
	MKV_CUE_REFERENCE = xe_make_matroska_id(0xdb),
	MKV_CUE_REF_TIME = xe_make_matroska_id(0x96),
	MKV_CUE_REF_CLUSTER = xe_make_matroska_id(0x97),
	MKV_CUE_REF_NUMBER = xe_make_matroska_id(0x535f),
	MKV_CUE_REF_CODEC_STATE = xe_make_matroska_id(0xeb),

	MKV_CLUSTER = xe_make_matroska_id(0x1f43b675),
	MKV_CLUSTER_TIMECODE = xe_make_matroska_id(0xe7),
	MKV_CLUSTER_POSITION = xe_make_matroska_id(0xa7),
	MKV_CLUSTER_SIMPLE_BLOCK = xe_make_matroska_id(0xa3),
	MKV_CLUSTER_BLOCK_GROUP = xe_make_matroska_id(0xa0),
	MKV_CLUSTER_BLOCK_GROUP_BLOCK = xe_make_matroska_id(0xa1),
	MKV_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK = xe_make_matroska_id(0xfb),
	MKV_CLUSTER_BLOCK_GROUP_BLOCK_DURATION = xe_make_matroska_id(0x9b),
	MKV_CLUSTER_SILENT_TRACKS = xe_make_matroska_id(0x5854),
	MKV_CLUSTER_SILENT_TRACK_NUMBER = xe_make_matroska_id(0x58d7),
	MKV_CLUSTER_PREV_SIZE = xe_make_matroska_id(0xab),
	MKV_CLUSTER_BLOCK_VIRTUAL = xe_make_matroska_id(0xa2),
	MKV_CLUSTER_BLOCK_ADDITIONS = xe_make_matroska_id(0x75a1),
	MKV_CLUSTER_BLOCK_MORE = xe_make_matroska_id(0xa6),
	MKV_CLUSTER_BLOCK_ADD_ID = xe_make_matroska_id(0xee),
	MKV_CLUSTER_BLOCK_ADDITIONAL = xe_make_matroska_id(0xa5),
	MKV_CLUSTER_REFERENCE_PRIORITY = xe_make_matroska_id(0xfa),
	MKV_CLUSTER_REFERENCE_VIRTUAL = xe_make_matroska_id(0xfd),
	MKV_CLUSTER_CODEC_STATE = xe_make_matroska_id(0xa4),
	MKV_CLUSTER_DISCARD_PADDING = xe_make_matroska_id(0x75a2),
	MKV_CLUSTER_SLICES = xe_make_matroska_id(0x8e),
	MKV_CLUSTER_TIME_SLICE = xe_make_matroska_id(0xe8),
	MKV_CLUSTER_LACE_NUMBER = xe_make_matroska_id(0xcc),
	MKV_CLUSTER_FRAME_NUMBER = xe_make_matroska_id(0xcd),
	MKV_CLUSTER_BLOCK_ADDITION_ID = xe_make_matroska_id(0xcb),
	MKV_CLUSTER_DELAY = xe_make_matroska_id(0xce),
	MKV_CLUSTER_SLICE_DURATION = xe_make_matroska_id(0xcf),
	MKV_CLUSTER_REFERENCE_FRAME = xe_make_matroska_id(0xc8),
	MKV_CLUSTER_REFERENCE_OFFSET = xe_make_matroska_id(0xc9),
	MKV_CLUSTER_REFERENCE_TIMESTAMP = xe_make_matroska_id(0xca),
	MKV_CLUSTER_ENCRYPTED_BLOCK = xe_make_matroska_id(0xaf),

	MKV_ATTACHMENTS = xe_make_matroska_id(0x1941a469),
	MKV_ATTACHED_FILE = xe_make_matroska_id(0x61a7),
	MKV_FILE_DESCRIPTION = xe_make_matroska_id(0x467e),
	MKV_FILE_NAME = xe_make_matroska_id(0x466e),
	MKV_FILE_MIME_TYPE = xe_make_matroska_id(0x4660),
	MKV_FILE_DATA = xe_make_matroska_id(0x465c),
	MKV_FILE_UID = xe_make_matroska_id(0x46ae),
	MKV_FILE_REFERRAL = xe_make_matroska_id(0x4675),
	MKV_FILE_USED_START_TIME = xe_make_matroska_id(0x4661),
	MKV_FILE_USED_END_TIME = xe_make_matroska_id(0x4662),

	MKV_CHAPTERS = xe_make_matroska_id(0x1043a770),
	MKV_EDITION_ENTRY = xe_make_matroska_id(0x45b9),
	MKV_EDITION_UID = xe_make_matroska_id(0x45bc),
	MKV_EDITION_FLAG_HIDDEN = xe_make_matroska_id(0x45bd),
	MKV_EDITION_FLAG_DEFAULT = xe_make_matroska_id(0x45db),
	MKV_EDITION_FLAG_ORDERED = xe_make_matroska_id(0x45dd),
	MKV_CHAPTER_ATOM = xe_make_matroska_id(0xb6),
	MKV_CHAPTER_UID = xe_make_matroska_id(0x73c4),
	MKV_CHAPTER_STRING_UID = xe_make_matroska_id(0x5654),
	MKV_CHAPTER_TIME_START = xe_make_matroska_id(0x91),
	MKV_CHAPTER_TIME_END = xe_make_matroska_id(0x92),
	MKV_CHAPTER_FLAG_HIDDEN = xe_make_matroska_id(0x98),
	MKV_CHAPTER_FLAG_ENABLED = xe_make_matroska_id(0x4598),
	MKV_CHAPTER_SEGMENT_UID = xe_make_matroska_id(0x6e67),
	MKV_CHAPTER_SEGMENT_EDITION_UID = xe_make_matroska_id(0x6ebc),
	MKV_CHAPTER_PHYSICAL_EQUIV = xe_make_matroska_id(0x63c3),
	MKV_CHAPTER_TRACK = xe_make_matroska_id(0x8f),
	MKV_CHAPTER_TRACK_UID = xe_make_matroska_id(0x89),
	MKV_CHAPTER_DISPLAY = xe_make_matroska_id(0x80),
	MKV_CHAP_STRING = xe_make_matroska_id(0x85),
	MKV_CHAP_LANGUAGE = xe_make_matroska_id(0x437c),
	MKV_CHAP_LANGUAGE_IETF = xe_make_matroska_id(0x437d),
	MKV_CHAP_COUNTRY = xe_make_matroska_id(0x437e),
	MKV_CHAP_PROCESS = xe_make_matroska_id(0x6944),
	MKV_CHAP_PROCESS_CODEC_ID = xe_make_matroska_id(0x6955),
	MKV_CHAP_PROCESS_PRIVATE = xe_make_matroska_id(0x450d),
	MKV_CHAP_PROCESS_COMMAND = xe_make_matroska_id(0x6911),
	MKV_CHAP_PROCESS_TIME = xe_make_matroska_id(0x6922),
	MKV_CHAP_PROCESS_DATA = xe_make_matroska_id(0x6933),

	MKV_TAGS = xe_make_matroska_id(0x1254c367),
	MKV_TAG = xe_make_matroska_id(0x7373),
	MKV_TAG_TARGETS = xe_make_matroska_id(0x63c0),
	MKV_TAG_TARGET_TYPE_VALUE = xe_make_matroska_id(0x68ca),
	MKV_TAG_TARGET_TYPE = xe_make_matroska_id(0x63ca),
	MKV_TAG_TRACK_UID = xe_make_matroska_id(0x63c5),
	MKV_TAG_EDITION_UID = xe_make_matroska_id(0x63c9),
	MKV_TAG_CHAPTER_UID = xe_make_matroska_id(0x63c4),
	MKV_TAG_ATTACHMENT_UID = xe_make_matroska_id(0x63c6),
	MKV_TAG_SIMPLE = xe_make_matroska_id(0x67c8),
	MKV_TAG_NAME = xe_make_matroska_id(0x45a3),
	MKV_TAG_LANGUAGE = xe_make_matroska_id(0x447a),
	MKV_TAG_LANGUAGE_IETF = xe_make_matroska_id(0x447b),
	MKV_TAG_DEFAULT = xe_make_matroska_id(0x4484),
	MKV_TAG_DEFAULT_BOGUS = xe_make_matroska_id(0x44b4),
	MKV_TAG_STRING = xe_make_matroska_id(0x4487),
	MKV_TAG_BINARY = xe_make_matroska_id(0x4485)
};

static constexpr xe_cstr xe_matroska_id_str(xe_matroska_id id){
	switch(id){
		case EBML_ROOT:
			return "Root";

		case EBML_HEADER:
			return "Header";
		case EBML_VERSION:
			return "Version";
		case EBML_READER_VERSION:
			return "Reader Version";
		case EBML_MAX_ID_LENGTH:
			return "Max ID Length";
		case EBML_MAX_SIZE_LENGTH:
			return "Max Size Length";
		case EBML_DOCTYPE:
			return "DocType";
		case EBML_DOCTYPE_VERSION:
			return "DocType Version";
		case EBML_DOCTYPE_READER_VERSION:
			return "DocType Reader Version";
		case EBML_DOCTYPE_EXTENSION:
			return "DocType Extension";
		case EBML_DOCTYPE_EXTENSION_NAME:
			return "DocType Extension Name";
		case EBML_DOCTYPE_EXTENSION_VERSION:
			return "DocType Extension Version";

		case CRC32:
			return "CRC32";
		case VOID:
			return "Void";

		case MKV_SEGMENT:
			return "Segment";

		case MKV_SEGMENT_INFO:
			return "Info";
		case MKV_SEGMENT_TIMECODE_SCALE:
			return "Timecode Scale";
		case MKV_SEGMENT_DURATION:
			return "Duration";
		case MKV_SEGMENT_UID:
			return "UID";
		case MKV_SEGMENT_FILENAME:
			return "Filename";
		case MKV_SEGMENT_PREV_UID:
			return "Prev UID";
		case MKV_SEGMENT_PREV_FILENAME:
			return "Prev Filename";
		case MKV_SEGMENT_NEXT_UID:
			return "Next UID";
		case MKV_SEGMENT_NEXT_FILENAME:
			return "Next Filename";
		case MKV_SEGMENT_FAMILY:
			return "Family";
		case MKV_SEGMENT_CHAPTER_TRANSLATE:
			return "Chapter Translate";
		case MKV_SEGMENT_CHAPTER_TRANSLATE_ID:
			return "Chapter Translate ID";
		case MKV_SEGMENT_CHAPTER_TRANSLATE_CODEC:
			return "Chapter Translate Codec";
		case MKV_SEGMENT_CHAPTER_TRANSLATE_EDITION_UID:
			return "Chapter Translate Edition UID";
		case MKV_SEGMENT_DATE_UTC:
			return "Date UTC";
		case MKV_SEGMENT_TITLE:
			return "Title";
		case MKV_SEGMENT_MUXING_APP:
			return "Muxing App";
		case MKV_SEGMENT_WRITING_APP:
			return "Writing App";

		case MKV_SEEK_HEAD:
			return "Seek Head";
		case MKV_SEEK:
			return "Seek";
		case MKV_SEEK_ID:
			return "ID";
		case MKV_SEEK_POSITION:
			return "Position";

		case MKV_TRACKS:
			return "Tracks";
		case MKV_TRACK:
			return "Track";
		case MKV_TRACK_NUMBER:
			return "Number";
		case MKV_TRACK_UID:
			return "UID";
		case MKV_TRACK_TYPE:
			return "Type";
		case MKV_TRACK_FLAG_ENABLED:
			return "Enabled";
		case MKV_TRACK_FLAG_DEFAULT:
			return "Default";
		case MKV_TRACK_FLAG_FORCED:
			return "Forced";
		case MKV_TRACK_DEFAULT_DURATION:
			return "Default Duration";
		case MKV_TRACK_TIMECODE_SCALE:
			return "Timecode Scale";
		case MKV_TRACK_CODEC_ID:
			return "Codec ID";
		case MKV_TRACK_CODEC_PRIVATE:
			return "Codec Private";
		case MKV_TRACK_CODEC_NAME:
			return "Codec Name";
		case MKV_TRACK_FLAG_LACING:
			return "Lacing";
		case MKV_TRACK_LANGUAGE:
			return "Language";
		case MKV_TRACK_CODEC_DELAY:
			return "Codec Delay";
		case MKV_TRACK_SEEK_PREROLL:
			return "Seek Preroll";
		case MKV_TRACK_FLAG_HEARING_IMPAIRED:
			return "Hearing Impaired";
		case MKV_TRACK_FLAG_VISUAL_IMPAIRED:
			return "Visual Impaired";
		case MKV_TRACK_FLAG_TEXT_DESCRIPTIONS:
			return "Text Descriptions";
		case MKV_TRACK_FLAG_ORIGINAL:
			return "Original";
		case MKV_TRACK_FLAG_COMMENTARY:
			return "Commentary";
		case MKV_TRACK_MIN_CACHE:
			return "Min Cache";
		case MKV_TRACK_MAX_CACHE:
			return "Max Cache";
		case MKV_TRACK_DEFAULT_DECODED_FIELD_DURATION:
			return "Default Decoded Field Duration";
		case MKV_TRACK_OFFSET:
			return "Offset";
		case MKV_TRACK_MAX_BLOCK_ADDITION_ID:
			return "Max Block Addition ID";
		case MKV_TRACK_BLOCK_ADDITION_MAPPING:
			return "Block Addition Mapping";
		case MKV_TRACK_BLOCK_ADD_ID_VALUE:
			return "Block Add ID Value";
		case MKV_TRACK_BLOCK_ADD_ID_NAME:
			return "Block Add ID Name";
		case MKV_TRACK_BLOCK_ADD_ID_TYPE:
			return "Block Add ID Type";
		case MKV_TRACK_BLOCK_ADD_ID_EXTRA_DATA:
			return "Block Add ID Extra Data";
		case MKV_TRACK_NAME:
			return "Name";
		case MKV_TRACK_LANGUAGE_IETF:
			return "Language IETF";
		case MKV_TRACK_ATTACHMENT_LINK:
			return "Attachment Link";
		case MKV_TRACK_CODEC_SETTINGS:
			return "Codec Settings";
		case MKV_TRACK_CODEC_INFO_URL:
			return "Codec Info URL";
		case MKV_TRACK_CODEC_DOWNLOAD_URL:
			return "Codec Download URL";
		case MKV_TRACK_CODEC_DECODE_ALL:
			return "Codec Decode All";
		case MKV_TRACK_OVERLAY:
			return "Overlay";
		case MKV_TRACK_TRANSLATE:
			return "Translate";
		case MKV_TRACK_TRANSLATE_TRACK_ID:
			return "Translate Track ID";
		case MKV_TRACK_TRANSLATE_CODEC:
			return "Translate Codec";
		case MKV_TRACK_TRANSLATE_EDITION_UID:
			return "Translate Edition UID";
		case MKV_TRACK_VIDEO:
			return "Video";
		case MKV_TRACK_FLAG_INTERLACED:
			return "Interlaced";
		case MKV_TRACK_FIELD_ORDER:
			return "Field Order";
		case MKV_TRACK_STEREO_MODE:
			return "Stereo Mode";
		case MKV_TRACK_ALPHA_MODE:
			return "Alpha Mode";
		case MKV_TRACK_OLD_STEREO_MODE:
			return "Old Stereo Mode";
		case MKV_TRACK_PIXEL_WIDTH:
			return "Pixel Width";
		case MKV_TRACK_PIXEL_HEIGHT:
			return "Pixel Height";
		case MKV_TRACK_PIXEL_CROP_BOTTOM:
			return "Pixel Crop Bottom";
		case MKV_TRACK_PIXEL_CROP_TOP:
			return "Pixel Crop Top";
		case MKV_TRACK_PIXEL_CROP_LEFT:
			return "Pixel Crop Left";
		case MKV_TRACK_PIXEL_CROP_RIGHT:
			return "Pixel Crop Right";
		case MKV_TRACK_DISPLAY_WIDTH:
			return "Display Width";
		case MKV_TRACK_DISPLAY_HEIGHT:
			return "Display Height";
		case MKV_TRACK_DISPLAY_UNIT:
			return "Display Unit";
		case MKV_TRACK_ASPECT_RATIO_TYPE:
			return "Aspect Ratio Type";
		case MKV_TRACK_UNCOMPRESSED_FOUR_CC:
			return "Uncompressed Four CC";
		case MKV_TRACK_GAMMA_VALUE:
			return "Gamma Value";
		case MKV_TRACK_FRAME_RATE:
			return "Frame Rate";
		case MKV_TRACK_COLOUR:
			return "Colour";
		case MKV_TRACK_MATRIX_COEFFICIENTS:
			return "Matrix Coefficients";
		case MKV_TRACK_BITS_PER_CHANNEL:
			return "Bits Per Channel";
		case MKV_TRACK_CHROMA_SUBSAMPLING_HORZ:
			return "Chroma Subsampling Horz";
		case MKV_TRACK_CHROMA_SUBSAMPLING_VERT:
			return "Chroma Subsampling Vert";
		case MKV_TRACK_CB_SUBSAMPLING_HORZ:
			return "Cb Subsampling Horz";
		case MKV_TRACK_CB_SUBSAMPLING_VERT:
			return "Cb Subsampling Vert";
		case MKV_TRACK_CHROMA_SITING_HORZ:
			return "Chroma Siting Horz";
		case MKV_TRACK_CHROMA_SITING_VERT:
			return "Chroma Siting Vert";
		case MKV_TRACK_RANGE:
			return "Range";
		case MKV_TRACK_TRANSFER_CHARACTERISTICS:
			return "Transfer Characteristics";
		case MKV_TRACK_PRIMARIES:
			return "Primaries";
		case MKV_TRACK_MAX_CLL:
			return "Max CLL";
		case MKV_TRACK_MAX_FALL:
			return "Max FALL";
		case MKV_TRACK_MASTERING_METADATA:
			return "Mastering Metadata";
		case MKV_TRACK_PRIMARY_RCHROMATICITY_X:
			return "Primary RChromaticityX";
		case MKV_TRACK_PRIMARY_RCHROMATICITY_Y:
			return "Primary RChromaticityY";
		case MKV_TRACK_PRIMARY_GCHROMATICITY_X:
			return "Primary GChromaticityX";
		case MKV_TRACK_PRIMARY_GCHROMATICITY_Y:
			return "Primary GChromaticityY";
		case MKV_TRACK_PRIMARY_BCHROMATICITY_X:
			return "Primary BChromaticityX";
		case MKV_TRACK_PRIMARY_BCHROMATICITY_Y:
			return "Primary BChromaticityY";
		case MKV_TRACK_WHITE_POINT_CHROMATICITY_X:
			return "White Point ChromaticityX";
		case MKV_TRACK_WHITE_POINT_CHROMATICITY_Y:
			return "White Point ChromaticityY";
		case MKV_TRACK_LUMINANCE_MAX:
			return "Luminance Max";
		case MKV_TRACK_LUMINANCE_MIN:
			return "Luminance Min";
		case MKV_TRACK_PROJECTION:
			return "Projection";
		case MKV_TRACK_PROJECTION_TYPE:
			return "Projection Type";
		case MKV_TRACK_PROJECTION_PRIVATE:
			return "Projection Private";
		case MKV_TRACK_PROJECTION_POSE_YAW:
			return "Projection PoseYaw";
		case MKV_TRACK_PROJECTION_POSE_PITCH:
			return "Projection PosePitch";
		case MKV_TRACK_PROJECTION_POSE_ROLL:
			return "Projection PoseRoll";
		case MKV_TRACK_AUDIO:
			return "Audio";
		case MKV_TRACK_AUDIO_SAMPLING_FREQUENCY:
			return "Sampling Frequency";
		case MKV_TRACK_AUDIO_OUTPUT_SAMPLING_FREQUENCY:
			return "Output Sampling Frequency";
		case MKV_TRACK_AUDIO_CHANNELS:
			return "Channels";
		case MKV_TRACK_AUDIO_CHANNEL_POSITIONS:
			return "Channel Positions";
		case MKV_TRACK_AUDIO_BIT_DEPTH:
			return "Bit Depth";
		case MKV_TRACK_OPERATION:
			return "Operation";
		case MKV_TRACK_COMBINE_PLANES:
			return "CombinePlanes";
		case MKV_TRACK_PLANE:
			return "Plane";
		case MKV_TRACK_PLANE_UID:
			return "Plane UID";
		case MKV_TRACK_PLANE_TYPE:
			return "Plane Type";
		case MKV_TRACK_JOIN_BLOCKS:
			return "Join Blocks";
		case MKV_TRACK_JOIN_UID:
			return "Join UID";
		case MKV_TRACK_TRICK_TRACK_UID:
			return "Trick Track UID";
		case MKV_TRACK_TRICK_TRACK_SEGMENT_UID:
			return "Trick Track Segment UID";
		case MKV_TRACK_TRICK_TRACK_FLAG:
			return "Trick Track Flag";
		case MKV_TRACK_TRICK_MASTER_TRACK_UID:
			return "Trick Master Track UID";
		case MKV_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID:
			return "Trick Master Track Segment UID";
		case MKV_TRACK_CONTENT_ENCODINGS:
			return "Content Encodings";
		case MKV_TRACK_CONTENT_ENCODING:
			return "Content Encoding";
		case MKV_TRACK_CONTENT_ENCODING_ORDER:
			return "Content Encoding Order";
		case MKV_TRACK_CONTENT_ENCODING_SCOPE:
			return "Content Encoding Scope";
		case MKV_TRACK_CONTENT_ENCODING_TYPE:
			return "Content Encoding Type";
		case MKV_TRACK_CONTENT_COMPRESSION:
			return "Content Compression";
		case MKV_TRACK_CONTENT_COMP_ALGO:
			return "Content Compression Algo";
		case MKV_TRACK_CONTENT_COMP_SETTINGS:
			return "Content Compression Settings";
		case MKV_TRACK_CONTENT_ENCRYPTION:
			return "Content Encryption";
		case MKV_TRACK_CONTENT_ENC_ALGO:
			return "Content Encryption Algo";
		case MKV_TRACK_CONTENT_ENC_KEY_ID:
			return "Content Encryption Key ID";
		case MKV_TRACK_CONTENT_ENC_AESSETTINGS:
			return "Content Encryption AES Settings";
		case MKV_TRACK_AESSETTINGS_CIPHER_MODE:
			return "AES Settings Cipher Mode";
		case MKV_TRACK_CONTENT_SIGNATURE:
			return "Content Signature";
		case MKV_TRACK_CONTENT_SIG_KEY_ID:
			return "Content Signature Key ID";
		case MKV_TRACK_CONTENT_SIG_ALGO:
			return "Content Signature Algorithm";
		case MKV_TRACK_CONTENT_SIG_HASH_ALGO:
			return "Content Signature Hash Algorithm";

		case MKV_CUES:
			return "Cues";
		case MKV_CUE_POINT:
			return "Cue Point";
		case MKV_CUE_TIME:
			return "Time";
		case MKV_CUE_TRACK_POSITION:
			return "Track Position";
		case MKV_CUE_TRACK:
			return "Track";
		case MKV_CUE_CLUSTER_POSITION:
			return "Cluster Position";
		case MKV_CUE_BLOCK_NUMBER:
			return "Block Number";
		case MKV_CUE_RELATIVE_POSITION:
			return "CueRelativePosition";
		case MKV_CUE_DURATION:
			return "CueDuration";
		case MKV_CUE_CODEC_STATE:
			return "CueCodecState";
		case MKV_CUE_REFERENCE:
			return "CueReference";
		case MKV_CUE_REF_TIME:
			return "CueRefTime";
		case MKV_CUE_REF_CLUSTER:
			return "CueRefCluster";
		case MKV_CUE_REF_NUMBER:
			return "CueRefNumber";
		case MKV_CUE_REF_CODEC_STATE:
			return "CueRefCodecState";

		case MKV_CLUSTER:
			return "Cluster";
		case MKV_CLUSTER_TIMECODE:
			return "Timecode";
		case MKV_CLUSTER_POSITION:
			return "Position";
		case MKV_CLUSTER_SIMPLE_BLOCK:
			return "Simple Block";
		case MKV_CLUSTER_BLOCK_GROUP:
			return "Block Group";
		case MKV_CLUSTER_BLOCK_GROUP_BLOCK:
			return "Block";
		case MKV_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK:
			return "Reference Block";
		case MKV_CLUSTER_BLOCK_GROUP_BLOCK_DURATION:
			return "Duration";
		case MKV_CLUSTER_SILENT_TRACKS:
			return "SilentTracks";
		case MKV_CLUSTER_SILENT_TRACK_NUMBER:
			return "SilentTrackNumber";
		case MKV_CLUSTER_PREV_SIZE:
			return "PrevSize";
		case MKV_CLUSTER_BLOCK_VIRTUAL:
			return "BlockVirtual";
		case MKV_CLUSTER_BLOCK_ADDITIONS:
			return "BlockAdditions";
		case MKV_CLUSTER_BLOCK_MORE:
			return "BlockMore";
		case MKV_CLUSTER_BLOCK_ADD_ID:
			return "BlockAddID";
		case MKV_CLUSTER_BLOCK_ADDITIONAL:
			return "BlockAdditional";
		case MKV_CLUSTER_REFERENCE_PRIORITY:
			return "ReferencePriority";
		case MKV_CLUSTER_REFERENCE_VIRTUAL:
			return "ReferenceVirtual";
		case MKV_CLUSTER_CODEC_STATE:
			return "CodecState";
		case MKV_CLUSTER_DISCARD_PADDING:
			return "DiscardPadding";
		case MKV_CLUSTER_SLICES:
			return "Slices";
		case MKV_CLUSTER_TIME_SLICE:
			return "TimeSlice";
		case MKV_CLUSTER_LACE_NUMBER:
			return "LaceNumber";
		case MKV_CLUSTER_FRAME_NUMBER:
			return "FrameNumber";
		case MKV_CLUSTER_BLOCK_ADDITION_ID:
			return "BlockAdditionID";
		case MKV_CLUSTER_DELAY:
			return "Delay";
		case MKV_CLUSTER_SLICE_DURATION:
			return "SliceDuration";
		case MKV_CLUSTER_REFERENCE_FRAME:
			return "ReferenceFrame";
		case MKV_CLUSTER_REFERENCE_OFFSET:
			return "ReferenceOffset";
		case MKV_CLUSTER_REFERENCE_TIMESTAMP:
			return "ReferenceTimestamp";
		case MKV_CLUSTER_ENCRYPTED_BLOCK:
			return "EncryptedBlock";

		case MKV_ATTACHMENTS:
			return "Attachments";
		case MKV_ATTACHED_FILE:
			return "Attached File";
		case MKV_FILE_DESCRIPTION:
			return "File Description";
		case MKV_FILE_NAME:
			return "File Name";
		case MKV_FILE_MIME_TYPE:
			return "File Mime Type";
		case MKV_FILE_DATA:
			return "File Data";
		case MKV_FILE_UID:
			return "File UID";
		case MKV_FILE_REFERRAL:
			return "File Referral";
		case MKV_FILE_USED_START_TIME:
			return "File Used Start Time";
		case MKV_FILE_USED_END_TIME:
			return "File Used End Time";
		case MKV_CHAPTERS:
			return "Chapters";
		case MKV_EDITION_ENTRY:
			return "Edition";
		case MKV_EDITION_UID:
			return "UID";
		case MKV_EDITION_FLAG_HIDDEN:
			return "Hidden";
		case MKV_EDITION_FLAG_DEFAULT:
			return "Default";
		case MKV_EDITION_FLAG_ORDERED:
			return "Ordered";
		case MKV_CHAPTER_ATOM:
			return "Chapter Atom";
		case MKV_CHAPTER_UID:
			return "Chapter UID";
		case MKV_CHAPTER_STRING_UID:
			return "Chapter String UID";
		case MKV_CHAPTER_TIME_START:
			return "Chapter Time Start";
		case MKV_CHAPTER_TIME_END:
			return "Chapter Time End";
		case MKV_CHAPTER_FLAG_HIDDEN:
			return "Hidden";
		case MKV_CHAPTER_FLAG_ENABLED:
			return "Enabled";
		case MKV_CHAPTER_SEGMENT_UID:
			return "Segment UID";
		case MKV_CHAPTER_SEGMENT_EDITION_UID:
			return "Segment Edition UID";
		case MKV_CHAPTER_PHYSICAL_EQUIV:
			return "Physical Equiv";
		case MKV_CHAPTER_TRACK:
			return "Track";
		case MKV_CHAPTER_TRACK_UID:
			return "Track UID";
		case MKV_CHAPTER_DISPLAY:
			return "Display";
		case MKV_CHAP_STRING:
			return "Chap String";
		case MKV_CHAP_LANGUAGE:
			return "Chap Language";
		case MKV_CHAP_LANGUAGE_IETF:
			return "Chap Language IETF";
		case MKV_CHAP_COUNTRY:
			return "Chap Country";
		case MKV_CHAP_PROCESS:
			return "Chap Process";
		case MKV_CHAP_PROCESS_CODEC_ID:
			return "Chap Process Codec ID";
		case MKV_CHAP_PROCESS_PRIVATE:
			return "Chap Process Private";
		case MKV_CHAP_PROCESS_COMMAND:
			return "Chap Process Command";
		case MKV_CHAP_PROCESS_TIME:
			return "Chap Process Time";
		case MKV_CHAP_PROCESS_DATA:
			return "Chap Process Data";
		case MKV_TAGS:
			return "Tags";
		case MKV_TAG:
			return "Tag";
		case MKV_TAG_TARGETS:
			return "Targets";
		case MKV_TAG_TARGET_TYPE_VALUE:
			return "Target Type Value";
		case MKV_TAG_TARGET_TYPE:
			return "Target Type";
		case MKV_TAG_TRACK_UID:
			return "Track UID";
		case MKV_TAG_EDITION_UID:
			return "Edition UID";
		case MKV_TAG_CHAPTER_UID:
			return "Chapter UID";
		case MKV_TAG_ATTACHMENT_UID:
			return "Attachment UID";
		case MKV_TAG_SIMPLE:
			return "SimpleTag";
		case MKV_TAG_NAME:
			return "Name";
		case MKV_TAG_LANGUAGE:
			return "Language";
		case MKV_TAG_LANGUAGE_IETF:
			return "LanguageIETF";
		case MKV_TAG_DEFAULT:
			return "Default";
		case MKV_TAG_DEFAULT_BOGUS:
			return "DefaultBogus";
		case MKV_TAG_STRING:
			return "String";
		case MKV_TAG_BINARY:
			return "Binary";
	}

	return "UNKNOWN";
}

enum xe_matroska_track_type{
	MKV_VIDEO = 0x01,
	MKV_AUDIO = 0x02,
	MKV_COMPLEX = 0x03,
	MKV_LOGO = 0x10,
	MKV_SUBTITLE = 0x11,
	MKV_BUTTON = 0x12,
	MKV_CONTROL = 0x20,
	MKV_METADATA = 0x21,
};

enum{
	EBML_UNKNOWN_LENGTH = 0xffffffffffffffff
};

enum xe_vint_type{
	XE_VINT,
	XE_VSINT,
	XE_VINT_ID,
	XE_VINT_SIZE
};

class xe_ebml_element{
public:
	ulong size;
	ulong offset;
	ulong end;
	xe_matroska_id id;
};

struct xe_seek{
	xe_matroska_id id;
	ulong position;
};

struct xe_matroska_track : public xe_track{
	ulong number;
	ulong default_duration;

	bool has_content_encodings;
	bool has_attachments;
};

struct xe_cue_track_position{
	ulong track;
	ulong position;
};

struct xe_matroska_cue{
	ulong time;

	xe_vector<xe_cue_track_position> track_positions;

	xe_cue_track_position* alloc_track_position(){
		size_t size = track_positions.size();

		if(!track_positions.grow(size + 1))
			return null;
		track_positions.resize(size + 1);

		xe_zero(&track_positions[size]);

		return &track_positions[size];
	}
};

class xe_matroska;
class xe_matroska_reader{
public:
	xe_matroska& matroska;
	xe_reader& reader;

	ulong segment_offset;
	xe_ebml_element stack[16];
	uint depth;

	union{
		xe_ptr ptr;
		xe_seek* seek;
		xe_matroska_track* track;
		xe_matroska_cue* cue;
	};

	xe_matroska_reader(xe_matroska& matroska, xe_reader& reader):
		matroska(matroska), reader(reader){
		depth = 0;
	}

	template<xe_vint_type type = XE_VINT, bool strict = false>
	int read_vint(ulong& result){
		size_t length, bits;

		int err;

		result = reader.r8();

		if(!result)
			goto err;
		length = xe_arch_clzl(result << 56);
		bits = 1 << (7 - length);
		result ^= bits;
		bits = (bits << length * 8) - 1;

		for(size_t i = 0; i < length; i++){
			result <<= 8;
			result |= reader.r8();
		}

		switch(type){
			case XE_VINT:
				break;
			case XE_VSINT:
				result -= (1ul << (length * 7 + 6)) - 1;

				break;
			case XE_VINT_ID:
				if(strict && (!result || result == bits))
					goto err;
				break;
			case XE_VINT_SIZE:
				if(result == bits)
					result = EBML_UNKNOWN_LENGTH;
				break;
		}

		return reader.error();

		err:

		if((err = reader.error()))
			return err;
		return XE_INVALID_DATA;
	}

	int read_vsint(long& result){
		return read_vint<XE_VSINT>((ulong&)result);
	}

	int read_id(xe_matroska_id& id){
		return read_vint<XE_VINT_ID>((ulong&)id);
	}

	int read_size(ulong& size){
		return read_vint<XE_VINT_SIZE>(size);
	}

	int read_uint(xe_ebml_element& element, ulong& result){
		if(!element.size)
			return 0;
		if(element.size > 8)
			return XE_INVALID_DATA;
		result = 0;

		for(uint i = 0; i < element.size; i++){
			result <<= 8;
			result |= reader.r8();
		}

		return 0;
	}

	int read_sint(xe_ebml_element& element, long& result){
		ulong rval;

		if(!element.size)
			return 0;
		if(element.size > 8)
			return XE_INVALID_DATA;
		rval = (long)(signed char)reader.r8();

		for(uint i = 1; i < element.size; i++){
			rval <<= 8;
			rval |= reader.r8();
		}

		result = rval;

		return 0;
	}

	int read_float(xe_ebml_element& element, double& result){
		if(element.size == 4)
			result = reader.f32be();
		else if(element.size == 8)
			result = reader.f64be();
		else if(element.size != 0)
			return XE_INVALID_DATA;
		return 0;
	}

	template<typename T>
	struct match_single{
		xe_string string;
		T value;
	};

	template<size_t N, typename T>
	int string_match(xe_ebml_element& element, T& value, const match_single<T> (&match)[N]){
		int err;

		value = match[N - 1].value;

		for(size_t i = 0; i < N - 1; i++){
			const xe_string& string = match[i].string;

			if(element.size != string.length())
				continue;
			char data[string.length()];

			if((err = reader.read(data, string.length())))
				return err;
			for(; i < N - 1; i++){
				if(xe_string(data, string.length()) == match[i].string){
					value = match[i].value;

					break;
				}
			}

			break;
		}

		return 0;
	}

	ulong element_left(xe_ebml_element& element){
		return element.offset + element.size - reader.offset();
	}

	bool element_has(xe_ebml_element& element, ulong bytes){
		if(element.size == EBML_UNKNOWN_LENGTH)
			return element.end ? element.end >= bytes + reader.offset() : true;
		return element.offset + element.size >= bytes + reader.offset();
	}

	void stack_push(xe_ebml_element& element){
		xe_log_debug(this, "%*s%s : %lu @ %lu", depth * 2, "", xe_matroska_id_str(element.id), element.size, element.offset);

		stack[depth++] = element; // TODO move stack copy to read children
	}

	void stack_pop(){
		xe_ebml_element& element = stack_top();

		element_finished(element);
		xe_log_debug(this, "%*s%li bytes remaining", depth * 2, "", element.size == EBML_UNKNOWN_LENGTH ? 0 : element.offset + element.size - reader.offset());

		depth--;
	}

	xe_ebml_element& stack_top(){
		return stack[depth - 1];
	}

	template<
		enum xe_matroska_id parent_id,
		int (xe_matroska_reader::*parse)(xe_ebml_element& element),
		bool master,
		bool skip_if_not_master
	> int element_handler(xe_ebml_element& element){
		xe_matroska_id parent;

		if(!master && element.size == EBML_UNKNOWN_LENGTH)
			return XE_INVALID_DATA;
		parent = depth ? stack_top().id : EBML_ROOT;

		if(parent != parent_id){
			if(depth && stack_top().size == EBML_UNKNOWN_LENGTH){
				if(parent_id == EBML_ROOT){
					while(depth)
						stack_pop();
				}else if(depth >= 2 && stack[depth - 2].id == element.id){
					stack_pop();
					stack_pop();
				}else if(depth >= 2){
					for(uint i = 0; i <= depth - 2; i++){
						if(stack[depth - i - 2].id == parent_id)
							for(uint j = 0; j <= i; j++)
								stack_pop();
					}
				}else{
					return 0;
				}
			}else{
				return 0;
			}
		}

		if(master && element.size == EBML_UNKNOWN_LENGTH){
			if(depth){
				xe_ebml_element& top = stack_top();

				if(top.size != EBML_UNKNOWN_LENGTH)
					element.end = top.offset + top.size;
				else if(top.end)
					element.end = top.end;
			}else{
				element.end = 0;
			}
		}

		int err;

		stack_push(element);

		err = (this ->* parse)(element);

		if(!master && skip_if_not_master && !err){
			stack_pop();

			err = skip_element(element);
		}

		return err;
	}

	int read_master(xe_ebml_element& element){
		return 0;
	}

	template<
		int (xe_matroska_reader::*parse)(xe_ebml_element& element) = &xe_matroska_reader::read_master
	> int handle_root(xe_ebml_element& element){
		return element_handler<EBML_ROOT, parse, true, true>(element);
	}

	template<
		enum xe_matroska_id parent_id,
		int (xe_matroska_reader::*parse)(xe_ebml_element& element) = &xe_matroska_reader::read_master
	> int handle_master(xe_ebml_element& element){
		return element_handler<parent_id, parse, true, true>(element);
	}

	template<
		enum xe_matroska_id parent_id,
		int (xe_matroska_reader::*parse)(xe_ebml_element& element),
		bool skip_if_not_master = true
	> int parse_element(xe_ebml_element& element){
		return element_handler<parent_id, parse, false, skip_if_not_master>(element);
	}

	template<
		int (xe_matroska_reader::*parse)(xe_ebml_element& element, ulong result)
	> int handle_uint(xe_ebml_element& element){
		ulong result;
		int err;

		if((err = read_uint(element, result)))
			return err;
		return (this ->* parse)(element, result);
	}

	template<
		enum xe_matroska_id parent_id,
		int (xe_matroska_reader::*parse)(xe_ebml_element& element, ulong result)
	> int parse_uint(xe_ebml_element& element){
		return parse_element<parent_id, &xe_matroska_reader::handle_uint<parse>>(element);
	}

	template<
		int (xe_matroska_reader::*parse)(xe_ebml_element& element, double result)
	> int handle_float(xe_ebml_element& element){
		double result;
		int err;

		if((err = read_float(element, result)))
			return err;
		return (this ->* parse)(element, result);
	}

	template<
		enum xe_matroska_id parent_id,
		int (xe_matroska_reader::*parse)(xe_ebml_element& element, double result)
	> int parse_float(xe_ebml_element& element){
		return parse_element<parent_id, &xe_matroska_reader::handle_float<parse>>(element);
	}

	int read_children(){
		int err;

		xe_ebml_element element;

		while(true){
			while(depth && !element_has(stack_top(), 2))
				stack_pop();
			if((err = read_id(element.id))){
				if(depth){
					xe_ebml_element& top = stack_top();

					if(top.size == EBML_UNKNOWN_LENGTH && !top.end && err == XE_EOF){
						err = 0;
						depth = 0;
					}
				}

				break;
			}

			if((err = read_size(element.size)))
				break;
			element.offset = reader.offset();

			switch(element.id){
				case EBML_HEADER:
					err = handle_root(element);

					break;
				case EBML_READER_VERSION:
					err = parse_uint<EBML_HEADER, &xe_matroska_reader::read_ebml_version>(element);

					break;
				case EBML_MAX_ID_LENGTH:
					err = parse_uint<EBML_HEADER, &xe_matroska_reader::read_ebml_max_id_length>(element);

					break;
				case EBML_MAX_SIZE_LENGTH:
					err = parse_uint<EBML_HEADER, &xe_matroska_reader::read_ebml_max_size_length>(element);

					break;
				case EBML_DOCTYPE:
					err = parse_element<EBML_HEADER, &xe_matroska_reader::read_ebml_doctype>(element);

					break;
				case EBML_DOCTYPE_READER_VERSION:
					err = parse_uint<EBML_HEADER, &xe_matroska_reader::read_ebml_doctype_version>(element);

					break;
				case MKV_SEGMENT:
					err = handle_root<&xe_matroska_reader::read_segment>(element);

					break;
				case MKV_SEGMENT_INFO:
					err = handle_master<MKV_SEGMENT>(element);

					break;
				case MKV_SEGMENT_DURATION:
					err = parse_uint<MKV_SEGMENT_INFO, &xe_matroska_reader::read_segment_info_duration>(element);

					break;
				case MKV_SEGMENT_TIMECODE_SCALE:
					err = parse_uint<MKV_SEGMENT_INFO, &xe_matroska_reader::read_segment_info_timecode_scale>(element);

					break;
				case MKV_SEEK_HEAD:
					err = handle_master<MKV_SEGMENT>(element);

					break;
				case MKV_SEEK:
					err = handle_master<MKV_SEEK_HEAD, &xe_matroska_reader::read_seek>(element);

					break;
				case MKV_SEEK_ID:
					err = parse_element<MKV_SEEK, &xe_matroska_reader::read_seek_id>(element);

					break;
				case MKV_SEEK_POSITION:
					err = parse_uint<MKV_SEEK, &xe_matroska_reader::read_seek_position>(element);

					break;
				case MKV_TRACKS:
					err = handle_master<MKV_SEGMENT>(element);

					break;
				case MKV_TRACK:
					err = handle_master<MKV_TRACKS, &xe_matroska_reader::read_track>(element);

					break;
				case MKV_TRACK_NUMBER:
					err = parse_uint<MKV_TRACK, &xe_matroska_reader::read_track_number>(element);

					break;
				case MKV_TRACK_TYPE:
					err = parse_uint<MKV_TRACK, &xe_matroska_reader::read_track_type>(element);

					break;
				case MKV_TRACK_DEFAULT_DURATION:
					err = parse_uint<MKV_TRACK, &xe_matroska_reader::read_track_default_duration>(element);

					break;
				case MKV_TRACK_TIMECODE_SCALE:
					err = parse_float<MKV_TRACK, &xe_matroska_reader::read_track_timecode_scale>(element);

					break;
				case MKV_TRACK_CODEC_ID:
					err = parse_element<MKV_TRACK, &xe_matroska_reader::read_track_codec_id>(element);

					break;
				case MKV_TRACK_CODEC_PRIVATE:
					err = parse_element<MKV_TRACK, &xe_matroska_reader::read_track_codec_private>(element);

					break;
				case MKV_TRACK_AUDIO:
					err = handle_master<MKV_TRACK>(element);

					break;
				case MKV_TRACK_AUDIO_SAMPLING_FREQUENCY:
					err = parse_float<MKV_TRACK_AUDIO, &xe_matroska_reader::read_track_audio_sampling_frequency>(element);

					break;
				case MKV_TRACK_AUDIO_OUTPUT_SAMPLING_FREQUENCY:
					err = parse_float<MKV_TRACK_AUDIO, &xe_matroska_reader::read_track_audio_output_sampling_frequency>(element);

					break;
				case MKV_TRACK_AUDIO_CHANNELS:
					err = parse_uint<MKV_TRACK_AUDIO, &xe_matroska_reader::read_track_audio_channels>(element);

					break;
				case MKV_TRACK_AUDIO_BIT_DEPTH:
					err = parse_uint<MKV_TRACK_AUDIO, &xe_matroska_reader::read_track_audio_bit_depth>(element);

					break;
				case MKV_CUES:
					err = handle_master<MKV_SEGMENT>(element);

					break;
				case MKV_CUE_POINT:
					err = handle_master<MKV_CUES, &xe_matroska_reader::read_cue_point>(element);

					break;
				case MKV_CUE_TIME:
					err = parse_uint<MKV_CUE_POINT, &xe_matroska_reader::read_cue_time>(element);

					break;
				case MKV_CUE_TRACK_POSITION:
					err = handle_master<MKV_CUE_POINT, &xe_matroska_reader::read_cue_track_position>(element);

					break;
				case MKV_CUE_TRACK:
					err = parse_uint<MKV_CUE_TRACK_POSITION, &xe_matroska_reader::read_cue_track>(element);

					break;
				case MKV_CUE_CLUSTER_POSITION:
					err = parse_uint<MKV_CUE_TRACK_POSITION, &xe_matroska_reader::read_cue_cluster_position>(element);

					break;
				case MKV_CLUSTER:
					err = handle_master<MKV_SEGMENT>(element);

					break;
				case MKV_CLUSTER_TIMECODE:
					err = parse_element<MKV_CLUSTER, &xe_matroska_reader::read_cluster_timecode>(element);

					break;
				case MKV_CLUSTER_SIMPLE_BLOCK:
					err = parse_element<MKV_CLUSTER, &xe_matroska_reader::read_simple_block, false>(element);

					return 0;
				default:
					stack_push(element);
					stack_pop();

					err = skip_element(element);

					break;
			}

			if(err)
				return err;
			if(!element_has(element, 0))
				break;
			if((err = reader.error()))
				break;
		}

		return err;
	}

	int element_finished(xe_ebml_element& element){
		switch(element.id){
			case MKV_TRACK:
				break;
		}

		return 0;
	}

	int read_ebml_version(xe_ebml_element& element, ulong version){
		if(!version)
			return XE_INVALID_DATA;
		if(version != 1)
			return XE_ENOSYS;
		return 0;
	}

	int read_ebml_max_id_length(xe_ebml_element& element, ulong length){
		if(!length)
			return XE_INVALID_DATA;
		if(length > 8)
			return XE_ENOSYS;
		return 0;
	}

	int read_ebml_max_size_length(xe_ebml_element& element, ulong length){
		if(!length)
			return XE_INVALID_DATA;
		if(length > 8)
			return XE_ENOSYS;
		return 0;
	}

	int read_ebml_doctype(xe_ebml_element& element){
		constexpr match_single<bool> types[]{
			{"matroska", true},
			{"webm", true},
			{{}, false}
		};

		bool match;
		int err;

		if((err = string_match(element, match, types)))
			return err;
		if(!match)
			return XE_ENOSYS;
		return 0;
	}

	int read_ebml_doctype_version(xe_ebml_element& element, ulong version){
		if(!version)
			return XE_INVALID_DATA;
		if(version > 4)
			return XE_ENOSYS;
		return 0;
	}

	int read_segment(xe_ebml_element& element){
		segment_offset = element.offset;

		return 0;
	}

	int read_segment_info_duration(xe_ebml_element& element, ulong dur){
		duration() = dur;

		return 0;
	}

	int read_segment_info_timecode_scale(xe_ebml_element& element, ulong scale){
		timecode_scale() = scale;

		return 0;
	}

	int read_seek(xe_ebml_element& element){
		seek = alloc_seek();

		if(!seek)
			return XE_ENOMEM;
		return 0;
	}

	int read_seek_id(xe_ebml_element& element){
		ulong id;
		int err;

		if((err = read_vint<XE_VINT_ID>(id)))
			return err;
		seek -> id = (xe_matroska_id)id;

		return 0;
	}

	int read_seek_position(xe_ebml_element& element, ulong position){
		seek -> position = position + segment_offset;

		return 0;
	}

	int read_track(xe_ebml_element& element){
		track = alloc_track();

		if(!track)
			return XE_ENOMEM;
		track -> timescale.den = 1'000'000'000; // TODO after reading the entire track
		track -> timescale.num = timecode_scale();
		track -> timescale.reduce();

		return 0;
	}

	int read_track_number(xe_ebml_element& element, ulong number){
		if(!number)
			return XE_INVALID_DATA;
		track -> number = number;

		return 0;
	}

	int read_track_type(xe_ebml_element& element, ulong type){
		switch(type){
			case MKV_VIDEO:
				track -> type = XE_TRACK_TYPE_VIDEO;

				break;
			case MKV_AUDIO:
				track -> type = XE_TRACK_TYPE_AUDIO;

				break;
			case MKV_COMPLEX:
			case MKV_LOGO:
			case MKV_SUBTITLE:
			case MKV_BUTTON:
			case MKV_CONTROL:
				track -> type = XE_TRACK_TYPE_OTHER;

				break;
			default:
				return XE_INVALID_DATA;
		}

		return 0;
	}

	int read_track_default_duration(xe_ebml_element& element, ulong duration){
		track -> default_duration = duration;

		return 0;
	}

	int read_track_timecode_scale(xe_ebml_element& element, double scale){
		return XE_ENOSYS;
	}

	int read_track_codec_id(xe_ebml_element& element){
		constexpr match_single<xe_codec_id> types[]{
			{"A_OPUS", XE_CODEC_OPUS},
			{"A_VORBIS", XE_CODEC_VORBIS},
			{"A_AAC", XE_CODEC_AAC},
			{"A_FLAC", XE_CODEC_FLAC},
			{{}, XE_CODEC_NONE}
		};

		int err;

		if((err = string_match(element, track -> codec.id, types)))
			return err;
		if(track -> codec.id != XE_CODEC_AAC)
			track -> parse = XE_PARSE_HEADER;
		return 0;
	}

	int read_track_codec_private(xe_ebml_element& element){
		if(!track -> codec.alloc_config(element.size))
			return XE_ENOMEM;
		reader.read(track -> codec.config.data(), element.size);

		return 0;
	}

	int read_track_audio_sampling_frequency(xe_ebml_element& element, double frequency){
		track -> codec.sample_rate = frequency;

		return 0;
	}

	int read_track_audio_output_sampling_frequency(xe_ebml_element& element, double frequency){
		track -> codec.sample_rate = frequency;

		return 0;
	}

	int read_track_audio_channels(xe_ebml_element& element, ulong channels){
		track -> codec.channels = channels;

		return 0;
	}

	int read_track_audio_bit_depth(xe_ebml_element& element, ulong bit_depth){
		track -> codec.bits_per_sample = bit_depth;

		return 0;
	}

	int read_cue_point(xe_ebml_element& element){
		cue = alloc_cue();

		if(!cue)
			return XE_ENOMEM;
		return 0;
	}

	int read_cue_time(xe_ebml_element& element, ulong time){
		cue -> time = time;

		return 0;
	}

	int read_cue_track_position(xe_ebml_element& element){
		if(!cue -> alloc_track_position())
			return XE_ENOMEM;
		return 0;
	}

	int read_cue_track(xe_ebml_element& element, ulong track){
		xe_cue_track_position& position = cue -> track_positions[cue -> track_positions.size() - 1];

		position.track = track;

		return 0;
	}

	int read_cue_cluster_position(xe_ebml_element& element, ulong pos){
		xe_cue_track_position& position = cue -> track_positions[cue -> track_positions.size() - 1];

		position.position = pos + segment_offset;

		return 0;
	}

	int read_cluster_timecode(xe_ebml_element& element){
		return read_uint(element, cluster_timecode());
	}

	int read_simple_block(xe_ebml_element& element){
		int err;

		ulong track;
		short timecode;
		byte flags;
		byte lacing;

		if((err = read_vint(track)))
			return err;
		timecode = reader.r16be();
		flags = reader.r8();

		if(flags & 0x80)
			; /* keyframe */
		lacing = (flags & 0x6) >> 1;

		if(lacing){

		}else{
			sample_size() = element_left(element);
			sample_time() = cluster_timecode() + timecode;
		}

		return 0;
	}

	int skip_element(xe_ebml_element& element){
		return reader.skip(element.offset + element.size - reader.offset());
	}

	ulong& duration();
	ulong& cluster_timecode();
	ulong& sample_time();
	ulong& sample_size();
	ulong& timecode_scale();

	xe_matroska_track* alloc_track();
	xe_seek* alloc_seek();
	xe_matroska_cue* alloc_cue();

	static xe_cstr class_name(){
		return "xe_matroska_reader";
	}
};

class xe_matroska : public xe_demuxer{
public:
	ulong sample_size;
	ulong sample_time;
	ulong cluster_timecode;
	xe_ebml_element stack[16];
	uint depth;

	ulong duration;
	ulong timecode_scale;

	xe_vector<xe_matroska_track*> tracks;
	xe_vector<xe_seek> seek_head;
	xe_vector<xe_matroska_cue> cues;

	xe_matroska_reader mkv_reader;

	xe_matroska(xe_format::xe_context& context);

	xe_matroska_track* alloc_track(){
		xe_matroska_track* track = xe_zalloc<xe_matroska_track>(); // TODO default values

		if(!track)
			return null;
		if(tracks.push_back(track))
			return track;
		xe_dealloc(track);

		return null;
	}

	xe_seek* alloc_seek(){
		size_t size = seek_head.size();

		if(!seek_head.grow(size + 1))
			return null;
		seek_head.resize(size + 1);

		xe_zero(&seek_head[size]);

		return &seek_head[size];
	}

	xe_matroska_cue* alloc_cue(){
		size_t size = cues.size();

		if(!cues.grow(size + 1))
			return null;
		cues.resize(size + 1);

		xe_zero(&cues[size]);

		return &cues[size];
	}

	int open();

	int seek(uint stream, ulong pos){
		return 0;
	}

	int read_packet(xe_packet& packet);

	void reset(){

	}

	~xe_matroska(){}
};

ulong& xe_matroska_reader::duration(){
	return matroska.duration;
}

ulong& xe_matroska_reader::cluster_timecode(){
	return matroska.cluster_timecode;
}

ulong& xe_matroska_reader::sample_time(){
	return matroska.sample_time;
}

ulong& xe_matroska_reader::sample_size(){
	return matroska.sample_size;
}

ulong& xe_matroska_reader::timecode_scale(){
	return matroska.timecode_scale;
}

xe_matroska_track* xe_matroska_reader::alloc_track(){
	return matroska.alloc_track();
}

xe_seek* xe_matroska_reader::alloc_seek(){
	return matroska.alloc_seek();

}
xe_matroska_cue* xe_matroska_reader::alloc_cue(){
	return matroska.alloc_cue();
}

xe_matroska::xe_matroska(xe_format::xe_context& context):
	xe_demuxer(context),
	mkv_reader(*this, context.reader){
}

int xe_matroska::open(){
	int err = mkv_reader.read_children();

	if(err)
		return err;
	if(!context -> tracks.resize(tracks.size()))
		return XE_ENOMEM;
	for(uint i = 0; i < tracks.size(); i++)
		context -> tracks[i] = tracks[i];
	return 0;
}

int xe_matroska::read_packet(xe_packet& packet){
	xe_reader& reader = context -> reader;

	if(!sample_size){
		int err = mkv_reader.read_children();

		if(err)
			return err;
		if(!sample_size)
			return XE_EOF;
	}

	if(!alloc_packet(packet, sample_size))
		return XE_ENOMEM;
	packet.timestamp = sample_time;
	reader.read(packet.data(), sample_size);
	sample_size = 0;

	return reader.error();
}

xe_demuxer* xe_matroska_class::create(xe_format::xe_context& context) const{
	return xe_znew<xe_matroska>(context);
}

bool xe_matroska_class::probe(xe_reader& reader) const{
	xe_matroska_reader mkv(*(xe_matroska*)null, reader);
	xe_matroska_id id;

	if(mkv.read_id(id) || id != EBML_HEADER)
		return false;
	return true;
}