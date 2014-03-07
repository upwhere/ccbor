#ifndef CBOR_INT_H
#define CBOR_INT_H

#include<stdint.h>

#include"ccbor.h"

struct cbor_uint_t {
	struct cbor_t base;
	const uint64_t value;
};

struct cbor_nint_t {
	struct cbor_t base;
	const uint64_t nvalue;
};

/*@-exportlocal@*/

extern uint64_t cbor_value_uint(const uint8_t,const int);

extern int cbor_store_uint(struct cbor_t*,const uint8_t,const int);

extern int cbor_store_nint(struct cbor_t*,const uint8_t, const int);

/*@+exportlocal@*/

#endif
