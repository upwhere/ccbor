#include"ccbor.c"
#include<stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{
	struct cbor_t x={.major=cbor_major_uint,.next=NULL},*px;
	printf("decoding: %d\n",decode(open("test.cbor.dat",O_RDONLY),&x));
	px=x.next;
	while(px!=NULL)
	{
		printf("bor: %d :: %llu -> %p\n",px->major,(long long unsigned int)((struct cbor_uint_t*)px)->value,(void*)px->next);
		px=px->next;
	}
}
