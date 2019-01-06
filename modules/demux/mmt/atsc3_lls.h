/*
 * astc3_lls.h
 *
 *  Created on: Jan 5, 2019
 *      Author: jjustman
 */

#ifndef MODULES_DEMUX_MMT_ASTC3_LLS_H_
#define MODULES_DEMUX_MMT_ASTC3_LLS_H_

#include <stdlib.h>
#include <stdio.h>
#include "zlib.h"


#define println(...) printf(__VA_ARGS__);printf("\n")

/***
 * From < A/331 2017 - Signaling Delivery Sync > https://www.atsc.org/wp-content/uploads/2017/12/A331-2017-Signaling-Deivery-Sync-FEC-3.pdf
 * LLS shall be transported in IP packets with address:
 * 224.0.23.60 and destination port 4937/udp
 *
 *
 *UDP/IP packets delivering LLS data shall be formatted per the bit stream syntax given in Table 6.1 below.
 *UDP/IP The first byte of every UDP/IP packet carrying LLS data shall be the start of an LLS_table().
 *UDP/IP  The maximum length of any LLS table is limited by the largest IP packet that can be delivered from the PHY layer, 65,507 bytes5.
 *UDP/IP
 *      Syntax
 *

Syntax							Bits			Format
------							----			------
LLS_table() {

	LLS_table_id 				8
	LLS_group_id 				8
	group_count_minus1 			8
	LLS_table_version 			8
	switch (LLS_table_id) {
		case 0x01:
			SLT					var
			break;
		case 0x02:
			RRT					var
			break;
		case 0x03:
			SystemTime			var
			break;
		case 0x04:
			AEAT 				var
			break;
		case 0x05:
			OnscreenMessageNotification	var
			break;
		default:
			reserved			var
	}
}

No. of Bits
8 8 8 8
var var var var var var
Format
uimsbf uimsbf uimsbf uimsbf
Sec. 6.3
See Annex F Sec. 6.4 Sec. 6.5 Sec. 6.6
     }
 *
 */

typedef struct llt_xml_payload {
	uint8_t *xml_payload;
	uint xml_payload_size;


} lls_xml_payload_t;

typedef struct lls_xml_payload {

} slt_table_t;

typedef struct lls_xml_payload rrt_table_t;
typedef struct lls_xml_payload system_time_table_t;
typedef struct lls_xml_payload aeat_table_t;
typedef struct lls_xml_payload on_screen_message_notification_t;
typedef struct lls_xml_payload lls_reserved_table_t;

typedef struct lls_table {
	uint8_t	lls_table_id;
	uint8_t	lls_group_id;
	uint8_t group_count_minus1;
	uint8_t	lls_table_version;
	lls_xml_payload_t					raw_xml;

	union {

		slt_table_t							slt_table;
		rrt_table_t							rrt_table;
		system_time_table_t					system_time_table;
		aeat_table_t						aeat_table;
		on_screen_message_notification_t	on_screen_message_notification;
		lls_reserved_table_t				lls_reserved_table;
	};

} lls_table_t;

lls_table_t* lls_create_base_table(uint8_t* lls, int size) {

	lls_table_t *base_table = calloc(1, sizeof(lls_table_t));

	//read first 32 bytes in
	base_table->lls_table_id = lls[0];
	base_table->lls_group_id = lls[1];
	base_table->group_count_minus1 = lls[2];
	base_table->lls_table_version = lls[3];

	int remaining_payload_size = (size > 65531) ? 65531 : size;

	uint8_t *temp_gzip_payload = calloc(size, sizeof(uint8_t));
	FILE *f = fopen("slt.gz", "w");

	for(int i=4; i < remaining_payload_size; i++) {
		printf("i:0x%x ", lls[i]);
		fwrite(&lls[i], 1, 1, f);
		temp_gzip_payload[i-4] = lls[i];
	}
	base_table->raw_xml.xml_payload = temp_gzip_payload;
	base_table->raw_xml.xml_payload_size = remaining_payload_size -4;

	printf("first 4 hex: 0x%x 0x%x 0x%x 0x%x", temp_gzip_payload[0], temp_gzip_payload[1], temp_gzip_payload[2], temp_gzip_payload[3]);
	//remainder of playload is gzip'd, so read until size

	return base_table;
}

void lls_dump_base_table(lls_table_t *base_table) {
	println("base table:");
	println("-----------");
	println("lls_table_id:			%d	(0x%x)", base_table->lls_table_id, base_table->lls_table_id);
	println("lls_group_id: 			%d	(0x%x)", base_table->lls_group_id, base_table->lls_group_id);
	println("group_count_minus1: 	%d	(0x%x)", base_table->group_count_minus1, base_table->group_count_minus1);
	println("lls_table_version:%d	(0x%x)", base_table->lls_table_version, base_table->lls_table_version);
	println("(anon) xml payload : size %d", 	base_table->raw_xml.xml_payload_size);

}

