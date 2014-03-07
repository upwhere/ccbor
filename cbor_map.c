#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdbool.h>

#include"cbor_map.h"
#include"cbor_int.h"

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
		size_t i;
		if(map==NULL)return 1;

		//Grab all map entries
		for(i=0;i<length;i++ )
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

int cbor_store_map(struct cbor_t*storage,const uint8_t additional,const int stream)
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

