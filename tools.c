
#include"tools.h"

void alloc_assert(void *p,char *s)//¶ÏÑÔ
{
	if(p!=NULL) return;
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}