#define CHUNK 8192

int unzip_gzip_payload(uint8_t *payload, uint payload_size) {

    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);

	if (ret != Z_OK)
	   return ret;


	do {
		strm.avail_in = payload_size;
//		if (ferror(source)) {
//			(void)inflateEnd(&strm);
//			return Z_ERRNO;
//		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = payload;

		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;

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

			have = CHUNK - strm.avail_out;
			for(int i=0; i < have; i++)
				printf("%c", out[i]);
//			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
//				(void)inflateEnd(&strm);
			return Z_ERRNO;

		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);


	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;

}
/**
 *
 * Raw SLT example:

0000   01 01 00 02 1f 8b 08 08 92 17 18 5c 00 03 53 4c   ...........\..SL
0010   54 00 b5 d5 5b 6f 82 30 14 00 e0 f7 fd 0a d2 e7   T.µÕ[o.0..à÷ý.Òç
0020   0d 4a 41 37 0d 60 9c 9a c5 44 8d 09 2e d9 9b a9   .JA7.`..ÅD...Ù.©
0030   d0 61 17 68 5d 5b cd fc f7 3b a8 cb e2 bc 44 16   Ða.h][Íü÷;¨Ëâ¼D.
0040   7d 22 9c 4b cf e9 f7 00 41 eb ab c8 ad 15 53 9a   }".KÏé÷.Aë«È..S.
0050   4b 11 22 d7 c6 c8 62 22 91 29 17 59 88 96 e6 fd   K."×ÆÈb".).Y..æý
0060   e1 09 b5 a2 bb 20 1e 4c 2c a8 14 3a 44 86 66 4d   á.µ¢» .L,¨.:D.fM
0070   6a 74 62 4b 95 dd 13 ec d6 9b 6f c3 41 9c cc 59   jtbK.Ý.ìÖ.oÃA.ÌY
0080   41 b5 d3 9e c4 1d cf e9 b2 9c c3 99 6b 07 da 1c   AµÓ.Ä.Ïé².Ã.k.Ú.
0090   38 d3 41 d6 4c f3 34 44 35 8c a2 20 66 6a c5 13   8ÓAÖLó4D5.¢ fjÅ.
00a0   66 e9 ed b3 0f 71 17 63 17 59 59 2e 67 34 df a5   féí³.q.c.YY.g4ß¥
00b0   fb 5d 98 af c4 66 54 73 57 ca 53 78 65 05 9b 16   û].¯ÄfTsWÊSxe...
00c0   85 99 42 43 41 3f a4 ea cc a9 10 2c 1f c9 f2 18   ..BCA?¤êÌ©.,.Éò.
00d0   88 71 b1 1f 43 3f 83 3a d0 9b 49 b5 de c6 e6 52   .q±.C?.:Ð.IµÞÆæR
00e0   99 dd a8 11 2d 58 88 da 93 de b0 67 0d 87 13 ab   .Ý¨.-X.Ú.Þ°g...«
00f0   4c e7 26 5e 25 31 fb 1c 2d 8b 10 95 5b 3f 2b 49   Lç&^%1û.-...[?+I
0100   d3 84 ea 4d 9c 67 82 e6 40 04 75 7a ac a4 91 89   Ó.êM.g.æ@.uz¬¤..
0110   cc 43 44 ca 3e dd 65 da 70 41 0d 80 f6 17 ed 34   ÌCDÊ>ÝeÚpA..ö.í4
0120   55 4c 83 1a f1 1a 36 a9 d5 6c 17 db ee df b2 d7   UL..ñ.6©Õl.Ûîß²×
0130   74 31 86 6d 80 67 eb 00 d9 58 2e 15 18 fc f6 bb   t1.m.gë.ÙX...üö»
0140   8f c4 76 eb 36 c1 65 bf 13 05 ce 6e f7 53 9c a4   .Ävë6Áe¿..În÷S.¤
0150   2a 27 b9 8c 93 54 e7 24 b7 e5 3c 28 db e3 24 d7   *'¹..Tç$·å<(Ûã$×
0160   e1 f4 aa 72 7a 97 71 7a d5 39 bd db 72 7a 67 39   áôªrz.qzÕ9½Ûrzg9
0170   bd eb 70 fa 55 39 fd cb 38 fd ea 9c fe 6d 39 fd   ½ëpúU9ýË8ýê.þm9ý
0180   b3 9c fe 15 38 6b 18 37 2e e3 64 3a 3b e2 e3 1f   ³.þ.8k.7.ãd:;âã.
0190   f3 e9 c5 2f ff 74 39 f8 ba 1d 71 21 d8 6e 9c 76   óéÅ/ÿt9øº.q!Øn.v
01a0   21 9b 0b 55 73 29 ff 34 d1 dd 37 2e 0e fb 8f ce   !..Us)ÿ4ÑÝ7..û.Î
01b0   06 00 00                                          ...

Raw SystemTime message:
0000   01 00 5e 00 17 3c 00 1c 42 22 fa 9f 08 00 45 00   ..^..<..B"ú...E.
0010   00 dd 01 00 40 00 01 11 c0 27 c0 a8 00 04 e0 00   .Ý..@...À'À¨..à.
0020   17 3c 90 8b 13 49 00 c9 2d 76 03 01 00 01 1f 8b   .<...I.É-v......
0030   08 08 97 17 18 5c 00 03 53 79 73 74 65 6d 54 69   .....\..SystemTi
0040   6d 65 00 35 8d cb 0a 82 40 14 40 f7 7e c5 70 f7   me.5.Ë..@.@÷~Åp÷
0050   7a 0b 89 22 7c 10 15 14 28 05 63 50 cb 61 bc 3e   z.."|...(.cPËa¼>
0060   60 1c c3 b9 66 fe 7d 6e da 1e 38 e7 44 e9 b7 33   `.Ã¹fþ}nÚ.8çDé·3
0070   e2 43 83 6b 7b 1b c3 3a 58 81 20 ab fb b2 b5 75   âC.k{.Ã:X. «û²µu
0080   0c 23 57 fe 0e d2 c4 8b e4 ec 98 ba a2 ed 48 2c   .#Wþ.ÒÄ.äì.º¢íH,
0090   82 75 31 34 cc ef 3d e2 34 4d 81 62 a7 83 7e a8   .u14Ìï=â4M.b§.~¨
00a0   f1 99 67 52 37 d4 29 87 87 42 1e 43 3c 91 69 97   ñ.gR7Ô)..B.C<.i.
00b0   f8 8c f2 25 8b 6b 7e c6 65 80 20 f4 38 0c 64 f9   ø.ò%.k~Æe. ô8.dù
00c0   c1 fa 56 55 8e 38 86 70 0b 62 64 9d f5 5a 99 3f   ÁúVU.8.p.bd.õZ.?
00d0   f3 ef c5 e6 02 a2 74 92 15 8f cb b2 52 c6 11 60   óïÅæ.¢t...Ë²RÆ.`
00e0   e2 fd 00 35 18 c1 1f b6 00 00 00                  âý.5.Á.¶...

 *
 */

