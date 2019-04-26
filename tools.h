#ifndef TOOLS
#define TOOLS 10000

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include<math.h>
#include "initialize.h"

//和 SSD无关的一些常用工具类函数
void alloc_assert(void *p,char *s);

void file_assert(int error,char *s);

void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope);

int possion();

double U_Random();

// size 和state之间的互相转换, state的1 bit表示 1单位的sub_page 
unsigned int size(unsigned int stored);

int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size);
#endif