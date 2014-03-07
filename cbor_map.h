#ifndef CBOR_MAP_H
#define CBOR_MAP_H

#include"ccbor.h"

#include<stddef.h>

struct cbor_mapentry_t{
	const struct cbor_t*const key;
	const struct cbor_t*const value;
};

struct cbor_map_t{
	struct cbor_t base;
	const size_t length;
	struct cbor_mapentry_t * const map;
};

extern int cbor_store_map(struct cbor_t*storage,const uint8_t,const int);

#endif

