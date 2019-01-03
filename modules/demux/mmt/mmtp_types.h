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
	uint16_t mmtp_header_extension_type;	\
	uint16_t mmtp_header_extension_length;	\
	uint16_t mmtp_packet_id; 				\
	uint32_t mmtp_timestamp;				\
	uint32_t packet_sequence_number;		\
	uint32_t packet_counter;				\
	block_t *raw_packet;					\

typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;
} mmtp_packet_header_fields_t;

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
	block_t *mpu_data_unit_payload;			\

//
//is this the right place?
typedef struct {
	_MMTP_MPU_TYPE_PACKET_HEADER_FIELDS;
} mmtp_mpu_type_packet_header_fields_t;

typedef struct {
	_MMTP_MPU_TYPE_PACKET_HEADER_FIELDS;
	uint32_t movie_fragment_sequence_number;
	uint32_t sample_number;
	uint32_t offset;
	uint8_t priority;
	uint8_t dep_counter;

} mpu_data_unit_payload_fragments_timed_t;

typedef struct {
	_MMTP_MPU_TYPE_PACKET_HEADER_FIELDS;
	uint32_t non_timed_mfu_item_id;

	//add in padding for struct alignment, 32+32+8+8=
	uint8_t __padding[10];
} mpu_data_unit_payload_fragments_nontimed_t;

typedef struct VLC_VECTOR(struct mmtp_mpu_type_packet_header_fields_t *) 		mpu_type_packet_header_fields_vector_t;

typedef struct VLC_VECTOR(struct mpu_data_unit_payload_fragments_timed_t *) 	mpu_data_unit_payload_fragments_timed_vector_t;
typedef struct VLC_VECTOR(struct mpu_data_unit_payload_fragments_nontimed_t *)	mpu_data_unit_payload_fragments_nontimed_vector_t;

//todo, make this union
typedef struct {
	mpu_data_unit_payload_fragments_timed_vector_t 		timed_fragments_vector;
	mpu_data_unit_payload_fragments_nontimed_vector_t 	nontimed_fragments_vector;

} mpu_data_unit_payload_fragments_t;

typedef struct VLC_VECTOR(struct mpu_data_unit_payload_fragments_t *) mpu_data_unit_payload_fragments_vector_t;

typedef struct {
	//struct mmtp_sub_flow_t *mmtp_sub_flow;
	uint16_t mmtp_packet_id;


	mpu_type_packet_header_fields_vector_t 		all_fragments_vector;

	//MPU Fragment type collections for reconstruction/recovery of fragments

	//MPU metadata, 							mpu_fragment_type==0x00
	mpu_data_unit_payload_fragments_vector_t 	mpu_metadata_fragments_vector;

	//Movie fragment metadata, 					mpu_fragment_type==0x01
	mpu_data_unit_payload_fragments_vector_t	mpu_movie_fragment_metadata_vector;

	//MPU (media fragment_unit),				mpu_fragment_type==0x02
	mpu_data_unit_payload_fragments_vector_t	media_fragment_unit_vector;

} mpu_fragments_t;

/**
 * todo:  impl's
 */

typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;

} generic_object_fragments_t;

typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;

} signalling_message_fragments_t;

typedef struct {
	_MMTP_PACKET_HEADER_FIELDS;

} repair_symbol_t;

typedef struct VLC_VECTOR(mpu_fragments_t *) 					mpu_fragments_vector_t;
typedef struct VLC_VECTOR(generic_object_fragments_t *) 		generic_object_fragments_vector_t;
typedef struct VLC_VECTOR(signalling_message_fragments_t *) 	signalling_message_fragments_vector_t;
typedef struct VLC_VECTOR(repair_symbol_t *) 					repair_symbol_vector_t;

typedef struct  {
	uint16_t packet_id;

	//mmtp payload type collections for reconstruction/recovery of payload types

	//mpu (media_processing_unit):			paylod_type==0x00
	mpu_fragments_vector_t 					mpu_fragments_vector;

	//generic object:						payload_type==0x01
	generic_object_fragments_vector_t 		generic_object_fragments_vector;

	//signalling message: 					payload_type=0x02
	signalling_message_fragments_vector_t 	signalling_message_fragements_vector;

	//repair symbol:						payload_type==0x03
	repair_symbol_vector_t 					repair_symbol_vector;

} mmtp_sub_flow_t;


typedef struct VLC_VECTOR(mmtp_sub_flow_t *) mmtp_sub_flow_vector_t;


