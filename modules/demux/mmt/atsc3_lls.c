/*
 *
 * atsc3_llt.c:  driver for ATSC 3.0 LLS listener over udp
 *
 */

#include "atsc3_lls.h"
#include "xml.h"

static lls_table_t* __lls_create_base_table_raw(uint8_t* lls, int size) {

	lls_table_t *base_table = calloc(1, sizeof(lls_table_t));

	//read first 32 bytes in
	base_table->lls_table_id = lls[0];
	base_table->lls_group_id = lls[1];
	base_table->group_count_minus1 = lls[2];
	base_table->lls_table_version = lls[3];

	int remaining_payload_size = (size > 65531) ? 65531 : size;

	uint8_t *temp_gzip_payload = calloc(size, sizeof(uint8_t));
	//FILE *f = fopen("slt.gz", "w");

	for(int i=4; i < remaining_payload_size; i++) {
		//printf("i:0x%x ", lls[i]);
		//fwrite(&lls[i], 1, 1, f);
		temp_gzip_payload[i-4] = lls[i];
	}
	base_table->raw_xml.xml_payload_compressed = temp_gzip_payload;
	base_table->raw_xml.xml_payload_compressed_size = remaining_payload_size -4;

	//printf("first 4 hex: 0x%x 0x%x 0x%x 0x%x", temp_gzip_payload[0], temp_gzip_payload[1], temp_gzip_payload[2], temp_gzip_payload[3]);

	return base_table;
}


/**
 * footnote 5
 * The maximum size of the IP datagram is 65,535 bytes.
 * The maximum UDP data payload is 65,535 minus 20 bytes for the IP header minus 8 bytes for the UDP header.
 */

#define GZIP_CHUNK_INPUT_SIZE_MAX 65507
#define GZIP_CHUNK_INPUT_READ_SIZE 1024
#define GZIP_CHUNK_OUTPUT_BUFFER_SIZE 1024*8

static int __unzip_gzip_payload(uint8_t *input_payload, uint input_payload_size, uint8_t **decompressed_payload) {

	if(input_payload_size > GZIP_CHUNK_INPUT_SIZE_MAX) return -1;

	uint input_payload_offset = 0;
	uint output_payload_offset = 0;
    unsigned char *output_payload = NULL;

    int ret;
    unsigned have;
    z_stream strm;


    uint8_t *decompressed;

    strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	strm.data_type = Z_TEXT;

	//treat this input_payload as gzip not just delfate
	ret = inflateInit2(&strm, 16+MAX_WBITS);

	if (ret != Z_OK)
	   return ret;

	do {

		strm.next_in = &input_payload[input_payload_offset];

		uint payload_chunk_size = input_payload_size - input_payload_offset > GZIP_CHUNK_INPUT_READ_SIZE ? GZIP_CHUNK_INPUT_READ_SIZE : input_payload_size - input_payload_offset;
		strm.avail_in = payload_chunk_size;

		if (strm.avail_in <= 0)
			break;

		do {
			if(!output_payload) {
				output_payload = calloc(GZIP_CHUNK_OUTPUT_BUFFER_SIZE + 1, sizeof(uint8_t));
			} else {
				output_payload = realloc(output_payload, output_payload_offset + GZIP_CHUNK_OUTPUT_BUFFER_SIZE + 1);
			}

			if(!output_payload)
				return -1;

			strm.avail_out = GZIP_CHUNK_OUTPUT_BUFFER_SIZE;
			strm.next_out = &output_payload[output_payload_offset];

			ret = inflate(&strm, Z_NO_FLUSH);

			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
				return ret;
			}

			if(strm.avail_out == 0) {
				output_payload_offset += GZIP_CHUNK_OUTPUT_BUFFER_SIZE;
			}
		} while (strm.avail_out == 0);

		input_payload_offset += GZIP_CHUNK_INPUT_READ_SIZE;

	} while (ret != Z_STREAM_END && input_payload_offset < input_payload_size);


	int paylod_len = (output_payload_offset + (GZIP_CHUNK_OUTPUT_BUFFER_SIZE - strm.avail_out));
	/* clean up and return */
	output_payload[paylod_len] = '\0';
	*decompressed_payload = output_payload;

	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ?  paylod_len : Z_DATA_ERROR;

}

lls_table_t* lls_create_xml_table( uint8_t* lls_packet, int size) {
	lls_table_t *lls_table = __lls_create_base_table_raw(lls_packet, size);

	uint8_t *decompressed_payload;
	int ret = __unzip_gzip_payload(lls_table->raw_xml.xml_payload_compressed, lls_table->raw_xml.xml_payload_compressed_size, &decompressed_payload);

	if(ret > 0) {
		lls_table->raw_xml.xml_payload = decompressed_payload;
		lls_table->raw_xml.xml_payload_size = ret;

		return lls_table;
	}

	return NULL;
}

/***
 * decompressed l
 */




void lls_dump_base_table(lls_table_t *base_table) {
	println("base table:");
	println("-----------");
	println("lls_table_id				: %d	(0x%x)", base_table->lls_table_id, base_table->lls_table_id);
	println("lls_group_id				: %d	(0x%x)", base_table->lls_group_id, base_table->lls_group_id);
	println("group_count_minus1			: %d	(0x%x)", base_table->group_count_minus1, base_table->group_count_minus1);
	println("lls_table_version			: %d	(0x%x)", base_table->lls_table_version, base_table->lls_table_version);
	println("xml decoded payload size 	: %d", 	base_table->raw_xml.xml_payload_size);
	println("%s", base_table->raw_xml.xml_payload);
	println("---");

}



#define __UNIT_TEST 1
#ifdef __UNIT_TEST

int test_lls_create_xml_table(char* base64_payload);

int main() {

	test_lls_create_xml_table(__get_test_slt());

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


int test_lls_create_xml_table(char* base64_payload) {

	uint8_t *binary_payload;
	int binary_payload_size;

	__create_binary_payload(base64_payload, &binary_payload, &binary_payload_size);

	lls_table_t *lls_table = lls_create_xml_table(binary_payload, binary_payload_size);

	lls_dump_base_table(lls_table);

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
	lls_dump_base_table(parsed_table);

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


