#include"ccbor.h"
#include<stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include"cbor_int.h"
#include"cbor_str.h"
#include"cbor_arr.h"
#include"cbor_map.h"

static void print_cbor_t(struct cbor_t*item,const int nindent)
{
	char indent[nindent+1];
	for(int i=0;i<nindent;i++ )
		indent[i]= ' ';
	indent[nindent]= '\0';

	switch(item->major>>5)
	{
		default:
		case 0:
			printf( "\t%s%d (+int)\t%s:: %llu\n",indent,item->major>>5,indent, (long long unsigned int) ((struct cbor_uint_t*)item)->value);
			break;
		case 1:
			printf( "\t%s%d (-int)\t%s:: 1-%llu\n (= -%llu?)\n",indent,item->major>>5,indent, ((unsigned long long int)1)-(unsigned long long int)((struct cbor_nint_t*)item)->nvalue,(unsigned long long int)((struct cbor_nint_t*)item)->nvalue );
			break;
		case 2:
			printf( "\t%s%d (bstr)\t%s:: ",indent,item->major>>5,indent);
			for(size_t i=0;i<((struct cbor_bstr_t*)item)->length;i++)
			{
				printf( "%02x ", ((struct cbor_bstr_t*)item)->string[i] );
			}
			putchar( '\n' );
			break;
		case 3:
			printf( "\t%s%d (tstr)\t%s:: %s\n",indent,item->major>>5,indent, ((struct cbor_tstr_t*)item)->string);
			break;
		case 4:
		{
			struct cbor_t**x;
			printf( "\t%s%d (*arr)\t%s:: [\n",indent,item->major>>5,indent);
			x=((struct cbor_arr_t*)item)->array;
			for(size_t i=0;i<((struct cbor_arr_t*)item)->length;i++)
			{
				print_cbor_t(x[i],nindent+1);
			}
			printf( "\t%s        \t%s   ]\n",indent,indent);
			break;
		}
		case 5:
		{
			struct cbor_mapentry_t*const x=((struct cbor_map_t*)item)->map;
			printf( "\t%s%d (*map)\t%s:: [\n",indent,item->major>>5,indent);
			for(size_t i=0;i<((struct cbor_map_t*)item)->length;i++)
			{
				print_cbor_t((struct cbor_t*)x[i].key,nindent+1);
				print_cbor_t((struct cbor_t*)x[i].value,nindent+1);
				printf( "\t%s        \t%s---\n",indent,indent);
				
			}
			printf( "\t%s        \t%s   ]\n",indent,indent);
		}	
		case 6:
			printf( "\t%s%d (tag )\t:: %llu\n",indent,item->major>>5,(long long unsigned int)((struct cbor_uint_t*)item)->value);
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
		print_cbor_t(px,0);
		px=px->next;
	}
}
