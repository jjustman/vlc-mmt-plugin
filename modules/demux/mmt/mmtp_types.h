/*
 * mmtp_types.h
 *
 *  Created on: Jan 3, 2019
 *      Author: jjustman
 */

#ifndef MODULES_DEMUX_MMT_MMTP_TYPES_H_
#define MODULES_DEMUX_MMT_MMTP_TYPES_H_

#include <vlc_vector.h>


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


//define for mpu type common header fields for struct inheritance
//todo: add in MMTHSample box

#define _MMTP_MPU_TYPE_HEADER_FIELDS 		\
	_MMTP_PACKET_HEADER_FIELDS;				\
	uint16_t mpu_payload_length;			\
	uint8_t mpu_fragment_type;				\
	uint8_t mpu_timed_flag;					\
	uint8_t mpu_fragmentation_indicator;	\
	uint8_t mpu_aggregation_flag;			\
	uint8_t mpu_fragmentation_counter;		\
	block_t *mpu_data_unit_payload;			\

typedef struct {
	_MMTP_MPU_TYPE_HEADER_FIELDS;
	uint32_t movie_fragment_sequence_number;
	uint32_t sample_number;
	uint32_t offset;
	uint8_t priority;
	uint8_t dep_counter;

} mpu_data_unit_payload_fragments_timed_t;

typedef struct {
	_MMTP_MPU_TYPE_HEADER_FIELDS;

	uint32_t non_timed_mfu_item_id;

} mpu_data_unit_payload_fragments_nontimed_t;

typedef struct VLC_VECTOR(struct mpu_data_unit_payload_fragments_timed_t *) 	mpu_data_unit_payload_fragments_timed_vector_t;
typedef struct VLC_VECTOR(struct mpu_data_unit_payload_fragments_nontimed_t *)	mpu_data_unit_payload_fragments_nontimed_vector_t;

typedef struct {
	mpu_data_unit_payload_fragments_timed_vector_t 		timed_fragments_vector;
	mpu_data_unit_payload_fragments_nontimed_vector_t 	nontimed_fragments_vector;

} mpu_data_unit_payload_fragments_t;

typedef struct VLC_VECTOR(struct mpu_data_unit_payload_fragments_t *) mpu_data_unit_payload_fragments_vector_t;

typedef struct {
	struct mmtp_sub_flow_t *mmtp_sub_flow;
	uint32_t mpu_sequence_number;

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

typedef struct VLC_VECTOR(struct mpu_fragments_t *) 				mpu_fragments_vector_t;
typedef struct VLC_VECTOR(struct generic_object_fragments_t *) 		generic_object_fragments_vector_t;
typedef struct VLC_VECTOR(struct signalling_message_fragments_t *) 	signalling_message_fragments_vector_t;
typedef struct VLC_VECTOR(struct repair_symbol_t *) 				repair_symbol_vector_t;

typedef struct  {
	uint16_t packet_id;

	//mmtp payload type collections for reconstruction/recovery of payload types

	//mpu (media_processing_unit):			paylod_type==0x00
	mpu_fragments_vector_t 					mpu_fragments_vector;

	//generic object:						payload_type==0x01
	generic_object_fragments_vector_t 		eneric_object_fragments_vector;

	//signalling message: 					payload_type=0x02
	signalling_message_fragments_vector_t 	signalling_message_fragements_vector;

	//repair symbol:						payload_type==0x03
	repair_symbol_vector_t 					repair_symbol_vector;

} mmtp_sub_flow_t;


typedef struct VLC_VECTOR(struct mmtp_sub_flow_t *) mmtp_sub_flow_vector_t;


#endif /* MODULES_DEMUX_MMT_MMTP_TYPES_H_ */
