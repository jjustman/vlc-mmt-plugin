/*
 *
 * atsc3_llt.c:  driver for ATSC 3.0 LLS listener over udp
 *
 */

#include "atsc3_lls.h"
#include "xml.h"


#define __UNIT_TEST 1
#ifdef __UNIT_TEST

//slt with packet_id=2
static char* __get_test_slt()					{ return "010100021f8b08089217185c0003534c5400b5d55b6f82301400e0f7fd0ad2e70d4a41370d609c9ac5448d092ed99ba9d06117685d5bcdfcf73ba8cbe2bc44167d229c4bcfe9f70041ebabc8ad15539a4b1122d7c6c86222912917598896e6fde109b5a2bb201e4c2ca8143a4486664d6a74624b95dd13ecd69b6fc3419ccc5941b5d39ec41dcfe9b29cc3996b07da1c38d341d64cf33444358ca220666ac51366e9edb30f7117631759592e6734dfa5fb5d98afc466547357ca537865059b1685994243413fa4eacca9102c1fc9f2188871b11f433f833ad09b49b5dec6e65299dda8112d5888da93deb0670d8713ab4ce7265e2531fb1c2d8b10955b3f2b49d384ea4d9c6782e64004757aaca49189cc4344ca3edd65da70410d80f617ed34554c831af11a36a9d56c17dbeedfb2d77431866d8067eb00d9582e1518fcf6bb8fc476eb36c165bf1305ce6ef7539ca42a27b98c9354e724b7e53c28dbe324d7e1f4aa727a97717ad539bddb727a6739bdeb70fa5539fdcb38fdea9cfe6d39fdb39cfe15386b18372ee3643a3be2e31ff3e9c52fff7439f8ba1d7121d86e9c76219b0b557329ff34d1dd372e0efb8fce060000";}
//system_time_message with packet_id=1
static char* __get_test_system_time_message()	{ return "030100011f8b08089717185c000353797374656d54696d6500358dcb0a82401440f77ec570f77a0b89227c10151428056350cb61bc3e601cc3b966fe7d6eda1e38e744e9b733e243836b7b1bc33a588120abfbb2b5750c2357fe0ed2c48be4ec98baa2ed482c82753134ccef3de2344d8162a7837ea8f199675237d4298787421e433c916997f88cf2258b6b7ec6658020f4380c64f9c1fa56558e3886700b62649df55a993ff3efc5e602a27492158fcbb252c61160e2fd003518c11fb6000000"; }


int test_lls_create_xml_table(char* base64_payload);
void test_kvp_extraction();

int main() {

	//test_lls_create_xml_table(__get_test_slt());
	//test_lls_create_slt_table(__get_test_slt());
	test_kvp_extraction();
	return 0;
}

