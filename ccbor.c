#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define host_little_endian 1

uint16_t be16toh(const uint16_t bigendian)
{
	#if host_little_endian
	return ((bigendian&0x00ff) <<8) |
		((bigendian&0xff00) >>8);
	#endif
	return bigendian;
}

uint32_t be32toh(const uint32_t bigendian)
{
	#if host_little_endian
	return ((bigendian&0x000000ff) <<24) |
		((bigendian&0x0000ff00) <<8) |
		((bigendian&0x00ff0000) >>8) |
		((bigendian&0xff000000) >>24);
	#endif
	return bigendian;
}

uint64_t be64toh(const uint64_t bigendian)
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
	#endif
	return bigendian;
}

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

struct cbor_t {
	const cbor_major_t major;
	struct cbor_t*next;
};

struct cbor_uint_t {
	struct cbor_t base;
	const uint64_t value;
};

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
			v=be16toh(v16);
			break;
		}
		case 26:
		{
			uint32_t v32=0;
			(void)read(stream,&v32,sizeof(uint32_t));
			v=be32toh(v32);
			break;
		}
		case 27:
		{
			uint64_t v64=0;
			(void)read(stream,&v64,sizeof(uint64_t));
			v=be64toh(v64);
			break;
		}
		case 28:
		case 30:
			break;
	}
	return v;
}

int cbor_store_uint(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;
	
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

struct cbor_nint_t {
	struct cbor_t base;
	const uint64_t nvalue;
};

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
	if(n.nvalue==-1&&additional!=1)return 3;

	memcpy(fresh,&n,sizeof*fresh);

	storage->next=&fresh->base;
	return EXIT_SUCCESS;
}

struct cbor_bstr_t {
	struct cbor_t base;
	/* it is assumed size_t is at least as large as uint64_t */
	const size_t length;
	uint8_t const*const bytestring;
};

int cbor_store_bstr(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;
	{

		/* it is assumed size_t is at least as large as uint64_t */	
		size_t length=cbor_value_uint(additional,stream);

		// TODO: indefinite byte strings

		uint8_t*bytestring=malloc(length);

		if(bytestring==NULL)return 1;
		{
			if(read(stream,bytestring,length)<length)return 3;
			{
				struct cbor_bstr_t b= {
					.base= {
						.major=cbor_major_bstr,
						.next=NULL,
					},
					.length=length,
					.bytestring=bytestring,
				},*fresh=malloc(sizeof*fresh);

				if(fresh==NULL)return 1;
				memcpy(fresh,&b,sizeof*fresh);

				storage->next=&fresh->base;
			}
		}
		return EXIT_SUCCESS;
	}
}

struct cbor_tstr_t {
	struct cbor_t base;
	const size_t length;
	char const*const text;
};

int cbor_store_tstr(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;
	{
		size_t length=cbor_value_uint(additional,stream);

		// TODO: indefinite text strings

		char*text=malloc(length);

		if(text==NULL)return 1;
		{
			if(read(stream,text,length)<length)return 3;
			{
					struct cbor_tstr_t t= {
						.base= {
							.major=cbor_major_tstr,
							.next=NULL,
						},
						.length=length,
						.text=text,
					},*fresh=malloc(sizeof*fresh);

					if(fresh==NULL)return 1;
					memcpy(fresh,&t,sizeof*fresh);

					storage->next=&fresh->base;
			}
		}
		return EXIT_SUCCESS;
	}
}

struct cbor_tag_t {
	struct cbor_t base;
	const uint64_t value;
};

int cbor_store_tag(struct cbor_t*storage,const uint8_t additional, const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;

	struct cbor_tag_t t= {
		.base= {
			.major=cbor_major_tag,
			.next=NULL,
		},
		.value=cbor_value_uint(additional,stream),
	},*fresh=malloc(sizeof*fresh);

	if(fresh==NULL)return 1;

	memcpy(fresh,&t,sizeof*fresh);

	storage->next=&fresh->base;
	return EXIT_SUCCESS;
}

int(*const cbor_store[cbor_major_t_max])(struct cbor_t*,const uint8_t,const int stream) = {
	&cbor_store_uint,
	&cbor_store_nint,
	NULL,//&cbor_store_bstr,
	NULL,//&cbor_store_tstr,
	NULL,//&cbor_store_arr,
	NULL,//&cbor_store_map,
	&cbor_store_tag,
	NULL,//&cbor_store_flt,
};

uint8_t cbor_major_of(uint8_t item)
{
	return (item& (cbor_major_mask))>>5;
}

uint8_t cbor_additional_of(uint8_t item)
{
	return item& ~(cbor_major_mask);
}

int decode(const int stream,struct cbor_t*storage)
{
	int wat=0;
	struct cbor_t*store=storage;
	do
	{
		uint8_t item;
	
		if(read(stream,&item,sizeof item) < 1 )
		{
			break;
			return EXIT_FAILURE;
		}

		wat=cbor_store[cbor_major_of(item)](store,cbor_additional_of(item),stream);
		if(store!=NULL)store=store->next;
	}
	while(wat==0);
	return EXIT_SUCCESS;
}
