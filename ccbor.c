#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>

#define host_little_endian 1

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

uint8_t cbor_major_of(uint8_t item)
{
	return (item& (cbor_major_mask))>>5;
}

uint8_t cbor_additional_of(uint8_t item)
{
	return item& ~(cbor_major_mask);
}

struct cbor_t {
	const cbor_major_t major;
	struct cbor_t*next;
};

int(*const cbor_store[cbor_major_t_max])(struct cbor_t*,const uint8_t,const int stream);

#define cbor_BREAK 0xff

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
	if(n.nvalue==(uint64_t)-1&&additional!=1)
	{
		free(fresh);
		return 3;
	}

	memcpy(fresh,&n,sizeof*fresh);

	storage->next=&fresh->base;
	return EXIT_SUCCESS;
}

struct cbor_bstr_t {
	struct cbor_t base;
	/* it is assumed size_t is at least as large as uint64_t */
	const size_t length;
	uint8_t const*const string;
};

static void recursive_naive_cbor_free(struct cbor_t*listitem)
{
	if(listitem==NULL)return;
	recursive_naive_cbor_free(listitem->next);
	free(listitem);
	return;
}

static int store_definite_bstr(struct cbor_t*storage,const uint8_t additional, const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;

	/* it is assumed size_t is at least as large as uint64_t */
	/** How many bytes follow */
	size_t length=cbor_value_uint(additional,stream);

	uint8_t*string=malloc(length);

	if(string==NULL)return 1;
	if(read(stream,string,length)<(ssize_t)length)
	{
		free(string);
		return 3;
	}
	{
		struct cbor_bstr_t b= {
			.base= {
				.major=cbor_major_bstr,
				.next=NULL,
			},
			.length=length,
			.string=string,
		},*fresh=malloc(sizeof*fresh);

		if(fresh==NULL)
		{
			free(string);
			return 1;
		}
		memcpy(fresh,&b,sizeof*fresh);

		storage->next=&fresh->base;
	}
	return EXIT_SUCCESS;
}