/**
 *
 *
 *
203:process_xml_payload - node ptr: 0x100400250, name is: SLT, size: 2
70:dump_xml_string::xml_string: len: 3, is_self_closing: 0, val: |SLT|, attributes len: 70, val: xmlns="tag:atsc.org,2016:XMLSchemas/ATSC3/Delivery/SLT/1.0/" bsid="50"
236:build_SLT_table, attributes are:

xmlns="tag:atsc.org,2016:XMLSchemas/ATSC3/Delivery/SLT/1.0/" bsid="50"
70:dump_xml_string::xml_string: len: 7, is_self_closing: 0, val: |Service|, attributes len: 172, val: serviceId="1001" globalServiceID="urn:atsc:serviceid:ateme_mmt_1" majorChannelNo="10" minorChannelNo="1" serviceCategory="1" shortServiceName="ATEME MMT 1" sltSvcSeqNum="0"
70:dump_xml_string::xml_string: len: 21, is_self_closing: 1, val: |BroadcastSvcSignaling|, attributes len: 118, val: slsProtocol="2" slsDestinationIpAddress="239.255.10.1" slsDestinationUdpPort="51001" slsSourceIpAddress="172.16.200.1"
283:build_SLT_BROADCAST_SVC_SIGNALING_table - attributes are: serviceId="1001" globalServiceID="urn:atsc:serviceid:ateme_mmt_1" majorChannelNo="10" minorChannelNo="1" serviceCategory="1" shortServiceName="ATEME MMT 1" sltSvcSeqNum="0"70:dump_xml_string::xml_string: len: 7, is_self_closing: 0, val: |Service|, attributes len: 172, val: serviceId="1002" globalServiceID="urn:atsc:serviceid:ateme_mmt_2" majorChannelNo="10" minorChannelNo="2" serviceCategory="1" shortServiceName="ATEME MMT 2" sltSvcSeqNum="0"
70:dump_xml_string::xml_string: len: 21, is_self_closing: 1, val: |BroadcastSvcSignaling|, attributes len: 118, val: slsProtocol="2" slsDestinationIpAddress="239.255.10.2" slsDestinationUdpPort="51002" slsSourceIpAddress="172.16.200.1"
283:build_SLT_BROADCAST_SVC_SIGNALING_table - attributes are: serviceId="1002" globalServiceID="urn:atsc:serviceid:ateme_mmt_2" majorChannelNo="10" minorChannelNo="2" serviceCategory="1" shortServiceName="ATEME MMT 2" sltSvcSeqNum="0"70:dump_xml_string::xml_string: len: 7, is_self_closing: 0, val: |Service|, attributes len: 172, val: serviceId="1003" globalServiceID="urn:atsc:serviceid:ateme_mmt_3" majorChannelNo="10" minorChannelNo="3" serviceCategory="1" shortServiceName="ATEME MMT 3" sltSvcSeqNum="0"
70:dump_xml_string::xml_string: len: 21, is_self_closing: 1, val: |BroadcastSvcSignaling|, attributes len: 118, val: slsProtocol="2" slsDestinationIpAddress="239.255.10.3" slsDestinationUdpPort="51003" slsSourceIpAddress="172.16.200.1"
283:build_SLT_BROADCAST_SVC_SIGNALING_table - attributes are: serviceId="1003" globalServiceID="urn:atsc:serviceid:ateme_mmt_3" majorChannelNo="10" minorChannelNo="3" serviceCategory="1" shortServiceName="ATEME MMT 3" sltSvcSeqNum="0"70:dump_xml_string::xml_string: len: 7, is_self_closing: 0, val: |Service|, attributes len: 172, val: serviceId="1004" globalServiceID="urn:atsc:serviceid:ateme_mmt_4" majorChannelNo="10" minorChannelNo="4" serviceCategory="1" shortServiceName="ATEME MMT 4" sltSvcSeqNum="0"
70:dump_xml_string::xml_string: len: 21, is_self_closing: 1, val: |BroadcastSvcSignaling|, attributes len: 118, val: slsProtocol="2" slsDestinationIpAddress="239.255.10.4" slsDestinationUdpPort="51004" slsSourceIpAddress="172.16.200.1"
283:build_SLT_BROADCAST_SVC_SIGNALING_table - attributes are: serviceId="1004" globalServiceID="urn:atsc:serviceid:ateme_mmt_4" majorChannelNo="10" minorChannelNo="4" serviceCategory="1" shortServiceName="ATEME MMT 4" sltSvcSeqNum="0"70:dump_xml_string::xml_string: len: 7, is_self_closing: 0, val: |Service|, attributes len: 117, val: serviceId="5009" globalServiceID="urn:atsc:serviceid:esg" serviceCategory="4" shortServiceName="ESG" sltSvcSeqNum="0"
70:dump_xml_string::xml_string: len: 21, is_self_closing: 1, val: |BroadcastSvcSignaling|, attributes len: 118, val: slsProtocol="1" slsDestinationIpAddress="239.255.20.9" slsDestinationUdpPort="52009" slsSourceIpAddress="172.16.200.1"
283:build_SLT_BROADCAST_SVC_SIGNALING_table - attributes are: serviceId="5009" globalServiceID="urn:atsc:serviceid:esg" serviceCategory="4" shortServiceName="ESG" sltSvcSeqNum="0"base table:
-----------
lls_table_id				: 1	(0x1)
lls_group_id				: 1	(0x1)



 */
