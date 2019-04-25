
#include"tools.h"


/*****************************************************
*断言,当申请内存空间失败时，输出“malloc 变量名 error”
******************************************************/
void alloc_assert(void *p,char *s)
{
	if(p!=NULL) return;
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}




/************************************************
*断言,当打开文件失败时，输出“open 文件名 error”
*************************************************/
void file_assert(int error,char *s)
{
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}



/*********************************************************************************
*断言
*A，读到的time_t，device，lsn，size，ope都<0时，输出“trace error:.....”
*B，读到的time_t，device，lsn，size，ope都=0时，输出“probable read a blank line”
**********************************************************************************/
void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope)
{
	if(time_t <0 || device < 0 || lsn < 0 || size < 0 || ope < 0)
	{
		printf("trace error:%lld %d %d %d %d\n",time_t,device,lsn,size,ope);
		getchar();
		exit(-1);
	}
	if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
	{
		printf("probable read a blank line\n");
		getchar();
	}
}




int possion()  /* 产生一个泊松分布的随机数，Lamda为总体平均数*/
{
	int Lambda = 100, k = 0;
	long double p = 1.0;
	long double l=exp(-Lambda);  /* 为了精度，才定义为long double的，exp(-Lambda)是接近0的小数*/
	while (p>=l)
	{
		double u = U_Random();
		p *= u;
		k++;
	}
	return k-1;
}

double U_Random()  /* 产生一个0~1之间的随机数 */
{
	static int done = 0;
	int number;
	if(!done)  /*srand种子只产生一次*/
	{  
		srand((int)time(0));
		done = 1;
	}
	number=1+(int)(100.0*rand()/(RAND_MAX+1.0));
	return number/100.0;
}


/***********************************************************************************
*根据每一页的状态计算出每一需要处理的子页的数目，也就是一个子请求需要处理的子页的页数, 
×计算需要读操作的次数, NAND flash每一次读取8bit的数据
************************************************************************************/
unsigned int size(unsigned int stored)
{
	unsigned int i,total=0,mask=0x80000000;

	#ifdef DEBUG
	printf("enter size\n");
	#endif
	for(i=1;i<=32;i++)
	{
		if(stored & mask) total++;
		stored<<=1;
	}
	#ifdef DEBUG
	    printf("leave size\n");
    #endif
    return total;
}

/********************************
*函数功能是获得一个读子请求的状态
*********************************/
int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size)
{
	int temp,state,move;

	temp=~(0xffffffff<<size);
	move=lsn%ssd->parameter->subpage_page;
	state=temp<<move; // size + lsn多出来的部分

	return state;
}