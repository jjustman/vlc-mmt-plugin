/*
 * mmtp_types.h
 *
 *  Created on: Jan 3, 2019
 *      Author: jjustman
 */

#ifndef MODULES_DEMUX_MMT_MMTP_TYPES_H_
#define MODULES_DEMUX_MMT_MMTP_TYPES_H_

#include <vlc_common.h>
#include <vlc_vector.h>

#include <assert.h>
#include <limits.h>

#include "libmp4.h"
#include "mp4.h"

//logging hack to quiet output....
#define __LOG_INFO(...)
#define __LOG_INFO2(...) (msg_Info(__VA_ARGS__))

#define __LOG_DEBUG(...)
#define __PRINTF_DEBUG(...)

/**
 *
 * these sizes aren't bit-aligned to the 23008-1 spec, but they are right-shifted to LSB values
 *
 */

/**
 * base mmtp_packet_header fields
 *
 * clang doesn't know how to inherit from structs, e.g. -fms-extensions, so use a define instead
 * see https://stackoverflow.com/questions/1114349/struct-inheritance-in-c
 *
 * typedef struct mmtp_packet_header {
 *
 * todo, ptr back to chain to mmtp_packet_id
 */


#define _MMTP_PACKET_HEADER_FIELDS 			\
	block_t *raw_packet;					\
	struct mmtp_sub_flow_t *mmtp_sub_flow;	\
	uint8_t mmtp_packet_version; 			\
	uint8_t packet_counter_flag; 			\
	uint8_t fec_type; 						\
	uint8_t mmtp_payload_type;				\
	uint8_t mmtp_header_extension_flag;		\
	uint8_t mmtp_rap_flag;					\
	uint8_t mmtp_qos_flag;					\
	uint8_t mmtp_flow_identifer_flag;		\
	uint8_t mmtp_flow_extension_flag;		\
	uint8_t mmtp_header_compression;		\
	uint8_t	mmtp_indicator_ref_header_flag;	\
	uint8_t mmtp_type_of_bitrate;			\
	uint8_t mmtp_delay_sensitivity;			\
	uint8_t mmtp_transmission_priority;		\
	uint8_t flow_label;						\
	uint16_t mmtp_header_extension_type;	\
	uint16_t mmtp_header_extension_length;	\
	uint8_t *mmtp_header_extension_value;	\
	uint16_t mmtp_packet_id; 				\
	uint32_t mmtp_timestamp;				\
	uint32_t packet_sequence_number;		\
	uint32_t packet_counter;				\

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;
} __mmtp_packet_header_fields_t;

//define for mpu type common header fields for struct inheritance
//todo: add in MMTHSample box

#define _MMTP_MPU_TYPE_PACKET_HEADER_FIELDS \
	_MMTP_PACKET_HEADER_FIELDS;				\
	uint16_t mpu_payload_length;			\
	uint8_t mpu_fragment_type;				\
	uint8_t mpu_timed_flag;					\
	uint8_t mpu_fragmentation_indicator;	\
	uint8_t mpu_aggregation_flag;			\
	uint8_t mpu_fragmentation_counter;		\
	uint32_t mpu_sequence_number;			\
	uint16_t data_unit_length;				\
	block_t *mpu_data_unit_payload;			\

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_MPU_TYPE_PACKET_HEADER_FIELDS;
} __mmtp_mpu_type_packet_header_fields_t;

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_MPU_TYPE_PACKET_HEADER_FIELDS;
	uint32_t movie_fragment_sequence_number;
	uint32_t sample_number;
	uint32_t offset;
	uint8_t priority;
	uint8_t dep_counter;
} __mpu_data_unit_payload_fragments_timed_t;

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_MPU_TYPE_PACKET_HEADER_FIELDS;
	uint32_t non_timed_mfu_item_id;

} __mpu_data_unit_payload_fragments_nontimed_t;

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;

} __generic_object_fragments_t;

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;

} __signalling_message_fragments_t;

//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY
typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;

} __repair_symbol_t;
//DO NOT REFERENCE INTEREMDIATE STRUCTS DIRECTLY


