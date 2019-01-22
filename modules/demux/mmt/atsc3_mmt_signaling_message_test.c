/*
 *
 * atsc3_mmt_signaling_message_test.c:  driver for MMT signaling message mapping MPU sequence numbers to MPU presentatino time
 *
 */

#include "atsc3_mmtp_types.h"
#include "atsc3_mmt_signaling_message.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


#define __UNIT_TEST 1
#ifdef __UNIT_TEST

//system_time_message with packet_id=1
static char* __get_test_mmt_signaling_message_mpu_timestamp_descriptor()	{ return "62020023afb90000002b4f2f00351058a40000000012ce003f12ce003b04010000000000000000101111111111111111111111111111111168657631fd00ff00015f9001000023000f00010c000016cedfc2afb8d6459fff"; }

int test_mmt_signaling_message_mpu_timestamp_descriptor_table(char* base64_payload);

int main() {

	test_mmt_signaling_message_mpu_timestamp_descriptor_table(__get_test_mmt_signaling_message_mpu_timestamp_descriptor());

	return 0;
}



void __create_binary_payload(char *test_payload_base64, uint8_t **binary_payload, int * binary_payload_size) {
	int test_payload_base64_length = strlen(test_payload_base64);
	int test_payload_binary_size = test_payload_base64_length/2;

	uint8_t *test_payload_binary = calloc(test_payload_binary_size, sizeof(uint8_t));

	for (size_t count = 0; count < test_payload_binary_size; count++) {
	        sscanf(test_payload_base64, "%2hhx", &test_payload_binary[count]);
	        test_payload_base64 += 2;
	}

	*binary_payload = test_payload_binary;
	*binary_payload_size = test_payload_binary_size;
}


int test_mmt_signaling_message_mpu_timestamp_descriptor_table(char* base64_payload) {

	uint8_t *binary_payload;
	int binary_payload_size;

	__create_binary_payload(base64_payload, &binary_payload, &binary_payload_size);

//	lls_table_t* lls = lls_table_create(binary_payload, binary_payload_size);
//	if(lls) {
//		lls_dump_instance_table(lls);
//	} else {
//		_LLS_ERROR("test_lls_create_SystemTime_table() - lls_table_t* is NULL");
//	}
//
	return 0;
}




#endif



