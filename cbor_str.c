#include"cbor_str.h"

#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>

#include"cbor_int.h"

static int store_definite_bstr(struct cbor_t*storage,const uint8_t additional, const int stream)
/*@globals errno@*/
{
	if(storage==NULL || storage->next!=NULL)return 2;
	{
		/* it is assumed size_t is at least as large as uint64_t */
		/** How many bytes follow */
		size_t length=cbor_value_uint(additional,stream);

		uint8_t*const string=malloc(length);

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
			} , *fresh=malloc(sizeof*fresh);
	
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

