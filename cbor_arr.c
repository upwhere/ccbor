#include"cbor_arr.h"

#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<unistd.h>

#include"cbor_int.h"

static int cbor_store_definite_arr(struct cbor_t*storage, const uint8_t additional, const int stream)
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