//YOU CAN REFERENCE mmtp_payload_fragments_union_t* ONLY
//todo - convert this to discriminated union
typedef union  {
	__mmtp_packet_header_fields_t					mmtp_packet_header;
	__mmtp_mpu_type_packet_header_fields_t			mmtp_mpu_type_packet_header;

	__mpu_data_unit_payload_fragments_timed_t 		mpu_data_unit_payload_fragments_timed;
	__mpu_data_unit_payload_fragments_nontimed_t	mpu_data_unit_payload_fragments_nontimed;

	//add in the other mmtp types here
	__generic_object_fragments_t 					mmtp_generic_object_fragments;
	__signalling_message_fragments_t				mmtp_signalling_message_fragments;
	__repair_symbol_t								mmtp_repair_symbol;
} mmtp_payload_fragments_union_t;

typedef struct VLC_VECTOR(union mmtp_payload_fragments_union_t *) 	mpu_type_packet_header_fields_vector_t;
typedef struct VLC_VECTOR(union mmtp_payload_fragments_union_t *) 	mpu_data_unit_payload_fragments_timed_vector_t;
typedef struct VLC_VECTOR(union mmtp_payload_fragments_union_t *)	mpu_data_unit_payload_fragments_nontimed_vector_t;
typedef struct VLC_VECTOR(union mmtp_payload_fragments_union_t *) 	mmtp_generic_object_fragments_vector_t;
typedef struct VLC_VECTOR(union mmtp_payload_fragments_union_t *) 	mmtp_signalling_message_fragments_vector_t;
typedef struct VLC_VECTOR(union mmtp_payload_fragments_union_t *) 	mmtp_repair_symbol_vector_t;

//todo, make this union
typedef struct {
	uint32_t mpu_sequence_number;
	mpu_data_unit_payload_fragments_timed_vector_t 		timed_fragments_vector;
	mpu_data_unit_payload_fragments_nontimed_vector_t 	nontimed_fragments_vector;

} mpu_data_unit_payload_fragments_t;

typedef struct VLC_VECTOR(mpu_data_unit_payload_fragments_t *) mpu_data_unit_payload_fragments_vector_t;


typedef struct {
	MP4_Box_t*		mpu_fragments_p_root_box;
	MP4_Box_t*		mpu_fragments_p_moov;
	mp4_track_t*	mpu_demux_track;
	block_t*		p_mpu_block;
	uint32_t     	i_timescale;          /* movie time scale */
	uint64_t     	i_moov_duration;
	uint64_t     	i_cumulated_duration; /* Same as above, but not from probing, (movie time scale) */
	uint64_t     	i_duration;           /* Declared fragmented duration (movie time scale) */
	unsigned int 	i_tracks;       /* number of tracks */
	mp4_track_t  	*track;         /* array of track */
	bool        	b_fragmented;   /* fMP4 */
	bool         	b_seekable;
	stream_t 		*s_frag;

	struct
	{
		 uint32_t        i_current_box_type;
		 MP4_Box_t      *p_fragment_atom;
		 uint64_t        i_post_mdat_offset;
		 uint32_t        i_lastseqnumber;
	} context;
} mpu_isobmff_fragment_parameters_t;

typedef struct {
	struct mmtp_sub_flow_t *mmtp_sub_flow;
	uint16_t mmtp_packet_id;

	mpu_type_packet_header_fields_vector_t 		all_mpu_fragments_vector;

	//MPU Fragment type collections for reconstruction/recovery of fragments

	//MPU metadata, 							mpu_fragment_type==0x00
	mpu_data_unit_payload_fragments_vector_t 	mpu_metadata_fragments_vector;

	//Movie fragment metadata, 					mpu_fragment_type==0x01
	mpu_data_unit_payload_fragments_vector_t	mpu_movie_fragment_metadata_vector;

	//MPU (media fragment_unit),				mpu_fragment_type==0x02
	mpu_data_unit_payload_fragments_vector_t	media_fragment_unit_vector;

	mpu_isobmff_fragment_parameters_t			mpu_isobmff_fragment_parameters;

} mpu_fragments_t;