#define GENERATE_STORE_STRINGLIKE(major_shorthand,stringlike_type,heed_nullterm) int cbor_store_##major_shorthand(struct cbor_t*storage,const uint8_t additional,const int stream) \
{ \
	if(storage==NULL || storage->next!=NULL)return 2; \
 \
	if(additional!=cbor_additional_indefinite) \
	{ \
		return store_definite_##major_shorthand(storage,additional,stream); \
	} \
	else \
	{ \
		/* we'll assume the entire indefinite array fits into memory because what else are we going to do with it? */ \
		size_t total_length=0; \
		stringlike_type *str,*strindex; \
		struct cbor_t indefinite,*next=&indefinite; \
		while(true) \
		{ \
			int store_attempt_ret; \
			uint8_t item; \
			if(read(stream,&item,sizeof item) < (ssize_t) sizeof item)return 3; \
 \
			if(item==cbor_BREAK)break; \
 \
			if(cbor_major_of(item)!=(cbor_major_##major_shorthand>>5))return 3; \
 \
				/* FIXME: free the previously stored subitems */\
			if((store_attempt_ret=store_definite_bstr(next,cbor_additional_of(item),stream))!=EXIT_SUCCESS)return store_attempt_ret; \
 \
			next=next->next; \
			total_length+=((struct cbor_bstr_t*)next)->length; \
		} \
 \
		if((strindex=str=malloc(total_length+ (heed_nullterm?sizeof'\0' :0) ) ) ==NULL)return 1; \
 \
		/* Rewind to first element */ \
		next=&indefinite; \
 \
		while((next=next->next)!=NULL) \
		{ \
			memcpy( \
				strindex, \
				((struct cbor_##major_shorthand##_t*)next)->string, \
				((struct cbor_##major_shorthand##_t*)next)->length); \
			free((stringlike_type*)((struct cbor_##major_shorthand##_t*)next)->string); \
			strindex+=((struct cbor_##major_shorthand##_t*)next)->length; \
		} \
		if(heed_nullterm)*strindex='\0'; \
 \
		/* Individual strings already freed */ \
		recursive_naive_cbor_free(indefinite.next); \
 \
		{ \
			struct cbor_##major_shorthand##_t s={ \
				.base={ \
					.major=cbor_major_##major_shorthand, \
					.next=NULL, \
				}, \
				.length=total_length, \
				.string=str, \
			},*fresh=malloc(sizeof*fresh); \
 \
			if(fresh==NULL) \
			{ \
				free(str); \
				return 1; \
			} \
 \
			memcpy(fresh,&s,sizeof*fresh); \
			storage->next=&fresh->base; \
		} \
 \
		return EXIT_SUCCESS; \
	} \
}

GENERATE_STORE_STRINGLIKE(bstr,uint8_t,false)

struct cbor_tstr_t {
	struct cbor_t base;
	const size_t length;
	char const*const string;
};

static int store_definite_tstr(struct cbor_t*storage,const uint8_t additional, const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;

	/* it is assumed size_t is at least as large as uint64_t */
	/** How many bytes follow */
	size_t length=cbor_value_uint(additional,stream);

	uint8_t*bytestring=calloc(length+1,sizeof*bytestring);

	if(bytestring==NULL)return 1;
	if(read(stream,bytestring,length)<(ssize_t)length)
	{
		free(bytestring);
		return 3;
	}
	{
		struct cbor_tstr_t t= {
			.base= {
				.major=cbor_major_tstr,
				.next=NULL,
			},
			.length=length+sizeof'\0',
			.string=(char*)bytestring,
		},*fresh=malloc(sizeof*fresh);

		if(fresh==NULL)return 1;

		memcpy(fresh,&t,sizeof*fresh);
		
		storage->next=&fresh->base;
	}
	return EXIT_SUCCESS;
}

GENERATE_STORE_STRINGLIKE(tstr,char,true)

struct cbor_arr_t{
	struct cbor_t base;
	const size_t length;
	struct cbor_t **const array;
};

int cbor_store_definite_arr(struct cbor_t*storage, const uint8_t additional, const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;

	const size_t length=cbor_value_uint(additional,stream);
	struct cbor_t **array = malloc(length*sizeof(struct cbor_t const *));

	// storage needs a head.
	struct cbor_t head= {
		.major=0xFF,
		.next=NULL,
	} , *element= &head;

	int store_status;
	
	for(size_t i=0;i<length;i++)
	{
		uint8_t item;
		if(read(stream,&item,sizeof item)<(ssize_t)sizeof item)
		{
			free(array);
			return EXIT_FAILURE;
		}

		if((store_status=cbor_store[cbor_major_of(item)](element,cbor_additional_of(item),stream))!=EXIT_SUCCESS)
		{
			free(array);
			return store_status;
		}

		array[i]=element->next;
		element->next=NULL;
	}
	{
		struct cbor_arr_t a= {
			.base= {
				.major=cbor_major_arr,
				.next=NULL,
			} ,
			.length=length,
			.array=array,
		} , *fresh= malloc(sizeof*fresh);

		if(fresh==NULL)
		{
			free(array);
			return 1;
		}

		memcpy(fresh, &a,sizeof*fresh);

		storage->next=&fresh->base;
	}

	return EXIT_SUCCESS;
}

int cbor_store_arr(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;
	if(additional!=cbor_additional_indefinite)
	{
		return cbor_store_definite_arr(storage,additional,stream);
	}
	{
		size_t total_length=0;
		struct cbor_t head= {
			.major=0xFF,
			.next=NULL,
		} , *element= &head;
		struct cbor_t**assembled,**assembled_index;

		while(true)
		{
			uint8_t item;
			int store_attempt_ret;
			if(read(stream,&item,sizeof item) < (ssize_t) sizeof item)return 3;

			if(item==cbor_BREAK)break;

			if( (store_attempt_ret=cbor_store[cbor_major_of(item) ] (element,cbor_additional_of(item) ,stream) ) !=EXIT_SUCCESS)
			{
				// FIXME: free the previously stored subitems, here and at the other returns.
				return store_attempt_ret;
			}

			element=element->next;
			total_length++;
		}

		if( (assembled=malloc(total_length* (sizeof(struct cbor_t const * ) ) ) ) ==NULL)return 1;

		element= &head;
		assembled_index= assembled;

		while( (element=element->next) !=NULL)
		{
			*assembled_index= element;
			assembled_index++;
		}
		{
			struct cbor_arr_t a= {
				.base= {
					.major=cbor_major_arr,
					.next=NULL,
				} ,
				.length=total_length,
				.array=assembled,
			} , *fresh= malloc(sizeof*fresh);

			if(fresh==NULL)
			{
				free(assembled);
				return 1;
			}

			memcpy(fresh, &a,sizeof*fresh);
			storage->next=&fresh->base;
		}
	}
	return EXIT_SUCCESS;
}

struct cbor_mapentry_t{
	const struct cbor_t*const key;
	const struct cbor_t*const value;
};

struct cbor_map_t{
	struct cbor_t base;
	const size_t length;
	struct cbor_mapentry_t * const map;
};

static int cbor_store_mapentry(struct cbor_mapentry_t*storage,const uint8_t key,const int stream)
{
	if(storage==NULL)return 2;
	if(key==cbor_BREAK)return 3;
	{
		struct cbor_t head= {
			.major=0xFF,
			.next=NULL,
		};
		struct cbor_t*pair=&head;

		int store_attempt_ret;
		uint8_t item;

		if( (store_attempt_ret=cbor_store[cbor_major_of(key) ] (pair,cbor_additional_of(key),stream) ) !=EXIT_SUCCESS)return store_attempt_ret;

		if(read(stream,&item,sizeof item) < (ssize_t) sizeof item)return 3;
		// Break may not occur between a key and a value
		if(item==cbor_BREAK)
		{
			store_attempt_ret=3;
			goto free;
		}

		pair=pair->next;
		if( (store_attempt_ret=cbor_store[cbor_major_of(item) ] (pair,cbor_additional_of(item) ,stream) ) !=EXIT_SUCCESS)goto free;
		
		pair=&head;
		{
			struct cbor_mapentry_t e= {
				.key=pair->next,
				.value=pair->next->next,
			};

			memcpy(storage,&e,sizeof*storage);
		}
		return EXIT_SUCCESS;free:
		{
			//FIXME: free whatever was stored
			return store_attempt_ret;
		}
	}
}

static int cbor_store_definite_map(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL||storage->next!=NULL)return 2;
	{
		const size_t length=cbor_value_uint(additional,stream);

		struct cbor_mapentry_t*const map=malloc(length*(sizeof*map));
		struct cbor_mapentry_t*map_index=map;
		int store_attempt_ret;
		if(map==NULL)return 1;

		//Grab all map entries
		for(size_t i=0;i<length;i++ )
		{
			uint8_t item;
			if(read(stream,&item,sizeof item) < (ssize_t) sizeof item)
			{
				store_attempt_ret=3;
				goto free;
			}
			if( (store_attempt_ret=cbor_store_mapentry(map_index,item,stream) ) !=EXIT_SUCCESS)goto free;
			++map_index;
		}

		{
			struct cbor_map_t m= {
				.base= {
					.major=cbor_major_map,
					.next=NULL,
				},
				.length=length,
				.map=map,
			} , *fresh= malloc(sizeof*fresh);
			if(fresh==NULL)
			{
				store_attempt_ret=1;
				goto free;
			}
			memcpy(fresh,&m,sizeof*fresh);

			storage->next=&fresh->base;
		}

		return EXIT_SUCCESS;free:
		{
			//FIXME: free whatever was constructed
			return store_attempt_ret;
		}
	}
}

static int cbor_store_map(struct cbor_t*storage,const uint8_t additional,const int stream)
{
	if(storage==NULL||storage->next!=NULL)return 2;

	if(additional!=cbor_additional_indefinite)
	{
		return cbor_store_definite_map(storage,additional,stream);
	}
	{
		struct linkedlist_t{
			struct cbor_mapentry_t*e;
			struct linkedlist_t*next;
		}head= {
			.e=NULL,
			.next=NULL,
		} , *list_index=&head;

		long unsigned int entry_count=0;
		int store_attempt_ret;

		// Grab pairs from stream
		while(true)
		{
			uint8_t item;

			if(read(stream,&item,sizeof item) < (ssize_t) sizeof item)
			{
				store_attempt_ret=3;
				goto free;
			}

			if(item==cbor_BREAK)break;

			list_index->next=malloc(sizeof list_index->next);
			if(list_index->next==NULL)
			{
				store_attempt_ret=1;
				goto free;
			}
			list_index=list_index->next;
			
			list_index->e=malloc(sizeof list_index->e);
			if(list_index->e==NULL)
			{
				store_attempt_ret=1;
				goto free;
			}

			if( (store_attempt_ret=cbor_store_mapentry(list_index->e,item,stream) ) !=EXIT_SUCCESS)goto free;
			
			++entry_count;
		}

		// Copy into map.
		{
			struct cbor_mapentry_t*destination=malloc((sizeof*destination)*entry_count),*destination_index=destination;
			if(destination==NULL)
			{
				store_attempt_ret=1;
				goto free;
			}

			list_index=&head;
			while((list_index=list_index->next)!=NULL)
				memcpy(destination_index++,list_index->e,sizeof*destination_index);

			{
				struct cbor_map_t e= {
					.base= {
						.major=cbor_major_map,
						.next=NULL,
					},
					.length=entry_count,
					.map=destination,
				} , *fresh=malloc(sizeof*fresh);
				if(fresh==NULL)
				{
					store_attempt_ret=1;
					goto free;
				}
				memcpy(fresh,&e,sizeof*fresh);
				
				storage->next=&fresh->base;
			}
		}

		free:
		{
			list_index=&head;
			while( (list_index=list_index->next) !=NULL)
			{
				//FIXME: deep pointers not freed.
				if(list_index->e!=NULL)free(list_index->e);
			}
			return store_attempt_ret;
		}
	}
	return EXIT_SUCCESS;
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
	&cbor_store_bstr,
	&cbor_store_tstr,
	&cbor_store_arr,
	&cbor_store_map,
	&cbor_store_tag,
	NULL,//&cbor_store_flt,
};

int decode(const int stream,struct cbor_t*storage)
{
	if(storage==NULL)return 2;
	int store_status=0;
	struct cbor_t*store=storage;
	do
	{
		uint8_t item;
	
		if(read(stream,&item,sizeof item) < 1 )
		{
			break;
		}

		store_status=cbor_store[cbor_major_of(item)](store,cbor_additional_of(item),stream);
		if(store_status!=EXIT_SUCCESS)
		{
			return store_status;
		}

		if(store!=NULL)store=store->next;
	}
	while(true);
	return EXIT_SUCCESS;
}
