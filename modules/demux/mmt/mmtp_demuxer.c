/*****************************************************************************
 * mmtp_demuxer.c : mmtp demuxer for ngbp
 *****************************************************************************
 *
 *
 * Author: jason@jasonjustman.com
 *
 * 2018-12-21
 *
 * Notes: alpha MMT parser and MPU/MFU chained demuxing
 *
 * Dependencies: pcap MMT unicast replay files or live ATSC 3.0 network mulitcast reception/reflection (see https://redzonereceiver.tv/)
 *
 * airwavez redzone SDR USB dongle
 *
 * the user space module can be flakey with recovery if the usb connection drops.
 * i use a script similar to the following to turn up, tune and monitor:
 *

#!/bin/bash

# Allow Multicast IP on the enp0s6 interface and route it there instead of to the wired interface
sudo ifconfig lo -multicast
sudo ifconfig enp0s5 -multicast
sudo ifconfig enp0s6 multicast
sudo route del -net 224.0.0.0 netmask 240.0.0.0 dev lo
sudo route add -net 224.0.0.0 netmask 240.0.0.0 dev enp0s6


#start userspace driver
klatsc30_web_ui -f -p 8080 &

sleep 10
#tune to channel 43 - you'll need to find a testing market (e.g. dallas or phenix)
wget 'http://127.0.0.1:8080/networktuner/tunefrequency?json={"operating_mode":"ATSC3","frequency_plan":"US_BROADCAST","frequency_Hz":647000000, "plp0_id":0}'


sleep 5

#start ff for monitoring
firefox http://127.0.0.1:8080

 *
 *
 * replay:
 * wireshark tcp captures must be in libpcap format, and most likely need to have packet checksums realcualted before replay:
 * e.g. tcprewrite --fixcsum -i 2018-12-17-mmt-airwavz-bad-checksums.pcap -o 2018-12-17-mmt-airwavz-recalc.pcap

 * replay via, e.g. bittwist -i enp0s6 2018-12-17-mmt-airwavz-recalc.pcap -v
 *
 *
 * lastly, i then have a host only interface between my ubuntu and mac configured in parallels, but mac's management of the mulitcast routes is a bit weird,
 * the two scripts will revoke any autoconfigured interface mulitcast routes, and then manually add the dedicated 224 route to the virtual host-only network:
 *
 *
cat /usr/local/bin/deleteMulticastRoute
netstat -nr
sudo route delete -net 224.0.0.0/4
sudo route delete -host 239.255.10.2
sudo route delete -host 239.255.255.250
sudo route delete -net 255.255.255.255/32

 cat /usr/local/bin/addVnic1MulitcastRoute
sudo route -nv add -net 224.0.0.0/4 -interface vnic1

 *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <limits.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_demux.h>
#include <vlc_sout.h>

#define ACCESS_TEXT N_("MMTP Demuxer module")
#define FILE_TEXT N_("Dump filename")
#define FILE_LONGTEXT N_( \
    "Name of the file to which the raw stream will be dumped." )
#define APPEND_TEXT N_("Append to existing file")
#define APPEND_LONGTEXT N_( \
    "If the file already exists, it will not be overwritten." )

static int  Open( vlc_object_t * );
static void Close ( vlc_object_t * );

#define MIN(a,b) (((a)<(b))?(a):(b))

vlc_module_begin ()
    set_shortname("MMTP")
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_DEMUX )
    set_description( N_("MMTP Demuxer") )
    set_capability( "demux", 500 )
  //  add_module("demuxdump-access", "sout access", "file",
  //            ACCESS_TEXT, ACCESS_TEXT)
  //  add_savefile("demuxdump-file", "stream-demux.dump",
  //               FILE_TEXT, FILE_LONGTEXT)
  //  add_bool( "demuxdump-append", false, APPEND_TEXT, APPEND_LONGTEXT,
  //           false )
    set_callbacks( Open, Close )
    add_shortcut( "MMTP" )
vlc_module_end ()


void processMpuPacket(demux_t* p_demux, uint16_t mmtp_packet_id, uint8_t mpu_fragment_type, uint8_t mpu_fragmentation_indicator, block_t *tmp_mpu_fragment );

//short reads from UDP may happen on starutp buffering or truncation
#define MIN_MMTP_SIZE 224
#define MAX_MMTP_SIZE 1514

static int Demux( demux_t * );
static int Control( demux_t *, int,va_list );

typedef struct  {
	//reconsititue mfu's into a p_out_muxed fifo
	block_t *p_mpu_block;
    vlc_thread_t  thread;

	vlc_demux_chained_t *p_out_muxed;
} demux_sys_t;

/**
 *
 *
 *  MMTP Packet V=0
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=0|C|FEC|r|X|R|RES|   type    |            packet_id          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |					     	timestamp						   | 64
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |					 packet_sequence_number				 	   | 96
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |					     packet_counter				 	       | 128
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | 						 Header Extension				   ..... 160 == 20 bytes
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | 						   Payload Data				       ..... 192
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |				      source_FEC_payload_ID					   | 224
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  ---
 *
 *  MMTP Packet V=1
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=1|C|FEC|X|R|Q|F|E|B|I| type  |           packet_id           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |					     	timestamp						   | 64
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |					 packet_sequence_number				 	   | 96
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |					     packet_counter				 	       | 128
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |r|TB | DS  | TP  | flow_label  |         extension_header  ....| 160+n == 20bytes
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | 						   Payload Data				       ..... 192
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |				      source_FEC_payload_ID					   | 224
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Semantics
 *
 * version  				indicates the version number of the MMTP protocol. This field shall be set
 * (V: 2 bits)			  	to "00" to for ISO 23008-1 2017, and 01 for QoS support.
 *
 * 							NOTE If version is set to 01, the length of type is set to 4 bits.
 *
 * packet_counter_flag   	1 in this field indicates that the packet_counter field is present.
 * (C: 1 bit)
 *
 * FEC_type  				indicates the type of the FEC scheme used for error protection of MMTP packets.
 * (FEC: 2 bits)			Valid values of this field are listed in Table 8.
 *								0	MMTP packet without source_FEC_payload_ID field (NO FEC applied)
 *								1	MMTP packet with source_FEC_payload_ID field
 *								2 	MMTP packet for repair symbol(s) for FEC Payload Mode 0 (FEC repair packet)
 *								3	MMTP packet for repair symbol(s) for FEC Payload Mode 1 (FEC repair packet)
 *
 * reserved					reserved for future use (in V=0, bit position 5 only)
 * (r: 1 bit)
 *
 * extension_flag			when set to 1, this flag indicates the header_extension field is present
 * (X: 1 bit) 					V=0, bit position 6
 *								V=1, bit position 5
 * RAP_flag
 * (R: 1 bit)				when set to 1, this flag indicates that the payload contains a Random Access Point to the datastream of that data type,
 *							defined by the data type itself.
 *
 * reserved					V=0 only - reserverd for future use
 * (RES: 2 bits)
 *
 * Compression_flag			V=1 only - this field will identify if header compression is used.
 * (B: 1 bit) 	 	 	 	 	 	 	 	 	 B=0, full size header will be used
 * 	 	 	 	 	 	 	 	 	 	 	 	 B=1, reduced size header will be used
 *
 * Indicator_flag			V=1 only - if set to I=1, this header is a reference header which will be
 * (I: 1 bit) 									 used later for a future packet with reduced headers.
 *
 * Payload Type				Payload data type and definitions, differs between V=0 and V=1
 * (type: 6 bits when V=0)
 * 							Value		Data Type			Definition of data unit
 * 							-----		---------			-----------------------
 * 							0x00		MPU					media-aware fragment of the MPU
 * 							0x01		generic object		generic such as complete MPU or another type
 * 							0x02		signaling message	one or more signaling messages
 * 							0x03		repair symbol		a single complete repair signal
 * 							0x04-0x1F	reserved 			for ISO use
 * 							0x20-0x3F	reserved			private use
 *
 * Payload Type
 * (type: 4 bits when V=1)
 * 							Value		Data Type			Definition of data unit
 * 							-----		---------			----------------------
 * 							0x0			MPU					media-aware fragment of the MPU
 * 							0x1			generic object		generic such as complete MPU or another type
 * 							0x2			signaling message	one or more signaling messages
 * 							0x3			repair signal		a single complete repair signal
 * 							0x4-0x9		reserved			for ISO use
 * 							0xA-0xF		reserved			private use
 *
 * packet_id				See ISO 23008-1 page 27
 * (16 bits)				used to distinguish one asset from another,
 * 							packet_id to asset_id is captured in the MMT Package Table as part of signaling message
 *
 * packet_sequence_number	used to distinguish between packets with the same packet_id
 * (32 bits)				begings at arbritary value, increases by one for each MMTP packet received,
 * 							and will wraparound to 0 at INT_MAX
 *
 * timestamp				time instance of MMTP packet delivery based upon UTC.
 * (32 bits)				short format defined in IETF RFC 5905 NTPv4 clause 6.
 *
 * packet_counter			integer value for counting MMTP packets, incremented by 1 when a MMTP packet is sent regardless of its packet_id value.
 * (32 bits)				arbitrary value, wraps around to 0
 * 							all packets of an MMTP flow shall have the same setting for packet_counter_flag (c)
 *
 * source_FEC_payload_ID	used only when FEC type=1.  MMTP packet will be AL-FEC Payload ID Mode
 * (32 bits)
 *
 * header_extension			contains user-defined information, used for proprietary extensions to the payload format
 * (16/16bits)						to enable applications and media types that require additional information the payload format header
 *
 * QoS_classifer flag		a value of 1 indicates the Qos classifer information is used
 * (Q: 1 bit)
 *
 * flow_identifer_flag		when set to 1, indicates that the flow identifier is used
 * (F:1 bit)					flow_label and flow_extnesion_flag fields, characteristics or ADC in a package
 *
 * flow_extension_flag		if there are more than 127 flows, this bit set set to 1 and more byte can be used in extension_header
 * (E: 1 bit)
 *
 * reliability_flag			when reliability flag is set to 0, data is loss tolerant (e.g media display), and pre-emptable by "transmission priority"
 * (r: 1 bit)				when reliability flag is set to 1, data is not loss tolerant (e.g signaling) and will pre-empt "transmission priority"
 *
 * type_of_bitrate			00 		constant bitrate, e.g. CBR
 * (TB: 2 bits)				01 		non-constrant bitrate, e.g. nCBR
 * 							10-11	reserved
 *
 * delay_sensitivity		indicates the sensitivty of the delay for end-to-end delivery
 * (DS: 3 bits)
 * 							111		conversational services (~100ms)
 * 							110		live-streming service (~1s)
 * 							101		delay-sensitive interactive service (~2s)
 * 							100		interactive service (~5s)
 * 							011		streaming service (~10s)
 * 							010		non-realtime service
 * 							001		reserved
 * 							000		reserved
 *
 * transmission_priority	provides the transmission priority of the packet, may be mapped
 * (TP: 3 bits)				to the NRI of the NAL, DSCP of IETF or other prioty fields from:
 * 							highest: 7 (1112)
 * 							lowest:  0 (0002)
 *
 * flow label				indicates the flow identifier, representing a bitstream or a group of bitstreams
 * (7 bits)					who's network resources are reserved according to an ADC or packet.
 * 							Range from 0-127, arbitrarily assigned in a session.
 */