//slt with packet_id=2
static char* __get_test_slt()					{ return "010100021f8b08089217185c0003534c5400b5d55b6f82301400e0f7fd0ad2e70d4a41370d609c9ac5448d092ed99ba9d06117685d5bcdfcf73ba8cbe2bc44167d229c4bcfe9f70041ebabc8ad15539a4b1122d7c6c86222912917598896e6fde109b5a2bb201e4c2ca8143a4486664d6a74624b95dd13ecd69b6fc3419ccc5941b5d39ec41dcfe9b29cc3996b07da1c38d341d64cf33444358ca220666ac51366e9edb30f7117631759592e6734dfa5fb5d98afc466547357ca537865059b1685994243413fa4eacca9102c1fc9f2188871b11f433f833ad09b49b5dec6e65299dda8112d5888da93deb0670d8713ab4ce7265e2531fb1c2d8b10955b3f2b49d384ea4d9c6782e64004757aaca49189cc4344ca3edd65da70410d80f617ed34554c831af11a36a9d56c17dbeedfb2d77431866d8067eb00d9582e1518fcf6bb8fc476eb36c165bf1305ce6ef7539ca42a27b98c9354e724b7e53c28dbe324d7e1f4aa727a97717ad539bddb727a6739bdeb70fa5539fdcb38fdea9cfe6d39fdb39cfe15386b18372ee3643a3be2e31ff3e9c52fff7439f8ba1d7121d86e9c76219b0b557329ff34d1dd372e0efb8fce060000";}
//system_time_message with packet_id=1
static char* __get_test_system_time_message()	{ return "030100011f8b08089717185c000353797374656d54696d6500358dcb0a82401440f77ec570f77a0b89227c10151428056350cb61bc3e601cc3b966fe7d6eda1e38e744e9b733e243836b7b1bc33a588120abfbb2b5750c2357fe0ed2c48be4ec98baa2ed482c82753134ccef3de2344d8162a7837ea8f199675237d4298787421e433c916997f88cf2258b6b7ec6658020f4380c64f9c1fa56558e3886700b62649df55a993ff3efc5e602a27492158fcbb252c61160e2fd003518c11fb6000000"; }




#endif /* MODULES_DEMUX_MMT_ASTC3_LLS_H_ */
