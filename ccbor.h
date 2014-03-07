#ifndef CCBOR_H
#define CCBOR_H

#include<stdint.h>

typedef enum cbor_major_t
{
	#define cbor_major_t_min cbor_major_uint
	cbor_major_uint = 0 <<5,
	cbor_major_nint = 1 <<5,
	cbor_major_bstr = 2 <<5,
	cbor_major_tstr = 3 <<5,
	cbor_major_arr = 4 <<5,
	cbor_major_map = 5 <<5,
	cbor_major_tag = 6 <<5,
	cbor_major_flt = 7 <<5,
	#define cbor_major_t_max cbor_major_flt
	
	#define cbor_major_mask cbor_major_uint | cbor_major_nint | cbor_major_bstr | cbor_major_tstr | cbor_major_arr | cbor_major_map | cbor_major_tag | cbor_major_flt
} cbor_major_t;

typedef enum cbor_additional_t
{
	cbor_additional_indefinite = 31,
} cbor_additional_t;

struct cbor_t {
	const cbor_major_t major;
	/*@only@*/
	/*@null@*/
	struct cbor_t*next;
};

/*@-exportlocal@*/

extern uint8_t cbor_major_of(uint8_t);
extern uint8_t cbor_additional_of(uint8_t);

extern void recursive_naive_cbor_free(/*@only@*//*@null@*/struct cbor_t*);

extern int(*const cbor_store[cbor_major_t_max])(struct cbor_t*,const uint8_t,const int);

extern int decode(const int stream, struct cbor_t*);

/*@+exportlocal@*/

#define cbor_BREAK 0xff

#endif