void mmtp_sub_flow_vector_init(mmtp_sub_flow_vector_t *mmtp_sub_flow_vector) {
	vlc_vector_init(mmtp_sub_flow_vector);
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




mpu_fragments_t* mpu_fragments_find_packet_id(mpu_fragments_vector_t *vec, uint16_t mmtp_packet_id) {
	for (size_t i = 0; i < vec->size; ++i) {
		mpu_fragments_t *mpu_fragments = vec->data[i];

		if (mpu_fragments->mmtp_packet_id == mmtp_packet_id) {
			return vec->data[i];
		}
	}
	return NULL;
}

//push this to mpu_fragments_vector->all_fragments_vector first,
// 	then re-assign once fragment_type and fragmentation info are parsed
//mpu_sequence_number *SHOULD* only be resolved from the interior all_fragments_vector for tuple lookup
mpu_fragments_t* mpu_fragments_get_or_set_packet_id(mpu_fragments_vector_t *vec, uint16_t mmtp_packet_id) {

	mpu_fragments_t *entry = mpu_fragments_find_packet_id(vec, mmtp_packet_id);
	if(!entry) {
		entry = malloc(sizeof(mpu_fragments_t));
		entry->mmtp_packet_id = mmtp_packet_id;
		vlc_vector_init(&entry->all_fragments_vector);
		vlc_vector_init(&entry->mpu_metadata_fragments_vector);
		vlc_vector_init(&entry->mpu_movie_fragment_metadata_vector);
		vlc_vector_init(&entry->media_fragment_unit_vector);

		vlc_vector_push(vec, entry);
	}

	return entry;
}



mmtp_sub_flow_t* mmtp_sub_flow_vector_find_packet_id(mmtp_sub_flow_vector_t *vec, uint16_t packet_id) {
	for (size_t i = 0; i < vec->size; ++i) {
		mmtp_sub_flow_t *mmtp_sub_flow = vec->data[i];

		if (mmtp_sub_flow->packet_id == packet_id) {
			return vec->data[i];
		}
	}
	return NULL;
}

mmtp_sub_flow_t* mmtp_sub_flow_vector_get_or_set_packet_id(mmtp_sub_flow_vector_t *vec, uint16_t packet_id) {

	mmtp_sub_flow_t *entry = mmtp_sub_flow_vector_find_packet_id(vec, packet_id);

	if(!entry) {
		entry = malloc(sizeof(mmtp_sub_flow_t));
		entry->packet_id = packet_id;
		vlc_vector_init(&entry->mpu_fragments_vector);
		vlc_vector_init(&entry->generic_object_fragments_vector);
		vlc_vector_init(&entry->signalling_message_fragements_vector);
		vlc_vector_init(&entry->repair_symbol_vector);
		vlc_vector_push(vec, entry);
	}

	return entry;
}


//TODO, build factory parser here
//think of this as castable to the base fields as they are the same size layouts
mmtp_packet_header_fields_t* mmtp_packet_create(block_t * raw_packet,
												uint8_t mmtp_packet_version,
												uint8_t mmtp_payload_type,
												uint16_t mmtp_packet_id,
												uint32_t packet_sequence_number,
												uint32_t packet_counter,
												uint32_t mmtp_timestamp) {
	mmtp_packet_header_fields_t *entry = NULL;

	//pick the larger of the timed vs. non-timed fragment struct sizes

	if(mmtp_packet_version == 0x00) {
		entry = malloc(__MAX(sizeof(mpu_data_unit_payload_fragments_timed_t), sizeof(mpu_data_unit_payload_fragments_nontimed_t)));
	} else if(mmtp_packet_version == 0x01) {
		entry = malloc(sizeof(generic_object_fragments_t));
	} else if(mmtp_packet_version == 0x02) {
		entry = malloc(sizeof(signalling_message_fragments_t));
	} else if(mmtp_packet_version == 0x03) {
		entry = malloc(sizeof(repair_symbol_t));
	}

	//bootstrap key values here
	if(entry) {
		entry->raw_packet = raw_packet;
		entry->mmtp_packet_version = mmtp_packet_version;
		entry->mmtp_payload_type = mmtp_payload_type;
		entry->mmtp_packet_id = mmtp_packet_id;
		entry->packet_sequence_number = packet_sequence_number;
		entry->packet_counter = packet_counter;
		entry->mmtp_timestamp = mmtp_timestamp;
	}

	return entry;
}

void mmtp_sub_flow_vector_push_mmtp_packet(mmtp_sub_flow_t *sub_flow_vec, mmtp_packet_header_fields_t *entry) {
	if(entry->mmtp_packet_version == 0x00) {
		/**
		 *
		 * 		mpu_fragments_t *mpu_fragments = mpu_fragments_get_or_set_mpu_sequence_number(&sub_flow_vec->mpu_fragments_vector, entry->mmtp_);
		//todo - peek for fragment_type
		 *
		 */

		mpu_fragments_t *mpu_fragments = mpu_fragments_get_or_set_packet_id(&sub_flow_vec->mpu_fragments_vector, entry->mmtp_packet_id);

		vlc_vector_push(&mpu_fragments->all_fragments_vector, (mmtp_mpu_type_packet_header_fields_t*)entry);
	} else if(entry->mmtp_packet_version == 0x01) {
		vlc_vector_push(&sub_flow_vec->generic_object_fragments_vector, entry);
	} else if(entry->mmtp_packet_version == 0x02) {
		vlc_vector_push(&sub_flow_vec->signalling_message_fragements_vector, entry);
	} else if(entry->mmtp_packet_version == 0x03) {
		vlc_vector_push(&sub_flow_vec->repair_symbol_vector, entry);
	}
}

//todo #define's
//	msg_Dbg( p_demux, "packet version: %d, payload_type: 0x%X, packet_id 0x%hu, timestamp: 0x%X, packet_sequence_number: 0x%X, packet_counter: 0x%X", mmtp_packet_version,
//			mmtp_payload_type, mmtp_packet_id, mmtp_timestamp, packet_sequence_number, packet_counter);

#endif /* MODULES_DEMUX_MMT_MMTP_TYPES_H_ */
