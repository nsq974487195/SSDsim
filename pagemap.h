#ifndef PAGEMAP_H
#define PAGEMAP_H 10000

#include <sys/types.h>
#include "initialize.h"
#include "flash.h"
#include "ssd.h"

#define MAX_INT64  0x7fffffffffffffffll

// ppn 与location的互相转换
struct local *find_location(struct ssd_info *ssd,unsigned int ppn);

unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page);


// 预处理读请求
struct ssd_info *pre_process_page(struct ssd_info *ssd);

unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn);

// 分配物理地址
struct local *init_location(struct ssd_info *ssd,unsigned int lpn);
// 为正常的写请求提供ppn  physical page
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub);


// 垃圾回收
unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag);

int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);

int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);

int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node);

int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel);

unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);

int erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die,unsigned int plane ,unsigned int block);

int erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command);

int move_page(struct ssd_info * ssd, struct local *location,unsigned int * transfer_size);

int gc_for_channel(struct ssd_info *ssd, unsigned int channel);

int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node);


// MLC/TLC SSD读写编程时间
int page_program_time(struct ssd_info *ssd, unsigned int page_loc);

int page_read_time(struct ssd_info *ssd, unsigned int page_loc, int pv);

int page_type(struct ssd_info *ssd, unsigned int page_loc);

#endif


