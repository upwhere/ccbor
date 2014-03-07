#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>

#include"ccbor.h"
#include"cbor_int.h"
#include"cbor_str.h"
#include"cbor_arr.h"
#include"cbor_map.h"

uint8_t cbor_major_of(uint8_t item)
{
	return (item& (cbor_major_mask))>>5;
}

uint8_t cbor_additional_of(uint8_t item)
{
	return item& ~(cbor_major_mask);
}

void recursive_naive_cbor_free(struct cbor_t*listitem)
{
	if(listitem==NULL)return;
	recursive_naive_cbor_free(listitem->next);
	free(listitem);
	return;
}

struct cbor_tag_t {
	struct cbor_t base;
	const uint64_t value;
};

static int cbor_store_tag(struct cbor_t*storage,const uint8_t additional, const int stream)
{
	if(storage==NULL || storage->next!=NULL)return 2;
	{
		struct cbor_tag_t t= {
			.base= {
				.major=cbor_major_tag,
				.next=NULL,
			},
			.value=cbor_value_uint(additional,stream),
		},*fresh=malloc(sizeof*fresh);

		if(fresh==NULL)return 1;

		memcpy(fresh,&t,sizeof*fresh);

		/*@-immediatetrans@*/
		storage->next=&(fresh->base);
		/*@+immediatetrans@*/
		return EXIT_SUCCESS;
	}
}

/*@-nullassign@*/
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
/*@+nullassign@*/

int decode(const int stream,struct cbor_t*storage)
{
	if(storage==NULL)return 2;
	{
		int store_status=0;
		struct cbor_t*store=storage;
		do
		{
			uint8_t item=0;

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
}
