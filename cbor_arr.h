#ifndef CBOR_ARR_H
#define CBOR_ARR_H

#include<stdint.h>
#include<stddef.h>

#include"ccbor.h"

struct cbor_arr_t{
	struct cbor_t base;
	const size_t length;
	struct cbor_t **const array;
};

extern int cbor_store_arr(struct cbor_t*,const uint8_t,const int);

#endif

