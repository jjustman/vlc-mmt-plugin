/*
 *
 * atsc3_llt.c:  driver for ATSC 3.0 LLS listener over udp
 *
 */

#include "atsc3_utils.h"
#include "atsc3_lls.h"
#include "xml.h"

static lls_table_t* __lls_create_base_table_raw(uint8_t* lls, int size) {

	//zero out full struct
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
	base_table->raw_xml.xml_payload_compressed_size = remaining_payload_size - 4;


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

int __unzip_gzip_payload(uint8_t *input_payload, uint input_payload_size, uint8_t **decompressed_payload) {

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

lls_table_t* lls_create_table( uint8_t* lls_packet, int size) {

	lls_table_t* lls_table = lls_create_xml_table(lls_packet, size);
	if(!lls_table) {
		_LLS_ERROR("lls_create_table - error creating instance of LLS table and subclass");
		return NULL;
	}
	//process XML payload

	_LLS_TRACE("lls_create_table, raw xml payload is: \n%s", lls_table->raw_xml.xml_payload);
	xml_node_t* xml_root = parse_xml_payload(lls_table->raw_xml.xml_payload, lls_table->raw_xml.xml_payload_size);

	//get our first tag name and delegate to parser methods

	_LLS_TRACE("lls_create_table: calling lls_create_table_type_instance with xml children count: %d\n", xml_node_children(xml_root));

	int res = lls_create_table_type_instance(lls_table, xml_root);

	if(!res)
		return lls_table;
	else
		return NULL;
}

xml_node_t* parse_xml_payload(uint8_t *xml, int xml_size) {
	xml_document_t* document = xml_parse_document(xml, xml_size);
	if (!document) {
		_LLS_ERROR("Could not parse document");
		return NULL;
	}

	xml_node_t* root = xml_document_root(document);
	xml_string_t* root_node_name = xml_node_name(root); //root

	//chomp past root xml document declaration
	if(xml_string_equals_ignore_case(root_node_name, "?xml")) {
		root = xml_node_child(root, 0);
		root_node_name = xml_node_name(root); //root
		dump_xml_string(root_node_name);
	}

	_LLS_TRACE("%d:atsc3_lls.c:parse_xml_payload, returning first node:");
	dump_xml_string(root_node_name);

	return root;
}

int lls_create_table_type_instance(lls_table_t* lls_table, xml_node_t* xml_root) {

	xml_string_t* root_node_name = xml_node_name(xml_root); //root

	uint8_t* node_name = xml_string_clone(root_node_name);
	_LLS_TRACE("lls_create_table_type_instance: lls_table_id: %d, node ptr: %p, name is: %s\n", lls_table->lls_table_id, root_node_name, node_name);

	int ret = -1;
	if(lls_table->lls_table_id == SLT) {
		//build SLT table
		ret = build_SLT_table(lls_table, xml_root);

	} else if(lls_table->lls_table_id == RRT) {
		_LLS_ERROR("lls_create_table_type_instance: LLS table RRT not supported yet");
	} else if(lls_table->lls_table_id == SystemTime) {
		ret = build_SystemTime_table(lls_table, xml_root);
	} else if(lls_table->lls_table_id == AEAT) {
		_LLS_ERROR("lls_create_table_type_instance: LLS table AEAT not supported yet");
	} else if(lls_table->lls_table_id == OnscreenMessageNotification) {
		_LLS_ERROR("lls_create_table_type_instance: LLS table OnscreenMessageNotification not supported yet");
	}

	return ret;
}

#define LLS_SLT_SIMULCAST_TSID 				"SimulcastTSID"
#define LLS_SLT_SVC_CAPABILITIES			"SvcCapabilities"
#define LLS_SLT_BROADCAST_SVC_SIGNALING 	"BroadcastSvcSignaling"
#define LLS_SLT_SVC_INET_URL				"SvcInetUrl"
#define LLS_SLT_OTHER_BSID					"OtherBsid"

int build_SLT_table(lls_table_t *lls_table, xml_node_t *xml_root) {

	/** bsid **/

	xml_string_t* root_node_name = xml_node_name(xml_root); //root
	dump_xml_string(root_node_name);

	uint8_t* slt_attributes = xml_attributes_clone(root_node_name);
	kvp_collection_t* slt_attributes_collecton = kvp_parse_string(slt_attributes);
	char* bsid_char = kvp_find_key(slt_attributes_collecton, "bsid");
	//if there is a space, split and callocif(strnstr(bsid, "", ))

	//TODO: fix me
	if(bsid_char) {
		int bsid_i;
		itoa(bsid_i, bsid_char, 10);

		lls_table->slt_table.bsid_n = 1;
		lls_table->slt_table.bsid =  (int**)calloc(lls_table->slt_table.bsid_n , sizeof(int));
		lls_table->slt_table.bsid[0] = &bsid_i;
	}

	_LLS_TRACE("build_SLT_table, attributes are: %s\n", slt_attributes);

	int svc_size = xml_node_children(xml_root);

	//build our service rows
	for(int i=0; i < svc_size; i++) {
		xml_node_t* service_row_node = xml_node_child(xml_root, i);
		xml_string_t* service_row_node_xml_string = xml_node_name(service_row_node);

		/** push service row **/
		lls_table->slt_table.service_entry_n++;
		if(!lls_table->slt_table.service_entry) {
			lls_table->slt_table.service_entry = (service_t**)calloc(32, sizeof(service_t**));
		}

		//service_row_node_xml_string
		uint8_t* child_row_node_attributes_s = xml_attributes_clone(service_row_node_xml_string);
		kvp_collection_t* service_attributes_collecton = kvp_parse_string(child_row_node_attributes_s);

		lls_table->slt_table.service_entry[lls_table->slt_table.service_entry_n-1] = calloc(1, sizeof(service_t));
		service_t* service_entry = lls_table->slt_table.service_entry[lls_table->slt_table.service_entry_n-1];
		//map in other attributes, e.g


		int scratch_i = 0;
		char* serviceId = kvp_find_key(service_attributes_collecton, "serviceId");
		if(!serviceId) {
			_LLS_ERROR("missing serviceId!");
			return -1;
		}
		_LLS_DEBUG("service id is:|%s|", serviceId);

		itoa(scratch_i, serviceId, 10);
		_LLS_DEBUG("service id is:|%s|", serviceId);


		service_entry->service_id = scratch_i & 0xFFFF;
		_LLS_DEBUG("service id is: %s, int is: %d, uint_16: %u", serviceId, scratch_i, (scratch_i & 0xFFFF));

		service_entry->short_service_name = kvp_find_key(service_attributes_collecton, "shortServiceName");

		int svc_child_size = xml_node_children(service_row_node);

		dump_xml_string(service_row_node_xml_string);

		for(int j=0; j < svc_child_size; j++) {

			xml_node_t* child_row_node = xml_node_child(service_row_node, j);
			xml_string_t* child_row_node_xml_string = xml_node_name(child_row_node);

			uint8_t* child_row_node_attributes_s = xml_attributes_clone(child_row_node_xml_string);
			kvp_collection_t* child_attributes_kvp = kvp_parse_string(child_row_node_attributes_s);

			dump_xml_string(child_row_node_xml_string);

			if(xml_string_equals_ignore_case(child_row_node_xml_string, LLS_SLT_SIMULCAST_TSID)) {
				_LLS_ERROR("build_SLT_table - not supported: LLS_SLT_SIMULCAST_TSID");
			} else if(xml_string_equals_ignore_case(child_row_node_xml_string, LLS_SLT_SVC_CAPABILITIES)) {
				_LLS_ERROR("build_SLT_table - not supported: LLS_SLT_SVC_CAPABILITIES");
			} else if(xml_string_equals_ignore_case(child_row_node_xml_string, LLS_SLT_BROADCAST_SVC_SIGNALING)) {
				build_SLT_BROADCAST_SVC_SIGNALING_table(service_entry, service_row_node, child_attributes_kvp);

			} else if(xml_string_equals_ignore_case(child_row_node_xml_string, LLS_SLT_SVC_INET_URL)) {
				_LLS_ERROR("build_SLT_table - not supported: LLS_SLT_SVC_INET_URL");
			} else if(xml_string_equals_ignore_case(child_row_node_xml_string, LLS_SLT_OTHER_BSID)) {
				_LLS_ERROR("build_SLT_table - not supported: LLS_SLT_OTHER_BSID");
			} else {
				_LLS_ERROR("build_SLT_table - unknown type: %s\n", xml_string_clone(child_row_node_xml_string));
			}
		}
	}
	return 0;
}

void build_SLT_BROADCAST_SVC_SIGNALING_table(service_t* service_table, xml_node_t *service_row_node, kvp_collection_t* kvp_collection) {
	xml_string_t* service_row_node_xml_string = xml_node_name(service_row_node);
	uint8_t *svc_attributes = xml_attributes_clone(service_row_node_xml_string);

	_LLS_TRACE("build_SLT_BROADCAST_SVC_SIGNALING_table - attributes are: %s", svc_attributes);

	service_table->broadcast_svc_signaling.sls_destination_ip_address = kvp_find_key(kvp_collection, "slsDestinationIpAddress");
	service_table->broadcast_svc_signaling.sls_destination_udp_port = kvp_find_key(kvp_collection, "slsDestinationUdpPort");
	service_table->broadcast_svc_signaling.sls_source_ip_address = kvp_find_key(kvp_collection, "slsSourceIpAddress");
	service_table->broadcast_svc_signaling.sls_protocol = -1;
	//kvp_find_key(kvp_collection, "slsProtocol";

}

/** payload looks like:
 *
 * <SystemTime xmlns="http://www.atsc.org/XMLSchemas/ATSC3/Delivery/SYSTIME/1.0/" currentUtcOffset="37" utcLocalOffset="-PT5H" dsStatus="false"/>
 */
int build_SystemTime_table(lls_table_t* lls_table, xml_node_t* xml_root) {

	xml_string_t* root_node_name = xml_node_name(xml_root); //root
	dump_xml_string(root_node_name);

	uint8_t* SystemTime_attributes = xml_attributes_clone(root_node_name);
	kvp_collection_t* SystemTime_attributes_collecton = kvp_parse_string(SystemTime_attributes);

	int scratch_i = 0;

	char* currentUtcOffset =	kvp_find_key(SystemTime_attributes_collecton, "currentUtcOffset");
	char* ptpPrepend = 			kvp_find_key(SystemTime_attributes_collecton, "ptpPrepend");
	char* leap59 =				kvp_find_key(SystemTime_attributes_collecton, "leap59");
	char* leap61 = 				kvp_find_key(SystemTime_attributes_collecton, "leap61");
	char* utcLocalOffset = 		kvp_find_key(SystemTime_attributes_collecton, "utcLocalOffset");
	char* dsStatus = 			kvp_find_key(SystemTime_attributes_collecton, "dsStatus");
	char* dsDayOfMonth = 		kvp_find_key(SystemTime_attributes_collecton, "dsDayOfMonth");
	char* dsHour = 				kvp_find_key(SystemTime_attributes_collecton, "dsHour");

	if(!currentUtcOffset || !utcLocalOffset) {
		_LLS_ERROR("build_SystemTime_table, required elements missing: currentUtcOffset: %p, utcLocalOffset: %p", currentUtcOffset, utcLocalOffset);
		return -1;
	}

	itoa(scratch_i, currentUtcOffset, 10);

	//munge negative sign
	if(scratch_i < 0) {
		lls_table->system_time_table.current_utc_offset = (1 << 15) | (scratch_i & 0x7FFF);
	} else {
		lls_table->system_time_table.current_utc_offset = scratch_i & 0x7FFF;
	}

	lls_table->system_time_table.utc_local_offset = utcLocalOffset;

	if(ptpPrepend) {
		itoa(scratch_i, ptpPrepend, 10);
		lls_table->system_time_table.ptp_prepend = scratch_i & 0xFFFF;
	}

	if(leap59) {
		lls_table->system_time_table.leap59 = strcasecmp(leap59, "t");
	}

	if(leap61) {
		lls_table->system_time_table.leap61 = strcasecmp(leap61, "t");
	}

	if(dsStatus) {
		lls_table->system_time_table.ds_status = strcasecmp(dsStatus, "t");
	}

	if(dsDayOfMonth) {
		itoa(scratch_i, dsDayOfMonth, 10);
		lls_table->system_time_table.ds_status = scratch_i & 0xFF;
	}

	if(dsHour) {
		itoa(scratch_i, dsHour, 10);
		lls_table->system_time_table.ds_status = scratch_i & 0xFF;
	}

	return 0;
}


void lls_dump_instance_table(lls_table_t* base_table) {
	_LLS_TRACE("dump_instance_table: base_table address: %p", base_table);
	_LLS_DEBUGN("");
	_LLS_DEBUGN("--------------------------");
	_LLS_DEBUGN("LLS Base Table:");
	_LLS_DEBUGN("--------------------------");
	_LLS_DEBUGN("lls_table_id             : %d (0x%x)", base_table->lls_table_id, base_table->lls_table_id);
	_LLS_DEBUGN("lls_group_id             : %d (0x%x)", base_table->lls_group_id, base_table->lls_group_id);
	_LLS_DEBUGN("group_count_minus1       : %d (0x%x)", base_table->group_count_minus1, base_table->group_count_minus1);
	_LLS_DEBUGN("lls_table_version        : %d (0x%x)", base_table->lls_table_version, base_table->lls_table_version);
	_LLS_DEBUGN("xml decoded payload size : %d", 	base_table->raw_xml.xml_payload_size);
	_LLS_DEBUGN("--------------------------");
	_LLS_DEBUGA("\t%s", base_table->raw_xml.xml_payload);
	_LLS_DEBUGN("--------------------------");

	if(base_table->lls_table_id == SLT) {
		_LLS_DEBUGN("SLT");
		_LLS_DEBUGN("--------------------------");
		for(int i=0; i < base_table->slt_table.bsid_n; i++) {
			_LLS_DEBUGNT("BSID: %d", *base_table->slt_table.bsid[i]);
		}
		_LLS_DEBUGNT("Service contains %d entries:", base_table->slt_table.service_entry_n);

		for(int i=0l; i < base_table->slt_table.service_entry_n; i++) {
			service_t* service = base_table->slt_table.service_entry[i];
			_LLS_DEBUGNT("--------------------------");
			_LLS_DEBUGNT("service_id                : %d", service->service_id);
			_LLS_DEBUGNT("global_service_id         : %s", service->global_service_id);
			_LLS_DEBUGNT("major_channel_no          : %d", service->major_channel_no);
			_LLS_DEBUGNT("minor_channel_no          : %d", service->minor_channel_no);
			_LLS_DEBUGNT("service_category          : %d", service->service_category);
			_LLS_DEBUGNT("short_service_name        : %s", service->short_service_name);
			_LLS_DEBUGNT("slt_svc_seq_num           : %d", service->slt_svc_seq_num);
			_LLS_DEBUGNT("--------------------------");
			_LLS_DEBUGNT("broadcast_svc_signaling");
			_LLS_DEBUGNT("--------------------------");
			_LLS_DEBUGNT("sls_protocol              : %d", service->broadcast_svc_signaling.sls_protocol);
			_LLS_DEBUGNT("sls_destination_ip_address: %s", service->broadcast_svc_signaling.sls_destination_ip_address);
			_LLS_DEBUGNT("sls_destination_udp_port  : %s", service->broadcast_svc_signaling.sls_destination_udp_port);
			_LLS_DEBUGNT("sls_source_ip_address     : %s", service->broadcast_svc_signaling.sls_source_ip_address);
			_LLS_DEBUGNT("--------------------------");

		}

		_LLS_DEBUGNT("");
		_LLS_DEBUGN("--------------------------");
	}

	//decorate with instance types: hd = int16_t, hu = uint_16t, hhu = uint8_t
	if(base_table->lls_table_id == SystemTime) {
		_LLS_DEBUGN("SystemTime:");
		_LLS_DEBUGN("--------------------------");
		_LLS_DEBUGNT("current_utc_offset       : %hd", base_table->system_time_table.current_utc_offset);
		_LLS_DEBUGNT("ptp_prepend              : %hu", base_table->system_time_table.ptp_prepend);
		_LLS_DEBUGNT("leap59                   : %d",  base_table->system_time_table.leap59);
		_LLS_DEBUGNT("leap61                   : %d",  base_table->system_time_table.leap61);
		_LLS_DEBUGNT("utc_local_offset         : %s",  base_table->system_time_table.utc_local_offset);
		_LLS_DEBUGNT("ds_status                : %d",  base_table->system_time_table.ds_status);
		_LLS_DEBUGNT("ds_day_of_month          : %hhu", base_table->system_time_table.ds_day_of_month);
		_LLS_DEBUGNT("ds_hour                  : %hhu", base_table->system_time_table.ds_hour);
		_LLS_DEBUGN("--------------------------");

	}
	_LLS_DEBUGN("");

}




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


