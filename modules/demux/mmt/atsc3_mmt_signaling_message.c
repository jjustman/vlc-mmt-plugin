/*
 * atsc3_mmt_signaling_message.h
 *
 *  Created on: Jan 21, 2019
 *      Author: jjustman
 */

#include "atsc3_mmt_signaling_message.h"

/**
 *
 * MPU_timestamp_descriptor message example
 *
0000   62 02 00 23 af b9 00 00 00 2b 4f 2f 00 35 10 58   b..#¯¹...+O/.5.X
0010   a4 00 00 00 00 12 ce 00 3f 12 ce 00 3b 04 01 00   ¤.....Î.?.Î.;...
0020   00 00 00 00 00 00 00 10 11 11 11 11 11 11 11 11   ................
0030   11 11 11 11 11 11 11 11 68 65 76 31 fd 00 ff 00   ........hev1ý.ÿ.
0040   01 5f 90 01 00 00 23 00 0f 00 01 0c 00 00 16 ce   ._....#........Î
0050   df c2 af b8 d6 45 9f ff                           ßÂ¯¸ÖE.ÿ

raw base64 payload:

62020023afb90000002b4f2f00351058a40000000012ce003f12ce003b04010000000000000000101111111111111111111111111111111168657631fd00ff00015f9001000023000f00010c000016cedfc2afb8d6459fff
 *
 */

typedef struct mmt_signaling_message_mpu_tuple {
	uint32_t mpu_sequence_number;
	uint64_t mpu_presentation_time;
} mmt_signaling_message_mpu_tuple_t;

typedef struct mmt_signaling_message_mpu_timestamp_desc {
	uint16_t							descriptor_tag;
	uint8_t								descriptor_length;
	uint8_t								mpu_tuple_n; //mpu_tuple_n = descriptor_length/12 = (32+64)/8
	mmt_signaling_message_mpu_tuple_t*	mpu_tuple;
} mmt_signaling_message_mpu_timestamp_desc_t;




#endif /* MODULES_DEMUX_MMT_ATSC3_MMT_SIGNALING_MESSAGE_H_ */
