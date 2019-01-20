/*
 * atsc3_utils.h
 *
 *  Created on: Jan 6, 2019
 *      Author: jjustman
 */

#ifndef ATSC3_UTILS_H_
#define ATSC3_UTILS_H_

#define uS 1000000ULL

#include <stdlib.h>


#define _ATSC3_UTILS_PRINTLN(...) printf(__VA_ARGS__);printf("\n")
#define _ATSC3_UTILS_PRINTF(...)  printf(__VA_ARGS__);

#define _ATSC3_UTILS_ERROR(...)   printf("%s:%d:ERROR:",__FILE__,__LINE__);_ATSC3_UTILS_PRINTLN(__VA_ARGS__);
#define _ATSC3_UTILS_WARN(...)    printf("%s:%d:WARN :",__FILE__,__LINE__);_ATSC3_UTILS_PRINTLN(__VA_ARGS__);
#define _ATSC3_UTILS_INFO(...)    printf("%s:%d:INFO :",__FILE__,__LINE__);_ATSC3_UTILS_PRINTLN(__VA_ARGS__);
#define _ATSC3_UTILS_DEBUG(...)   printf("%s:%d:DEBUG:",__FILE__,__LINE__);_ATSC3_UTILS_PRINTLN(__VA_ARGS__);

#define _ATSC3_UTILS_TRACE(...)   printf("%s:%d:TRACE:",__FILE__,__LINE__);_ATSC3_UTILS_PRINTLN(__VA_ARGS__);
#define _ATSC3_UTILS_TRACEF(...)  printf("%s:%d:TRACE:",__FILE__,__LINE__);_ATSC3_UTILS_PRINTF(__VA_ARGS__);
#define _ATSC3_UTILS_TRACEA(...)  _ATSC3_UTILS_PRINTF(__VA_ARGS__);
#define _ATSC3_UTILS_TRACEN(...)  _ATSC3_UTILS_PRINTLN(__VA_ARGS__);

void strreverse(char* begin, char* end);
void itoa(int value, char* str, int base);

//key=value or key="value" attribute par collection parsing and searching
typedef struct kvp {
	char* key;
	char* val;
} kvp_t;

typedef struct kvp_collection {
	kvp_t **kvp_collection;
	int 	size_n;
} kvp_collection_t;

kvp_collection_t* kvp_parse_string(uint8_t *input_string);
char* kvp_find_key(kvp_collection_t *collection, char* key);



#endif /* ATSC3_UTILS_H_ */