/**
 * todo:  impl's
 */


//todo - refactor mpu_fragments to vector, create a new tuple class for mmtp_sub_flow_sequence

typedef struct  {
	uint16_t mmtp_packet_id;

	//mmtp payload type collections for reconstruction/recovery of payload types

	//mpu (media_processing_unit):				paylod_type==0x00
	//mpu_fragments_vector_t 					mpu_fragments_vector;
	mpu_fragments_t								*mpu_fragments;

	//generic object:							payload_type==0x01
    mmtp_generic_object_fragments_vector_t 		mmtp_generic_object_fragments_vector;

	//signalling message: 						payload_type=0x02
	mmtp_signalling_message_fragments_vector_t 	mmtp_signalling_message_fragements_vector;

	//repair symbol:							payload_type==0x03
	mmtp_repair_symbol_vector_t 				mmtp_repair_symbol_vector;

} mmtp_sub_flow_t;


typedef struct VLC_VECTOR(mmtp_sub_flow_t*) mmtp_sub_flow_vector_t;


void mmtp_sub_flow_vector_init(mmtp_sub_flow_vector_t *mmtp_sub_flow_vector) {

	__PRINTF_DEBUG("%d:mmtp_sub_flow_vector_init: %p\n", __LINE__, mmtp_sub_flow_vector);

	vlc_vector_init(mmtp_sub_flow_vector);
	__PRINTF_DEBUG("%d:mmtp_sub_flow_vector_init: %p\n", __LINE__, mmtp_sub_flow_vector);
}
/**

static struct vlc_player_program *
vlc_player_program_vector_FindById(vlc_player_program_vector *vec, int id,
                                   size_t *idx)
{
    for (size_t i = 0; i < vec->size; ++i)
    {
        struct vlc_player_program *prgm = vec->data[i];
        if (prgm->group_id == id)
        {
            if (idx)
                *idx = i;
            return prgm;
        }
    }
    return NULL;
}
**/



mpu_data_unit_payload_fragments_t* mpu_data_unit_payload_fragments_find_mpu_sequence_number(mpu_data_unit_payload_fragments_vector_t *vec, uint32_t mpu_sequence_number) {
	for (size_t i = 0; i < vec->size; ++i) {
		mpu_data_unit_payload_fragments_t *mpu_fragments = vec->data[i];

		if (mpu_fragments->mpu_sequence_number == mpu_sequence_number) {
			return vec->data[i];
		}
	}
	return NULL;
}


mpu_fragments_t* mpu_data_unit_payload_fragments_get_or_set_mpu_sequence_number_from_packet(mpu_data_unit_payload_fragments_vector_t *vec, mmtp_payload_fragments_union_t *mpu_type_packet) {

	mpu_data_unit_payload_fragments_t *entry = mpu_data_unit_payload_fragments_find_mpu_sequence_number(vec, mpu_type_packet->mmtp_mpu_type_packet_header.mpu_sequence_number);
	if(!entry) {
		entry = calloc(1, sizeof(mpu_data_unit_payload_fragments_t));

		entry->mpu_sequence_number = mpu_type_packet->mmtp_mpu_type_packet_header.mpu_sequence_number;
		vlc_vector_init(&entry->timed_fragments_vector);
		vlc_vector_init(&entry->nontimed_fragments_vector);
		if(mpu_type_packet->mmtp_mpu_type_packet_header.mpu_timed_flag) {
			vlc_vector_push(&entry->timed_fragments_vector, entry);
		} else {
			vlc_vector_push(&entry->nontimed_fragments_vector, entry);
		}
	}

	return entry;
}



void allocate_mmtp_sub_flow_mpu_fragments(mmtp_sub_flow_t* entry) {
	entry->mpu_fragments = calloc(1, sizeof(mpu_fragments_t));
	entry->mpu_fragments->mmtp_sub_flow = entry;

	vlc_vector_init(&entry->mpu_fragments->all_mpu_fragments_vector);
	vlc_vector_init(&entry->mpu_fragments->mpu_metadata_fragments_vector);
	vlc_vector_init(&entry->mpu_fragments->mpu_movie_fragment_metadata_vector);
	vlc_vector_init(&entry->mpu_fragments->media_fragment_unit_vector);
}

