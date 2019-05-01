
#ifndef FLASH_H
#define FLASH_H 100000

#include <stdlib.h>
#include "pagemap.h"
#include <math.h>

#include "initialize.h"

#include "tools.h"


// 产生读写子请求
struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation);

int allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req);

//LRU 缓存替换算法
struct ssd_info *insert2buffer(struct ssd_info *,unsigned int,int,struct sub_request *,struct request *);


// the basic core funciton 
struct ssd_info *process(struct ssd_info *);

int services_2_r_cmd_trans_and_complete(struct ssd_info * ssd);

int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag);

int services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag);

int services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag);

struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die);

struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel);

int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );


// 读写操作最后一步 计算时间
int static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

int go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state);



// 安全删除操作
struct sub_request * create_del_sub_request(struct ssd_info* ssd, unsigned int lpn, unsigned int ppn, struct request * req);

int delete_del_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );

int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );


// 删除操作的选项
Status  static_delete_select(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

Status static_delete_SD(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

Status static_delete_baseline(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

struct local *copy_location(struct local *slocation, struct local *dlocation);

//分配block的策略，可以按组分配或者多写点分配
Status find_active_block(struct ssd_info *ssd,unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int page_group,struct local *location);

Status find_active_block_SD(struct ssd_info *ssd,struct local *location, struct sub_request *sub);

Status find_active_block_baseline(struct ssd_info *ssd,struct local *location, struct sub_request *sub);

Status  find_active_block_select(struct ssd_info *ssd,struct local *location, struct sub_request *sub);
#endif