/*
 * Initializes the MMTP demuxer
 *
 * 	TODO: chain to MPU sub-demuxer when MPT.payload_type_id=0x00
 *
 * 	read the first 32 bits to parse out version, packet_type and packet_id
 * 		if we think this is an MMPT packet, then wire up Demux and Control callbacks
 *
 *
 * TODO:  add mutex in p_demux->p_sys
 */

static int Open( vlc_object_t * p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = NULL;

    //bail if we have a short read, e.g. less than 4 bytes
//
//    if(vlc_stream_Peek( p_demux->s, &mmtpFirstQuad, 4) < 4 ) {
//        msg_Err(p_demux, "mmtp.open - peek was less than 4 bytes");
//        return VLC_EGENERIC;
//    }
//
//    //only support v=0 or v=1 for now
//    uint8_t mmtp_packet_version = mmtpFirstQuad[0] & 0xC0 >> 2;
//
//    if(mmtp_packet_version > 1) {
//        msg_Err(p_demux, "mmtp.open - version field is greater than 1, not supported, version is: %d", mmtp_packet_version);
//        return VLC_EGENERIC;
//    }
    p_demux->p_sys = p_sys = malloc( sizeof( demux_sys_t ) );
    if( !p_sys )
        return VLC_ENOMEM;

    memset( p_sys, 0, sizeof( demux_sys_t ) );

    p_demux->pf_demux = Demux;
    p_demux->pf_control = Control;


    //configure a m_fifo for when we have re-constituted MFU into MPU's to handoff to vlc_demux_chained_New
    //push down a new demux chain for the isobmff handoff
    //also vlc_demux_chained_Thread -?
    msg_Info(p_demux, "creating chained mp4 demuxer");

    p_sys->p_out_muxed = vlc_demux_chained_New( VLC_OBJECT(p_demux), "mp4", p_demux->out );
  //  vlc_clone (&p_sys->thread, Demux, p_demux, VLC_THREAD_PRIORITY_INPUT);
    msg_Info(p_demux, "mmtp_demuxer.open() - complete, ref at %p", (void*) p_sys->p_out_muxed);

    return  VLC_SUCCESS;
}

