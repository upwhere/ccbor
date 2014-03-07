#include"cbor_int.h"

#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#ifndef host_little_endian
#define host_little_endian 1
#endif

static uint16_t be16to_h(const uint16_t bigendian)
{
	#if host_little_endian
	return ((bigendian&0x00ff) <<8) |
		((bigendian&0xff00) >>8);
	/*@notreached@*/
	#endif
	return bigendian;
}

static uint32_t be32to_h(const uint32_t bigendian)
{
	#if host_little_endian
	return ((bigendian&0x000000ff) <<24) |
		((bigendian&0x0000ff00) <<8) |
		((bigendian&0x00ff0000) >>8) |
		((bigendian&0xff000000) >>24);
	/*@notreached@*/
	#endif
	return bigendian;
}

static uint64_t be64to_h(const uint64_t bigendian)
{
	#if host_little_endian
	return ((bigendian&0x00000000000000ff) <<56) |
		((bigendian&0x000000000000ff00) <<40) |
		((bigendian&0x0000000000ff0000) <<24) |
		((bigendian&0x00000000ff000000) <<8) |
		((bigendian&0x000000ff00000000) >>8) |
		((bigendian&0x0000ff0000000000) >>24) |
		((bigendian&0x00ff000000000000) >>40) |
		((bigendian&0xff00000000000000) >>56);
	/*@notreached@*/
	#endif
	return bigendian;
}

uint64_t cbor_value_uint(const uint8_t additional,const int stream)
{
	uint64_t v=0;
	switch(additional) {
		default:
			return additional;
		case 24:
			(void)read(stream,&v,sizeof(uint8_t));
			break;
		case 25:
		{
			uint16_t v16=0;
			(void)read(stream,&v16,sizeof(uint16_t));
			v=be16to_h(v16);
			break;
		}
		case 26:
		{
			uint32_t v32=0;
			(void)read(stream,&v32,sizeof(uint32_t));
			v=be32to_h(v32);
			break;
		}
		case 27:
		{
			uint64_t v64=0;
			(void)read(stream,&v64,sizeof(uint64_t));
			v=be64to_h(v64);
			break;
		}
		case 28:
		case 30:
		case 31:
			break;
	}
	return v;
}

int cbor_store_uint(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;
	{
		
		struct cbor_uint_t c= {
			.base= {
				.major=cbor_major_uint,
				.next=NULL,
			},
			.value=cbor_value_uint(additional,stream),
		},*fresh=malloc(sizeof*fresh);

		if(fresh==NULL)return 1;
		
		memcpy(fresh,&c,sizeof*fresh);

		storage->next=&fresh->base;
		return EXIT_SUCCESS;
	}
}

int cbor_store_nint(struct cbor_t*storage,const uint8_t additional, const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;

	struct cbor_nint_t n= {
		.base= {
			.major=cbor_major_nint,
			.next=NULL,
		},
		.nvalue=cbor_value_uint(additional,stream),
	},*fresh=malloc(sizeof*fresh);

	if(fresh==NULL)return 1;
	if(n.nvalue==(uint64_t)-1&&additional!=1)
	{
		free(fresh);
		return 3;
	}

	memcpy(fresh,&n,sizeof*fresh);

	storage->next=&fresh->base;
	return EXIT_SUCCESS;
}
