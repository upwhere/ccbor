#include"ccbor.c"
#include<stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

void printf_cbor_t(struct cbor_t*item)
{
	switch(item->major>>5)
	{
		default:
		case 0:
			printf( "\t%d (+int)\t:: %llu\n",item->major>>5,(long long unsigned int)((struct cbor_uint_t*)item)->value);
			break;
		case 1:
			printf( "\t%d (-int)\t:: 1-%llu\n (= -%llu?)\n", item->major>>5, ((unsigned long long int)1)-(unsigned long long int)((struct cbor_nint_t*)item)->nvalue,(unsigned long long int)((struct cbor_nint_t*)item)->nvalue );
			break;
		case 2:
			printf( "\t%d (bstr)\t:: ", item->major>>5);
			for(size_t i=0;i<((struct cbor_bstr_t*)item)->length;i++)
			{
				printf( "%02x ", ((struct cbor_bstr_t*)item)->string[i] );
			}
			putchar( '\n' );
			break;
		case 3:
			printf( "\t%d (tstr)\t:: %s\n", item->major>>5, ((struct cbor_tstr_t*)item)->string);
			break;
		case 4:;
			struct cbor_t**x;
			printf( "\t%d (arr )\t:: [\n", item->major>>5);
			x=((struct cbor_arr_t*)item)->array;
			for(size_t i=0;i<((struct cbor_arr_t*)item)->length;i++)
			{
				printf_cbor_t(x[i]);
			}
			printf( "\t       \t   ]\n" );
			break;
		case 6:
			printf( "\t%d (tag )\t:: %llu\n",item->major>>5,(long long unsigned int)((struct cbor_uint_t*)item)->value);
			break;
	}
}

int main(void)
{
	struct cbor_t x={.major=cbor_major_uint,.next=NULL},*px;
	printf("decoding: %d\n",decode(open("test.cbor.dat",O_RDONLY),&x));
	px=x.next;
	while(px!=NULL)
	{
		printf_cbor_t(px);
		px=px->next;
	}
}