/**
 * Destroys the MMTP-demuxer, no-op for now
 */
static void Close( vlc_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    msg_Info(p_demux, "mmtp_demuxer.close()");
}

/**
 *
 * Un-encapsulate MMTP into type,
 * 	0x00 -> process payload as MPU, re-constituting MFU accordingly
 *
 *
 *
 *
     * dont rely on stream read, use vlc_stream_block for
    block_t *block = block_Alloc( MAX_UDP_BLOCKSIZE );
    if( unlikely(block == NULL) )
        return -1;

    int rd = vlc_stream_Read( p_demux->s, block->p_buffer, MAX_UDP_BLOCKSIZE );
    if ( rd <= 0 )
    {
        block_Release( block );
        return rd;
    }
    block->i_buffer = rd;

    size_t wr = sout_AccessOutWrite( out, block );
    if( wr != (size_t)rd )
    {
        msg_Err( p_demux, "cannot write data" );
        return -1;
    }
    return 1;

    **/
//messy
void* extract(uint8_t *bufPosPtr, uint8_t *dest, int size) {
	for(int i=0; i < size; i++) {
		dest[i] = *bufPosPtr++;
	}
	return bufPosPtr;
}

static int Demux( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    msg_Info(p_demux, "mmtp_demuxer.demux()");

    /* Get a new MMTP packet, use p_demux->s as the blocking reference and 1514 as the max mtu in udp.c*/
    //	vlc_stream_Block will try and fill MAX_MTU_SIZE instead of relying on the
    //    MMTP udp frame size

    uint8_t *raw_buf = malloc( MAX_MMTP_SIZE );
    uint8_t *buf = raw_buf; //use buf to walk thru bytes in extract method without touching rawBuf

    ssize_t mmtp_raw_packet_size = -1;
    //readPartial still reads a block_chain
  //  if( !( mmtp_raw_packet_size = vlc_stream_ReadPartial( p_demux->s, (void*)rawBuf, MAX_MMTP_SIZE ) ) )
	block_t *read_block;

    if( !( read_block = vlc_stream_ReadBlock( p_demux->s) ) )
    {
		msg_Err( p_demux, "mmtp_demuxer - access request returned null!");
		return VLC_DEMUXER_SUCCESS;
	}

    mmtp_raw_packet_size =  read_block->i_buffer;
    msg_Info(p_demux, "mmtp_demuxer: vlc_stream_readblock size is: %d", read_block->i_buffer);

	if( mmtp_raw_packet_size > MAX_MMTP_SIZE || mmtp_raw_packet_size < MIN_MMTP_SIZE) {
		msg_Err( p_demux, "mmtp_demuxer - size from UDP was under/over heureis/max, dropping %d bytes", mmtp_raw_packet_size);
		free(raw_buf); //only free raw_buf
		return VLC_DEMUXER_SUCCESS;
	} else {

		block_ChainExtract(read_block, raw_buf, MAX_MMTP_SIZE);

		uint8_t mmtp_packet_preamble[20];

		msg_Warn( p_demux, "buf pos before extract is: %p", (void *)buf);
		buf = extract(buf, mmtp_packet_preamble, 20);
		//msg_Warn( p_demux, "buf pos is now %p vs %p", new_buf_pos, (void *)buf);

		msg_Dbg( p_demux, "raw packet size is: %d, first byte: 0x%X", mmtp_raw_packet_size, mmtp_packet_preamble[0]);

	    uint8_t mmtp_packet_version = (mmtp_packet_preamble[0] & 0xC0) >> 6;
	    uint8_t packet_counter_flag = (mmtp_packet_preamble[0] & 0x20) >> 5;
	    uint8_t fec_type = (mmtp_packet_preamble[0] & 0x18) >> 3;

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

	    uint8_t flow_label = 0;

	    if(mmtp_packet_version == 0x00) {
	    	//after fec_type, with v=0, next bitmask is 0x4 >>2
	    	//0000 0010
	    	//V0CF E-XR
	    	mmtp_header_extension_flag = (mmtp_packet_preamble[0] & 0x2) >> 1;
	    	mmtp_rap_flag = mmtp_packet_preamble[0] & 0x1;

	    	//6 bits right aligned
	    	mmtp_payload_type = mmtp_packet_preamble[1] & 0x3f;
	    	if(mmtp_header_extension_flag & 0x1) {
	    		mmtp_header_extension_type = (mmtp_packet_preamble[16]) << 8 | mmtp_packet_preamble[17];
	    		mmtp_header_extension_length = (mmtp_packet_preamble[18]) << 8 | mmtp_packet_preamble[19];
	    	} else {
	    		//walk back by 4 bytes
	    		buf-=4;
	    	}
	    } else if(mmtp_packet_version == 0x01) {
	    	//bitmask is 0000 00
	    	//0000 0100
	    	//V1CF EXRQ
	    	mmtp_header_extension_flag = mmtp_packet_preamble[0] & 0x4 >> 2; //X
	    	mmtp_rap_flag = (mmtp_packet_preamble[0] & 0x2) >> 1;				//RAP
	    	mmtp_qos_flag = mmtp_packet_preamble[0] & 0x1;					//QOS
	    	//0000 0000
	    	//FEBI TYPE
	    	//4 bits for preamble right aligned

	    	mmtp_flow_identifer_flag = ((mmtp_packet_preamble[1]) & 0x80) >> 7;			//F
	    	mmtp_flow_extension_flag = ((mmtp_packet_preamble[1]) & 0x40) >> 6;			//E
	    	mmtp_header_compression = ((mmtp_packet_preamble[1]) &  0x20) >> 5; 		//B
	    	mmtp_indicator_ref_header_flag = ((mmtp_packet_preamble[1]) & 0x10) >> 4;	//I

	    	mmtp_payload_type = mmtp_packet_preamble[1] & 0xF;

	    	//TB 2 bits
			mmtp_type_of_bitrate = ((mmtp_packet_preamble[16] & 0x40) >> 6) | ((mmtp_packet_preamble[16] & 0x20) >> 5);

	    	//DS 3 bits
			mmtp_delay_sensitivity = ((mmtp_packet_preamble[16] & 0x10) >> 4) | ((mmtp_packet_preamble[16] & 0x8) >> 3) | ((mmtp_packet_preamble[16] & 0x4) >> 2);

	    	//TP 3 bits
			mmtp_transmission_priority =(( mmtp_packet_preamble[16] & 0x02) << 2) | ((mmtp_packet_preamble[16] & 0x1) << 1) | ((mmtp_packet_preamble[17] & 0x80) >>7);

	    	flow_label = mmtp_packet_preamble[17] & 0x7f;

	    	//header extension is offset by 2 bytes in v=1, so an additional block chain read is needed to get extension length
	       	if(mmtp_header_extension_flag & 0x1) {
	     		mmtp_header_extension_type = (mmtp_packet_preamble[18] << 8) | mmtp_packet_preamble[19];

				msg_Warn( p_demux, "mmtp_demuxer - dping mmtp_header_extension_length_bytes: %d",  mmtp_header_extension_type);

	     		uint8_t mmtp_header_extension_length_bytes[2];
	     		buf = extract(buf, mmtp_header_extension_length_bytes, 2);

	     		mmtp_header_extension_length = mmtp_header_extension_length_bytes[0] << 8 | mmtp_header_extension_length_bytes[1];
	    	} else {
	    		//walk back our buf position by 2 bytes to start for payload ddata
	    		buf-=2;

	    	}
	    } else {
			msg_Warn( p_demux, "mmtp_demuxer - unknown packet version of 0x%X", mmtp_packet_version);
			free( raw_buf );

			return VLC_DEMUXER_SUCCESS;
	    }

	    uint16_t  mmtp_packet_id			= mmtp_packet_preamble[2]  << 8  | mmtp_packet_preamble[3];
	    uint32_t mmtp_timestamp 		= mmtp_packet_preamble[4]  << 24 | mmtp_packet_preamble[5]  << 16 | mmtp_packet_preamble[6]   << 8 | mmtp_packet_preamble[7];
	    uint32_t packet_sequence_number = mmtp_packet_preamble[8]  << 24 | mmtp_packet_preamble[9]  << 16 | mmtp_packet_preamble[10]  << 8 | mmtp_packet_preamble[11];
	    uint32_t packet_counter 		= mmtp_packet_preamble[12] << 24 | mmtp_packet_preamble[13] << 16 | mmtp_packet_preamble[14]  << 8 | mmtp_packet_preamble[15];

		msg_Dbg( p_demux, "packet version: %d, payload_type: 0x%X, packet_id 0x%hu, timestamp: 0x%X, packet_sequence_number: 0x%X, packet_counter: 0x%X", mmtp_packet_version,
				mmtp_payload_type, mmtp_packet_id, mmtp_timestamp, packet_sequence_number, packet_counter);

		//if our header extension length is set, then block extract the header extension length, adn we should be at our payload data
		uint8_t *mmtp_header_extension_value = NULL;

		if(mmtp_header_extension_flag & 0x1) {
			//clamp mmtp_header_extension_length
			mmtp_header_extension_length = MIN(mmtp_header_extension_length, 2^16);
			msg_Warn( p_demux, "mmtp_demuxer - doing  mmtp_header_extension_flag with size: %d", mmtp_header_extension_length);

			mmtp_header_extension_value = malloc(mmtp_header_extension_length);
			//read the header extension value up to the extension length field 2^16
			buf = extract(buf, &mmtp_header_extension_value, mmtp_header_extension_length);
    	}

	    if(mmtp_payload_type == 0) {
	    	//pull the mpu and frag iformation

	    	uint8_t mpu_payload_length_block[2];
	    	uint16_t mpu_payload_length = 0;

			msg_Warn( p_demux, "buf pos before mpu_payload_length extract is: %p", (void *)buf);
	    	buf = extract(buf, &mpu_payload_length_block, 2);
	    	mpu_payload_length = (mpu_payload_length_block[0] << 8) | mpu_payload_length_block[1];
			msg_Warn( p_demux, "mmtp_demuxer - doing mpu_payload_length: %hu (0x%X 0x%X)",  mpu_payload_length, mpu_payload_length_block[0], mpu_payload_length_block[1]);

	    	uint8_t mpu_fragmentation_info;
			msg_Warn( p_demux, "buf pos before extract is: %p", (void *)buf);
	    	buf = extract(buf, &mpu_fragmentation_info, 1);

	    	uint8_t mpu_fragment_type = (mpu_fragmentation_info & 0xF0) >> 4;
	    	uint8_t mpu_timed_flag = (mpu_fragmentation_info & 0x8) >> 3;
	    	uint8_t mpu_fragmentation_indicator = (mpu_fragmentation_info & 0x6) >> 1;
	    	uint8_t mpu_aggregation_flag = (mpu_fragmentation_info & 0x1);

	    	uint8_t mpu_fragmentation_counter;
			msg_Warn( p_demux, "buf pos before extract is: %p", (void *)buf);
	    	buf = extract(buf, &mpu_fragmentation_counter, 1);
    	    msg_Info(p_demux, "mpu_fragmentation_counter: %d", mpu_fragmentation_counter);

    	    //re-fanagle
    	    uint8_t mpu_sequence_number_block[4];
    	    uint32_t mpu_sequence_number;
			msg_Warn( p_demux, "buf pos before extract is: %p", (void *)buf);

    	    buf = extract(buf, &mpu_sequence_number_block, 4);
			mpu_sequence_number = (mpu_sequence_number_block[0] << 24)  | (mpu_sequence_number_block[1] <<16) | (mpu_sequence_number_block[2] << 8) | (mpu_sequence_number_block[3]);
    	    msg_Info(p_demux, "mpu_sequence_number: %d", mpu_sequence_number);

	    	uint16_t data_unit_length = 0;
 	    	int remainingPacketLen = -1;

	    	do {
				//pull out aggregate packets data unit length
	    		int to_read_packet_length = -1;
	    		//mpu_fragment_type

				if(mpu_aggregation_flag) {
					uint8_t data_unit_length_block[2];
					buf = extract(buf, &data_unit_length_block, 2);
					data_unit_length = (data_unit_length_block[0] << 8) | (data_unit_length_block[1]);
					msg_Info(p_demux, "mpu_aggregation_flag:1, data_unit_length: %d", data_unit_length);
					to_read_packet_length = data_unit_length;
				} else {
					to_read_packet_length = mmtp_raw_packet_size - ((buf-raw_buf) * 8);
				}

				if(mpu_fragment_type != 0x2) {
					//read
					block_t *tmp_mpu_fragment = block_Alloc(to_read_packet_length);
					buf = extract(buf, tmp_mpu_fragment->p_buffer, data_unit_length);
					tmp_mpu_fragment->i_buffer = data_unit_length;
					processMpuPacket(p_demux, mmtp_packet_id, mpu_fragment_type, mpu_fragmentation_indicator, tmp_mpu_fragment);
					remainingPacketLen = mmtp_raw_packet_size - ((buf-raw_buf) * 8);

				} else {
					//we use the du_header field
					//parse data unit header here based upon mpu timed flag
					if(mpu_timed_flag) {
						//112 bits in aggregate, 14 bytes
						uint8_t timed_mfu_block[14];
						extract(buf, &timed_mfu_block, 14); //dont re-position buffer, treat this as a "peek"

						uint32_t movie_fragment_sequence_number = (timed_mfu_block[0] << 24) | (timed_mfu_block[1] << 16) | (timed_mfu_block[2]  << 8) | (timed_mfu_block[3]);
						uint32_t sample_numnber 				= (timed_mfu_block[4] << 24) | (timed_mfu_block[5] << 16) | (timed_mfu_block[6]  << 8) | (timed_mfu_block[7]);
						uint32_t offset 						= (timed_mfu_block[8] << 24) | (timed_mfu_block[9] << 16) | (timed_mfu_block[10] << 8) | (timed_mfu_block[11]);
						uint8_t priority 						= timed_mfu_block[12];
						uint8_t dep_counter						= timed_mfu_block[13];

						msg_Info(p_demux, "mpu mode -timed MFU, movie_fragment_seq_num: %zu, sample_num: %zu, offset: %zu, pri: %d, dep_counter: %d",
								movie_fragment_sequence_number, sample_numnber, offset, priority, dep_counter);
					} else {
						//only 32 bits
						uint8_t non_timed_mfu_block[4];
						uint32_t non_timed_mfu_item_id;
						buf = extract(buf, &non_timed_mfu_block, 4);
						non_timed_mfu_item_id = (non_timed_mfu_block[0] << 24) | (non_timed_mfu_block[1] << 16) | (non_timed_mfu_block[2] << 8) | non_timed_mfu_block[3];
						msg_Info(p_demux, "mpu mode - non-timed MFU, item_id is: %zu", non_timed_mfu_item_id);
					}

					msg_Dbg( p_demux, "before reading fragment packet:  %p", (void*)p_sys->p_mpu_block);

					block_t *tmp_mpu_fragment = block_Alloc(data_unit_length);
					buf = extract(buf, tmp_mpu_fragment->p_buffer, data_unit_length);
					tmp_mpu_fragment->i_buffer = data_unit_length;
					processMpuPacket(p_demux, mmtp_packet_id, mpu_fragment_type, mpu_fragmentation_indicator, tmp_mpu_fragment);
					remainingPacketLen = mmtp_raw_packet_size - ((buf-raw_buf) * 8);

				}

	    	} while(mpu_aggregation_flag && remainingPacketLen);

	    }

	}

	return VLC_DEMUXER_SUCCESS;
}
static int lastPacketId = -1;
void processMpuPacket(demux_t* p_demux, uint16_t mmtp_packet_id, uint8_t mpu_fragment_type, uint8_t mpu_fragmentation_indicator, block_t *tmp_mpu_fragment ) {

    demux_sys_t *p_sys = p_demux->p_sys;
    //if we have a new packet id, assume we can send the current payload off
    if(mmtp_packet_id >= lastPacketId) {
    	//send off if we have a pending mpu block
    	if(p_sys->p_mpu_block) {
    		msg_Info(p_demux, "sending p_mpu_block to chained decoder");

			vlc_demux_chained_Send(p_sys->p_out_muxed, p_sys->p_mpu_block);
			unsigned update = UINT_MAX;
			vlc_demux_chained_Control(p_sys->p_out_muxed, DEMUX_TEST_AND_CLEAR_FLAGS, &update);

    	}
		msg_Info(p_demux, "creating new p_mpu_block packetId: %hu, fragmentType: %d, fragmentationIndicator: %d, appending tmp_mpu_fragement to p_mpu_block", mmtp_packet_id, mpu_fragment_type, mpu_fragmentation_indicator);

		p_sys->p_mpu_block = block_Duplicate(tmp_mpu_fragment);
	    lastPacketId = mmtp_packet_id;

    } else {
		msg_Info(p_demux, "appending new p_mpu_block packetId: %hu, fragmentType: %d, fragmentationIndicator: %d, appending tmp_mpu_fragement to p_mpu_block", mmtp_packet_id, mpu_fragment_type, mpu_fragmentation_indicator);

    	//append
		block_ChainAppend(p_sys->p_mpu_block, tmp_mpu_fragment);

    }
    return;

	if(mpu_fragmentation_indicator == 0) {
		msg_Info(p_demux, "mpu_fragmentation_indicator = 0, pushing tmp_mpu_fragement to fifo");

		//push to fifo
		vlc_demux_chained_Send(p_sys->p_out_muxed, tmp_mpu_fragment);

	//		block_Release(tmp_mpu_fragment);

	} else if(mpu_fragmentation_indicator == 1) {
		//start of a new packet
		if(p_sys->p_mpu_block) {
			msg_Err(p_demux, " fragmentation_indicator is 1 but p_sys->p_mpu_block not null?");
			vlc_demux_chained_Send(p_sys->p_out_muxed, block_ChainGather(p_sys->p_mpu_block));
		} else {
			msg_Info(p_demux, "appending - mpu_fragmentation_indicator == 1");
		}
		p_sys->p_mpu_block = block_Duplicate(tmp_mpu_fragment);

	} else if (mpu_fragmentation_indicator == 2) {
		//middle of fragment, but must drop if we don't have a p_mpu_block pending
		if(p_sys->p_mpu_block) {
			block_ChainAppend(p_sys->p_mpu_block, tmp_mpu_fragment);
			msg_Info(p_demux, "appending - mpu_fragmentation_indicator == 2");
		} else {
			msg_Warn(p_demux, "DROPPING mpu_fragmentation_indicator==2 but no active p_mpu_block");
		}
	} else if(mpu_fragmentation_indicator == 3) {
		if(p_sys->p_mpu_block) {
			block_ChainAppend(p_sys->p_mpu_block, tmp_mpu_fragment);
			//append and gather and push - completion of fragment, push to fifo
			vlc_demux_chained_Send(p_sys->p_out_muxed, block_ChainGather(p_sys->p_mpu_block));
			//block_chainGather will release
			p_sys->p_mpu_block = NULL;
		} else {
			msg_Warn(p_demux, "DROPPING mpu_fragmentation_indicator==3 but no active p_mpu_block");
		}
	}


}

