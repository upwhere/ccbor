#ifndef CBOR_STR_H
#define CBOR_STR_H

#include<stddef.h>
#include<stdint.h>

#include"ccbor.h"

struct cbor_bstr_t {
	struct cbor_t base;
	const size_t length;
	uint8_t const*const string;
};

struct cbor_tstr_t {
	struct cbor_t base;
	const size_t length;
	char const*const string;
};

extern int cbor_store_bstr(struct cbor_t*,const uint8_t,const int);
extern int cbor_store_tstr(struct cbor_t*,const uint8_t,const int);

#endif