//push this to mpu_fragments_vector->all_fragments_vector first,
// 	then re-assign once fragment_type and fragmentation info are parsed
//mpu_sequence_number *SHOULD* only be resolved from the interior all_fragments_vector for tuple lookup
mpu_fragments_t* mpu_fragments_get_or_set_packet_id(mmtp_sub_flow_t* mmtp_sub_flow, uint16_t mmtp_packet_id) {

	mpu_fragments_t *entry = mmtp_sub_flow->mpu_fragments;
	if(!entry) {
		__PRINTF_DEBUG("*** %d:mpu_fragments_get_or_set_packet_id - adding vector: %p, all_fragments_vector is: %p\n",
				__LINE__, entry, entry->all_mpu_fragments_vector);

		allocate_mmtp_sub_flow_mpu_fragments(mmtp_sub_flow);
	}

	return entry;
}

void mpu_fragments_assign_to_payload_vector(mmtp_sub_flow_t *mmtp_sub_flow, mmtp_payload_fragments_union_t *mpu_type_packet) {
	//use mmtp_sub_flow ref, find packet_id, map into mpu/mfu vector
//	mmtp_sub_flow_t mmtp_sub_flow = mpu_type_packet->mpu_

	mpu_fragments_t *mpu_fragments = mmtp_sub_flow->mpu_fragments;
	__PRINTF_DEBUG("%d:mpu_fragments_assign_to_payload_vector - mpu_fragments is: %p\n", __LINE__, mpu_fragments);

	if(mpu_type_packet->mmtp_mpu_type_packet_header.mpu_fragment_type == 0x00) {
		//push to mpu_metadata fragments vector
		mpu_data_unit_payload_fragments_get_or_set_mpu_sequence_number_from_packet(&mpu_fragments->mpu_metadata_fragments_vector, mpu_type_packet);
	} else if(mpu_type_packet->mmtp_mpu_type_packet_header.mpu_fragment_type == 0x01) {
		//push to mpu_movie_fragment
		mpu_data_unit_payload_fragments_get_or_set_mpu_sequence_number_from_packet(&mpu_fragments->mpu_movie_fragment_metadata_vector, mpu_type_packet);
	} else if(mpu_type_packet->mmtp_mpu_type_packet_header.mpu_fragment_type == 0x02) {
		//push to media_fragment
		mpu_data_unit_payload_fragments_get_or_set_mpu_sequence_number_from_packet(&mpu_fragments->media_fragment_unit_vector, mpu_type_packet);

	}
}



mmtp_sub_flow_t* mmtp_sub_flow_vector_find_packet_id(mmtp_sub_flow_vector_t *vec, uint16_t mmtp_packet_id) {
	for (size_t i = 0; i < vec->size; ++i) {
		mmtp_sub_flow_t *mmtp_sub_flow = vec->data[i];

		if (mmtp_sub_flow->mmtp_packet_id == mmtp_packet_id) {
			return mmtp_sub_flow;
		}
	}
	return NULL;
}

mpu_fragments_t* mpu_fragments_find_packet_id(mmtp_sub_flow_vector_t *vec, uint16_t mmtp_packet_id) {
	mmtp_sub_flow_t *entry = mmtp_sub_flow_vector_find_packet_id(vec, mmtp_packet_id);
	if(entry) {
		return entry->mpu_fragments;
	}

	return NULL;
}