/** todo - add
 * DEMUX_SET_GROUP_DEFAULT ?
 *
 * DEMUX_FILTER_DISABLE
 *
 */
static int Control( demux_t *p_demux, int i_query, va_list args )
{
   // msg_Info(p_demux, "control: query is: %d", i_query);

    bool *pb;

    switch ( i_query )
    {
    	case DEMUX_CAN_SEEK:
        case DEMUX_CAN_PAUSE:
        case DEMUX_CAN_CONTROL_PACE:
            pb = va_arg ( args, bool* );
            *pb = false;
            break;


        case DEMUX_GET_PTS_DELAY:
        	return VLC_EGENERIC;

        case DEMUX_GET_META:
        case DEMUX_GET_SIGNAL:
        case DEMUX_GET_TITLE:
        case DEMUX_GET_SEEKPOINT:
        case DEMUX_GET_TITLE_INFO:
        case DEMUX_IS_PLAYLIST:
        	return VLC_EGENERIC;

        case DEMUX_GET_LENGTH:
			*va_arg ( args, vlc_tick_t * ) = 0;

			//	vlc_tick_from_sec( p_sys->frames_total * p_sys->frame_rate_denom / p_sys->frame_rate_num );
			break;

		case DEMUX_GET_TIME:
			*va_arg( args, vlc_tick_t * ) = 0;
			break;

        case DEMUX_GET_ATTACHMENTS:
        	return VLC_EGENERIC;
        	break;
//
//        case DEMUX_TEST_AND_CLEAR_FLAGS:
//        	unsigned *flags = va_arg(args, unsigned *);
//            flags = 1;
//        	break;

    }

    return VLC_SUCCESS; //demux_vaControlHelper( p_demux->s, 0, -1, 0, 1, i_query, args );

}
