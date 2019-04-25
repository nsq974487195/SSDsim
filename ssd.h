/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º ssd.h
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
*****************************************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>




#ifndef INITIALIZE
#define INITIALIZE
#include "initialize.h"
#endif

#include <math.h>
#include "flash.h"
#include "pagemap.h"
#include "tools.h"


#define MAX_INT64  0x7fffffffffffffffll

struct ssd_info *simulate(struct ssd_info *);
int get_requests(struct ssd_info *);
struct ssd_info *buffer_management(struct ssd_info *);
unsigned int lpn2ppn(struct ssd_info * ,unsigned int lsn);
struct ssd_info *distribute(struct ssd_info *);
void trace_output(struct ssd_info* );
void statistic_output(struct ssd_info *);

unsigned int transfer_size(struct ssd_info *,int,unsigned int,struct request *);
int64_t find_nearest_event(struct ssd_info *);
void free_all_node(struct ssd_info *);
struct ssd_info *make_aged(struct ssd_info *);
struct ssd_info *no_buffer_distribute(struct ssd_info *);


int	create_del_request(struct ssd_info *ssd);
double U_Random();

int possion();  /* 产生一个泊松分布的随机数，Lamda为总体平均数*/

int read_data(struct ssd_info *ssd,char *buffer, int64_t *time_t, int *lsn, int *size, int *ope);