mmtp_sub_flow_t* mmtp_sub_flow_vector_get_or_set_packet_id(mmtp_sub_flow_vector_t *vec, uint16_t mmtp_packet_id) {

	mmtp_sub_flow_t *entry = mmtp_sub_flow_vector_find_packet_id(vec, mmtp_packet_id);

	if(!entry) {
		entry = calloc(1, sizeof(mmtp_sub_flow_t));
		entry->mmtp_packet_id = mmtp_packet_id;
		allocate_mmtp_sub_flow_mpu_fragments(entry);
		vlc_vector_init(&entry->mmtp_generic_object_fragments_vector);
		vlc_vector_init(&entry->mmtp_signalling_message_fragements_vector);
		vlc_vector_init(&entry->mmtp_repair_symbol_vector);

		vlc_vector_push(vec, entry);
	}

	return entry;
}



//TODO, build factory parser here

mmtp_payload_fragments_union_t* mmtp_packet_header_allocate_from_raw_packet(block_t *raw_packet) {
	mmtp_payload_fragments_union_t *entry = NULL;

	//pick the larger of the timed vs. non-timed fragment struct sizes
	entry = calloc(1, sizeof(mmtp_payload_fragments_union_t));

	if(!entry) {
		abort();
	}
	entry->mmtp_packet_header.raw_packet = raw_packet;

	return entry;
}

//think of this as castable to the base fields as they are the same size layouts
mmtp_payload_fragments_union_t* mmtp_packet_create(block_t * raw_packet,
												uint8_t mmtp_packet_version,
												uint8_t mmtp_payload_type,
												uint16_t mmtp_packet_id,
												uint32_t packet_sequence_number,
												uint32_t packet_counter,
												uint32_t mmtp_timestamp) {
	mmtp_payload_fragments_union_t *entry = NULL;

	//pick the larger of the timed vs. non-timed fragment struct sizes
	entry = calloc(1, sizeof(mmtp_payload_fragments_union_t));

	if(!entry) {
		abort();
	}

	entry->mmtp_packet_header.raw_packet = raw_packet;
	entry->mmtp_packet_header.mmtp_packet_version = mmtp_packet_version;
	entry->mmtp_packet_header.mmtp_payload_type = mmtp_payload_type;
	entry->mmtp_packet_header.mmtp_packet_id = mmtp_packet_id;
	entry->mmtp_packet_header.packet_sequence_number = packet_sequence_number;
	entry->mmtp_packet_header.packet_counter = packet_counter;
	entry->mmtp_packet_header.mmtp_timestamp = mmtp_timestamp;

	return entry;
}

void mmtp_sub_flow_push_mmtp_packet(mmtp_sub_flow_t *mmtp_sub_flow, mmtp_payload_fragments_union_t *mmtp_packet) {
	mmtp_packet->mmtp_packet_header.mmtp_sub_flow = mmtp_sub_flow;

	__PRINTF_DEBUG("%d:mmtp_sub_flow_push_mmtp_packet, mmtp_payload_type: 0x%x\n", __LINE__, mmtp_packet->mmtp_packet_header.mmtp_payload_type);
	if(mmtp_packet->mmtp_packet_header.mmtp_payload_type == 0x00) {
		//(mmtp_mpu_type_packet_header_fields_t*
		//defer, we don;'t know enough about the type
		mpu_fragments_t *mpu_fragments = mpu_fragments_get_or_set_packet_id(mmtp_sub_flow, mmtp_packet->mmtp_packet_header.mmtp_packet_id);
		vlc_vector_push(&mpu_fragments->all_mpu_fragments_vector, mmtp_packet);

	} else if(mmtp_packet->mmtp_packet_header.mmtp_payload_type == 0x01) {
		vlc_vector_push(&mmtp_sub_flow->mmtp_generic_object_fragments_vector, mmtp_packet);
	} else if(mmtp_packet->mmtp_packet_header.mmtp_payload_type == 0x02) {
		vlc_vector_push(&mmtp_sub_flow->mmtp_signalling_message_fragements_vector, mmtp_packet);
	} else if(mmtp_packet->mmtp_packet_header.mmtp_payload_type == 0x03) {
		vlc_vector_push(&mmtp_sub_flow->mmtp_repair_symbol_vector, mmtp_packet);
	}
}