void test_kvp_extraction() {
	char* kvp_test_string_1 = "xmlns=\"tag:atsc.org,2016:XMLSchemas/ATSC3/Delivery/SLT/1.0/\" bsid=\"50\"";
	kvp_collection_t *collection_1 = kvp_parse_string(kvp_test_string_1);
	printf("sizeof kvp collection: %d, bsid value is: %s", collection_1->size_n, kvp_find_key(collection, "bsid"));

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


int test_lls_create_xml_table(char* base64_payload) {

	uint8_t *binary_payload;
	int binary_payload_size;

	__create_binary_payload(base64_payload, &binary_payload, &binary_payload_size);

	lls_table_t *lls_table = lls_create_xml_table(binary_payload, binary_payload_size);

	lls_dump_instance_table(lls_table);

	return 0;
}



int test_lls_create_slt_table(char* base64_payload) {

	uint8_t *binary_payload;
	int binary_payload_size;

	__create_binary_payload(base64_payload, &binary_payload, &binary_payload_size);

	lls_table_t* lls = lls_create_table(binary_payload, binary_payload_size);

	lls_dump_instance_table(lls);

	return 0;
}
int test_lls_components() {

	char* test_payload_base64;

	test_payload_base64 = __get_test_slt();
	int test_payload_base64_length = strlen(test_payload_base64);
	int test_payload_binary_size = test_payload_base64_length/2;

	uint8_t *test_payload_binary = calloc(test_payload_binary_size, sizeof(uint8_t));

	for (size_t count = 0; count < test_payload_binary_size; count++) {
	        sscanf(test_payload_base64, "%2hhx", &test_payload_binary[count]);
	        test_payload_base64 += 2;
	}

	lls_table_t *parsed_table = lls_create_xml_table(test_payload_binary, test_payload_binary_size);
	lls_dump_instance_table(parsed_table);

	uint8_t *decompressed_payload;
	int ret = __unzip_gzip_payload(parsed_table->raw_xml.xml_payload, parsed_table->raw_xml.xml_payload_size, &decompressed_payload);
	//both char and %s with '\0' should be the same
	//printf("gzip ret is: %d\n", ret);
	for(int i=0; i < ret; i++) {
		printf("%c", decompressed_payload[i]);
	}

	printf("%s", decompressed_payload);

	return 0;
}


#endif


/** from vlc udp access module **/

#ifdef __STANDALONE

    char *psz_name = strdup( p_access->psz_location );
    char *psz_parser;
    const char *psz_server_addr, *psz_bind_addr = "";
    int  i_bind_port = 1234, i_server_port = 0;

    if( unlikely(psz_name == NULL) )
        return VLC_ENOMEM;

    /* Parse psz_name syntax :
     * [serveraddr[:serverport]][@[bindaddr]:[bindport]] */
    psz_parser = strchr( psz_name, '@' );
    if( psz_parser != NULL )
    {
        /* Found bind address and/or bind port */
        *psz_parser++ = '\0';
        psz_bind_addr = psz_parser;

        if( psz_bind_addr[0] == '[' )
            /* skips bracket'd IPv6 address */
            psz_parser = strchr( psz_parser, ']' );

        if( psz_parser != NULL )
        {
            psz_parser = strchr( psz_parser, ':' );
            if( psz_parser != NULL )
            {
                *psz_parser++ = '\0';
                i_bind_port = atoi( psz_parser );
            }
        }
    }

    psz_server_addr = psz_name;
    psz_parser = ( psz_server_addr[0] == '[' )
        ? strchr( psz_name, ']' ) /* skips bracket'd IPv6 address */
        : psz_name;

    if( psz_parser != NULL )
    {
        psz_parser = strchr( psz_parser, ':' );
        if( psz_parser != NULL )
        {
            *psz_parser++ = '\0';
            i_server_port = atoi( psz_parser );
        }
    }

    msg_Dbg( p_access, "opening server=%s:%d local=%s:%d",
             psz_server_addr, i_server_port, psz_bind_addr, i_bind_port );

    sys->fd = net_OpenDgram( p_access, psz_bind_addr, i_bind_port,
                             psz_server_addr, i_server_port, IPPROTO_UDP );
    free( psz_name );
    if( sys->fd == -1 )
    {
        msg_Err( p_access, "cannot open socket" );
        return VLC_EGENERIC;
    }

    sys->timeout = var_InheritInteger( p_access, "udp-timeout");
    if( sys->timeout > 0)
        sys->timeout *= 1000;

#endif


