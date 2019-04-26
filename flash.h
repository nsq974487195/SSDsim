
#ifndef FLASH_H
#define FLASH_H 100000

#include <stdlib.h>
#include "pagemap.h"
#include <math.h>

#include "initialize.h"

#include "tools.h"


// the basic funciton 
struct ssd_info *process(struct ssd_info *);

struct ssd_info *insert2buffer(struct ssd_info *,unsigned int,int,struct sub_request *,struct request *);

struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die);

struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel);

struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation);

struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *onepage,unsigned int command);

struct ssd_info *delete_from_channel(struct ssd_info *ssd,unsigned int channel,struct sub_request * sub_req);

Status find_active_block(struct ssd_info *ssd,unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int page_group,struct local *location);

int write_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn);

int allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req);

int go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command);

int services_2_r_cmd_trans_and_complete(struct ssd_info * ssd);

int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag);

int services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag);

int services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag);

int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );

int static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

struct sub_request * create_del_sub_request(struct ssd_info* ssd, unsigned int lpn, unsigned int ppn, struct request * req);

int delete_del_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );

Status find_active_block_SD(struct ssd_info *ssd,struct local *location, struct sub_request *sub);
// the advanced function
#endif