//todo #define's
//	msg_Dbg( p_demux, "packet version: %d, payload_type: 0x%X, packet_id 0x%hu, timestamp: 0x%X, packet_sequence_number: 0x%X, packet_counter: 0x%X", mmtp_packet_version,
//			mmtp_payload_type, mmtp_packet_id, mmtp_timestamp, packet_sequence_number, packet_counter);


/**
 * legacy p_sys
 * TODO - clean me up
 */


typedef struct
{
	vlc_object_t *obj;
	//hack for cross-parsing
	uint8_t *raw_buf;
	uint8_t *buf;

	//reconsititue mfu's into a p_out_muxed fifo

	block_t *p_mpu_block;

	//everthing below here is from libmp4

    MP4_Box_t    *p_root;      /* container for the whole file */
    MP4_Box_t	 *p_moov;

    vlc_tick_t   i_pcr;

    uint64_t     i_moov_duration;
    uint64_t     i_duration;           /* Declared fragmented duration (movie time scale) */
    uint64_t     i_cumulated_duration; /* Same as above, but not from probing, (movie time scale) */
    uint32_t     i_timescale;          /* movie time scale */
    vlc_tick_t   i_nztime;             /* time position of the presentation (CLOCK_FREQ timescale) */
    unsigned int i_tracks;       /* number of tracks */
    mp4_track_t  *track;         /* array of track */
    float        f_fps;          /* number of frame per seconds */

    bool         b_fragmented;   /* fMP4 */
    bool         b_seekable;
    bool         b_fastseekable;
    bool         b_error;        /* unrecoverable */

    bool            b_index_probed;     /* mFra sync points index */
    bool            b_fragments_probed; /* moof segments index created */


    struct
    {
        uint32_t        i_current_box_type;
        MP4_Box_t      *p_fragment_atom;
        uint64_t        i_post_mdat_offset;
        uint32_t        i_lastseqnumber;
    } context;

    /* */
    MP4_Box_t    *p_tref_chap;

    /* */
    bool seekpoint_changed;
    int          i_seekpoint;
    vlc_meta_t    *p_meta;

    /* ASF in MP4 */
    asf_packet_sys_t asfpacketsys;
    vlc_tick_t i_preroll;       /* foobar */
    vlc_tick_t i_preroll_start;

    struct
    {
        int es_cat_filters;
    } hacks;

    mp4_fragments_index_t *p_fragsindex;

    sig_atomic_t has_processed_ftype_moov;

    /** temp hacks until we have a map of mpu_sequence_numbers, use -1 for default values (0 is valid in mmtp spec)**/
    sig_atomic_t last_mpu_sequence_number;
    sig_atomic_t last_mpu_fragment_type;


    mmtp_sub_flow_vector_t mmtp_sub_flow_vector;

} demux_sys_t;

/**
 *
 * mmtp packet parsing
 */


void* extract(uint8_t *bufPosPtr, uint8_t *dest, int size) {
	for(int i=0; i < size; i++) {
		dest[i] = *bufPosPtr++;
	}
	return bufPosPtr;
}


#define MIN_MMTP_SIZE 32
#define MAX_MMTP_SIZE 1514


int mmtp_packet_header_parse_from_raw_packet(mmtp_payload_fragments_union_t *mmtp_packet, demux_t *p_demux ) {

	demux_sys_t *p_sys = p_demux->p_sys;

	//hack for joint parsing.....
	p_sys->raw_buf = malloc( MAX_MMTP_SIZE );
	p_sys->buf = p_sys->raw_buf; //use buf to walk thru bytes in extract method without touching rawBuf

	uint8_t *raw_buf = p_sys->raw_buf;
	uint8_t *buf = p_sys->buf;

	block_ChainExtract(mmtp_packet->mmtp_packet_header.raw_packet, raw_buf, MAX_MMTP_SIZE);

	uint8_t mmtp_packet_preamble[20];

	//msg_Warn( p_demux, "buf pos before extract is: %p", (void *)buf);
	buf = extract(buf, mmtp_packet_preamble, 20);
	//msg_Warn( p_demux, "buf pos is now %p vs %p", new_buf_pos, (void *)buf);

//		msg_Dbg( p_demux, "raw packet size is: %d, first byte: 0x%X", mmtp_raw_packet_size, mmtp_packet_preamble[0]);

//	if(false) {
//		char buffer[(mmtp_raw_packet_size * 3)+1];
//
//		for(int i=0; i < mmtp_raw_packet_size; i++) {
//			if(i>1 && (i+1)%8 == 0) {
//				sn__PRINTF_DEBUG(buffer + (i*3), 4, "%02X\n", raw_buf[i]);
//			} else {
//				sn__PRINTF_DEBUG(buffer + (i*3), 4, "%02X ", raw_buf[i]);
//			}
//		}
//		msg_Info(p_demux, "raw packet payload is:\n%s", buffer);
//	}

	mmtp_packet->mmtp_packet_header.mmtp_packet_version = (mmtp_packet_preamble[0] & 0xC0) >> 6;
	mmtp_packet->mmtp_packet_header.packet_counter_flag = (mmtp_packet_preamble[0] & 0x20) >> 5;
	mmtp_packet->mmtp_packet_header.fec_type = (mmtp_packet_preamble[0] & 0x18) >> 3;

	//v=0 vs v=1 attributes in the first 2 octets
	uint8_t  mmtp_payload_type = 0;
	uint8_t  mmtp_header_extension_flag = 0;
	uint8_t  mmtp_rap_flag = 0;
	uint8_t  mmtp_qos_flag = 0;

	uint8_t  mmtp_flow_identifer_flag = 0;
	uint8_t  mmtp_flow_extension_flag = 0;

	uint8_t  mmtp_header_compression = 0;
	uint8_t	 mmtp_indicator_ref_header_flag = 0;

	uint8_t mmtp_type_of_bitrate = 0;
	uint8_t mmtp_delay_sensitivity = 0;
	uint8_t mmtp_transmission_priority = 0;

	uint16_t mmtp_header_extension_type = 0;
	uint16_t mmtp_header_extension_length = 0;

	if(mmtp_packet->mmtp_packet_header.mmtp_packet_version == 0x00) {
		//after fec_type, with v=0, next bitmask is 0x4 >>2
		//0000 0010
		//V0CF E-XR
		mmtp_packet->mmtp_packet_header.mmtp_header_extension_flag = (mmtp_packet_preamble[0] & 0x2) >> 1;
		mmtp_packet->mmtp_packet_header.mmtp_rap_flag = mmtp_packet_preamble[0] & 0x1;

		//6 bits right aligned
		mmtp_packet->mmtp_packet_header.mmtp_payload_type = mmtp_packet_preamble[1] & 0x3f;
		if(mmtp_packet->mmtp_packet_header.mmtp_header_extension_flag & 0x1) {
			mmtp_packet->mmtp_packet_header.mmtp_header_extension_type = (mmtp_packet_preamble[16]) << 8 | mmtp_packet_preamble[17];
			mmtp_packet->mmtp_packet_header.mmtp_header_extension_length = (mmtp_packet_preamble[18]) << 8 | mmtp_packet_preamble[19];
		} else {
			//walk back by 4 bytes
			buf-=4;
		}
	} else if(mmtp_packet->mmtp_packet_header.mmtp_packet_version == 0x01) {
		//bitmask is 0000 00
		//0000 0100
		//V1CF EXRQ
		mmtp_packet->mmtp_packet_header.mmtp_header_extension_flag = mmtp_packet_preamble[0] & 0x4 >> 2; //X
		mmtp_packet->mmtp_packet_header.mmtp_rap_flag = (mmtp_packet_preamble[0] & 0x2) >> 1;				//RAP
		mmtp_packet->mmtp_packet_header.mmtp_qos_flag = mmtp_packet_preamble[0] & 0x1;					//QOS
		//0000 0000
		//FEBI TYPE
		//4 bits for preamble right aligned

		mmtp_packet->mmtp_packet_header.mmtp_flow_identifer_flag = ((mmtp_packet_preamble[1]) & 0x80) >> 7;			//F
		mmtp_packet->mmtp_packet_header.mmtp_flow_extension_flag = ((mmtp_packet_preamble[1]) & 0x40) >> 6;			//E
		mmtp_packet->mmtp_packet_header.mmtp_header_compression = ((mmtp_packet_preamble[1]) &  0x20) >> 5; 		//B
		mmtp_packet->mmtp_packet_header.mmtp_indicator_ref_header_flag = ((mmtp_packet_preamble[1]) & 0x10) >> 4;	//I

		mmtp_packet->mmtp_packet_header.mmtp_payload_type = mmtp_packet_preamble[1] & 0xF;

		//TB 2 bits
		mmtp_packet->mmtp_packet_header.mmtp_type_of_bitrate = ((mmtp_packet_preamble[16] & 0x40) >> 6) | ((mmtp_packet_preamble[16] & 0x20) >> 5);

		//DS 3 bits
		mmtp_packet->mmtp_packet_header.mmtp_delay_sensitivity = ((mmtp_packet_preamble[16] & 0x10) >> 4) | ((mmtp_packet_preamble[16] & 0x8) >> 3) | ((mmtp_packet_preamble[16] & 0x4) >> 2);

		//TP 3 bits
		mmtp_packet->mmtp_packet_header.mmtp_transmission_priority =(( mmtp_packet_preamble[16] & 0x02) << 2) | ((mmtp_packet_preamble[16] & 0x1) << 1) | ((mmtp_packet_preamble[17] & 0x80) >>7);

		mmtp_packet->mmtp_packet_header.flow_label = mmtp_packet_preamble[17] & 0x7f;

		//header extension is offset by 2 bytes in v=1, so an additional block chain read is needed to get extension length
		if(mmtp_packet->mmtp_packet_header.mmtp_header_extension_flag & 0x1) {
			mmtp_packet->mmtp_packet_header.mmtp_header_extension_type = (mmtp_packet_preamble[18] << 8) | mmtp_packet_preamble[19];

	//		msg_Warn( p_demux, "mmtp_demuxer - dping mmtp_header_extension_length_bytes: %d",  mmtp_header_extension_type);

			uint8_t mmtp_header_extension_length_bytes[2];
			buf = extract(buf, mmtp_header_extension_length_bytes, 2);

			mmtp_packet->mmtp_packet_header.mmtp_header_extension_length = mmtp_header_extension_length_bytes[0] << 8 | mmtp_header_extension_length_bytes[1];
		} else {
			//walk back our buf position by 2 bytes to start for payload ddata
			buf-=2;

		}
	} else {
		msg_Warn( p_demux, "mmtp_demuxer - unknown packet version of 0x%X", mmtp_packet->mmtp_packet_header.mmtp_packet_version);
		goto error;
	//	free( raw_buf );

	}

	mmtp_packet->mmtp_packet_header.mmtp_packet_id			= mmtp_packet_preamble[2]  << 8  | mmtp_packet_preamble[3];
	mmtp_packet->mmtp_packet_header.mmtp_timestamp 			= mmtp_packet_preamble[4]  << 24 | mmtp_packet_preamble[5]  << 16 | mmtp_packet_preamble[6]   << 8 | mmtp_packet_preamble[7];
	mmtp_packet->mmtp_packet_header.packet_sequence_number	= mmtp_packet_preamble[8]  << 24 | mmtp_packet_preamble[9]  << 16 | mmtp_packet_preamble[10]  << 8 | mmtp_packet_preamble[11];
	mmtp_packet->mmtp_packet_header.packet_counter 			= mmtp_packet_preamble[12] << 24 | mmtp_packet_preamble[13] << 16 | mmtp_packet_preamble[14]  << 8 | mmtp_packet_preamble[15];


	p_sys->raw_buf = raw_buf;
	p_sys->buf =  buf;

	return VLC_DEMUXER_SUCCESS;

error:

	return VLC_DEMUXER_EGENERIC;



}


#endif /* MODULES_DEMUX_MMT_MMTP_TYPES_H_ */
