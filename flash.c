#include "flash.h"

/*************************************************************************************
*删除操作  操作的服从泊松分布，平均100个请求发出1个删除请求， 删除的LPN地址服从完全随机，删除的范围也服从泊松分布，
*删除操作挂在channel上的删除操作请求队列上
*************************************************************************************/
struct sub_request * create_del_sub_request(struct ssd_info* ssd, unsigned int lpn, unsigned int ppn, struct request * req){

	struct sub_request* sub=NULL,* sub_r=NULL;
	struct channel_info * p_ch=NULL;
	struct local * loc=NULL;
	unsigned int flag=0, mask=~(0xffffffff<<(ssd->parameter->subpage_page));

	sub = (struct sub_request*)malloc(sizeof(struct sub_request));                        
	alloc_assert(sub,"sub_request");
	memset(sub,0, sizeof(struct sub_request));

	if ((sub==NULL)&&(sub->ppn!=0))
	{
		printf(" malloc failture\n");
		return NULL;
	}
	sub->location=NULL;
	sub->next_node=NULL;
	sub->next_subs=NULL;
	sub->update=NULL;
	
	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;
	}
	

	loc = find_location(ssd,ppn);
	sub->location=loc;
	sub->begin_time = ssd->current_time;
	sub->current_state = SR_WAIT;
	sub->current_time=MAX_INT64;
	sub->next_state = SR_R_C_A_TRANSFER;
	sub->next_state_predict_time=MAX_INT64;
	sub->lpn = lpn;
	sub->size=size( ~(0xffffffff<<(ssd->parameter->subpage_page)) );                                                               

	p_ch = &ssd->channel_head[loc->channel];	
	sub->ppn = ppn;
	sub->operation = DELETE;
	sub->state= mask;
	sub_r=p_ch->subs_d_head;                                                      
	flag=0;


	while (sub_r!=NULL)
	{
		if (sub_r->ppn==sub->ppn)
		{
			flag=1;
			break;
		}
		sub_r=sub_r->next_node;
	}
	if (flag==0)
	{
		if (p_ch->subs_d_tail!=NULL)
		{
			p_ch->subs_d_tail->next_node=sub;
			p_ch->subs_d_tail=sub;
		} 
		else
		{
			p_ch->subs_d_head=sub;
			p_ch->subs_d_tail=sub;
		}
	}
	else
	{
		sub->current_state = SR_D_DATA_TRANSFER;
		sub->current_time=ssd->current_time;
		sub->next_state = SR_COMPLETE;
		sub->next_state_predict_time=ssd->current_time+1000;
		sub->complete_time=ssd->current_time+1000;
	}

	return sub;
}



/**********************
*这个函数只作用于写请求
***********************/
Status allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req)
{
	struct sub_request * update=NULL;
	
	unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;
	
	struct local *location=NULL;
	/*******************************************************************
	用作计算本次更新的delta差值,  
	1. stateA&stateB 表示A和B共同的重合部分,表示更新写;
	2. stateA^(stateA&stateB) 表示A与重叠部分的不同,也就是新写
	3. stateB^(stateA&stateB) 表示B与重叠部分的不同,也就是未修改的数据,需要读出来
	*******************************************************************/
	unsigned int stateA=0,stateB=0,stateD=0;

	channel_num=ssd->parameter->channel_number;
	
	chip_num=ssd->parameter->chip_channel[0];
	
	die_num=ssd->parameter->die_chip;
	
	plane_num=ssd->parameter->plane_die;
    
	/******************************************************************
	* 更新操作 需要产生一个读请求，并且只有这个读请求完成后才能进行这个页的写操作
	*******************************************************************/	
	if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)
	{   
		/*这个写回的子请求的逻辑页不可以覆盖之前被写回的数据 需要产生读请求, 如果最新的state 是包含旧的state,则也不需要产生新的update read请求*/  
		stateA = sub_req->state;
		stateB = ssd->dram->map->map_entry[sub_req->lpn].state;
		stateD = stateA&stateB;

		if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)  
		{
			ssd->read_count++;
			ssd->update_read_count++;

			update=(struct sub_request *)malloc(sizeof(struct sub_request));
			alloc_assert(update,"update");
			memset(update,0, sizeof(struct sub_request));
			
			update->location=NULL;
			update->next_node=NULL;
			update->next_subs=NULL;
			update->update=NULL;						
			location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
			update->location=location;
			update->begin_time = ssd->current_time;
			update->current_state = SR_WAIT;
			update->current_time=MAX_INT64;
			update->next_state = SR_R_C_A_TRANSFER;
			update->next_state_predict_time=MAX_INT64;
			update->lpn = sub_req->lpn;
			update->state=((stateB^stateD)&0x7fffffff);
			update->size=size(update->state);
			update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
			update->operation = READ;
			
			if (ssd->channel_head[location->channel].subs_r_tail!=NULL)
			{
				ssd->channel_head[location->channel].subs_r_tail->next_node=update;
				ssd->channel_head[location->channel].subs_r_tail=update;
			} 
			else
			{
				ssd->channel_head[location->channel].subs_r_tail=update;
				ssd->channel_head[location->channel].subs_r_head=update;
			}
		}

		if (update!=NULL)
		{
			sub_req->update=update;
		}
	}

	location = init_location(ssd, sub_req->lpn);

	sub_req->location->channel = location->channel;
	sub_req->location->chip = location->chip;
	sub_req->location->die = location->die;
	sub_req->location->plane = location->plane;
	sub_req->location->block=-1;
	sub_req->location->page=-1;

	free(location);
	location=NULL;
	// 根据分配策略的不同,把请求挂载在SSD 或者 channel的请求队列上
	if((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))
	{
		if (ssd->subs_w_tail!=NULL){
			ssd->subs_w_tail->next_node=sub_req;
			ssd->subs_w_tail=sub_req;} 
		else{
			ssd->subs_w_tail=sub_req;
			ssd->subs_w_head=sub_req;}
	}else{

		if (ssd->channel_head[sub_req->location->channel].subs_w_tail!=NULL)
		{
			ssd->channel_head[sub_req->location->channel].subs_w_tail->next_node=sub_req;
			ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
		} 
		else
		{
			ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
			ssd->channel_head[sub_req->location->channel].subs_w_head=sub_req;
		}		
	}
	return SUCCESS;					
}



/*******************************************************************************
*insert2buffer这个函数是专门为写请求分配子请求服务的在buffer_management中被调用。
********************************************************************************/
struct ssd_info * insert2buffer(struct ssd_info *ssd,unsigned int lpn,int state,struct sub_request *sub,struct request *req)      
{
	int write_back_count,flag=0;                                                             /*flag表示为写入新数据腾空间是否完成，0表示需要进一步腾，1表示已经腾空*/
	unsigned int i,lsn,hit_flag,add_flag,sector_count,active_region_flag=0,free_sector=0;
	struct buffer_group *buffer_node=NULL,*pt,*new_node=NULL,key;
	struct sub_request *sub_req=NULL,*update=NULL;
	
	
	unsigned int sub_req_state=0, sub_req_size=0,sub_req_lpn=0;

	#ifdef DEBUG
	printf("enter insert2buffer,  current time:%lld, lpn:%d, state:%d,\n",ssd->current_time,lpn,state);
	#endif

	sector_count=size(state);                                                                 /*需要写到buffer的sector个数*/
	key.group=lpn;
	buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);   /*在平衡二叉树中寻找buffer node*/ 
    
	/************************************************************************************************
	*没有命中。
	*第一步根据这个lpn有多少子页需要写到buffer，去除已写回的lsn，为该lpn腾出位置，
	*首先即要计算出free sector（表示还有多少可以直接写的buffer节点）。
	*如果free_sector>=sector_count，即有多余的空间够lpn子请求写，不需要产生写回请求
	*否则，没有多余的空间供lpn子请求写，这时需要释放一部分空间，产生写回请求。就要creat_sub_request()
	*************************************************************************************************/
	if(buffer_node==NULL)
	{
		free_sector=ssd->dram->buffer->max_buffer_sector-ssd->dram->buffer->buffer_sector_count;   
		if(free_sector>=sector_count)
		{
			flag=1;    
		}
		if(flag==0)     
		{
			write_back_count=sector_count-free_sector;
			ssd->dram->buffer->write_miss_hit=ssd->dram->buffer->write_miss_hit+write_back_count;
			while(write_back_count>0)
			{
				sub_req=NULL;
				sub_req_state=ssd->dram->buffer->buffer_tail->stored; 
				sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
				sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
				sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);
				
				/**********************************************************************************
				*req不为空，表示这个insert2buffer函数是在buffer_management中调用，传递了request进来
				*req为空，表示这个函数是在process函数中处理一对多映射关系的读的时候，需要将这个读出
				*的数据加到buffer中，这可能产生实时的写回操作，需要将这个实时的写回操作的子请求挂在
				*这个读请求的总请求上
				***********************************************************************************/
				if(req!=NULL)                                             
				{
				}
				else    
				{
					sub_req->next_subs=sub->next_subs;
					sub->next_subs=sub_req;
				}
                
				/*********************************************************************
				*写请求插入到了平衡二叉树，这时就要修改dram的buffer_sector_count；
				*维持平衡二叉树调用avlTreeDel()和AVL_TREENODE_FREE()函数；维持LRU算法；
				**********************************************************************/
				ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req->size;
				pt = ssd->dram->buffer->buffer_tail;
				avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
				if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL){
					ssd->dram->buffer->buffer_head = NULL;
					ssd->dram->buffer->buffer_tail = NULL;
				}else{
					ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
					ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
				}
				pt->LRU_link_next=NULL;
				pt->LRU_link_pre=NULL;
				AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
				pt = NULL;
				
				write_back_count=write_back_count-sub_req->size;                            /*因为产生了实时写回操作，需要将主动写回操作区域增加*/
			}
		}
		
		/******************************************************************************
		*生成一个buffer node，根据这个页的情况分别赋值个各个成员，添加到队首和二叉树中
		*******************************************************************************/
		new_node=NULL;
		new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));
		alloc_assert(new_node,"buffer_group_node");
		memset(new_node,0, sizeof(struct buffer_group));
		
		new_node->group=lpn;
		new_node->stored=state;
		new_node->dirty_clean=state;
		new_node->LRU_link_pre = NULL;
		new_node->LRU_link_next=ssd->dram->buffer->buffer_head;
		if(ssd->dram->buffer->buffer_head != NULL){
			ssd->dram->buffer->buffer_head->LRU_link_pre=new_node;
		}else{
			ssd->dram->buffer->buffer_tail = new_node;
		}
		ssd->dram->buffer->buffer_head=new_node;
		new_node->LRU_link_pre=NULL;
		avlTreeAdd(ssd->dram->buffer, (TREE_NODE *) new_node);
		ssd->dram->buffer->buffer_sector_count += sector_count;
	}
	/****************************************************************************************
	*在buffer中命中的情况
	*算然命中了，但是命中的只是lpn，有可能新来的写请求，只是需要写lpn这一page的某几个sub_page
	*这时有需要进一步的判断
	*****************************************************************************************/
	else
	{
		for(i=0;i<ssd->parameter->subpage_page;i++)
		{
			/*************************************************************
			*判断state第i位是不是1
			*并且判断第i个sector是否存在buffer中，1表示存在，0表示不存在。
			**************************************************************/
			if((state>>i)%2!=0)                                                         
			{
				lsn=lpn*ssd->parameter->subpage_page+i;
				hit_flag=0;
				hit_flag=(buffer_node->stored)&(0x00000001<<i);
				
				if(hit_flag!=0)				                                           /*命中了，需要将该节点移到buffer的队首，并且将命中的lsn进行标记*/
				{	
					active_region_flag=1;                                             /*用来记录在这个buffer node中的lsn是否被命中，用于后面对阈值的判定*/

					if(req!=NULL)
					{
						if(ssd->dram->buffer->buffer_head!=buffer_node)     
						{				
							if(ssd->dram->buffer->buffer_tail==buffer_node)
							{				
								ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;
								buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							}				
							else if(buffer_node != ssd->dram->buffer->buffer_head)
							{					
								buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
								buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;
							}				
							buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;	
							ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
							buffer_node->LRU_link_pre=NULL;				
							ssd->dram->buffer->buffer_head=buffer_node;					
						}					
						ssd->dram->buffer->write_hit++;
						req->complete_lsn_count++;                                        /*关键 当在buffer中命中时 就用req->complete_lsn_count++表示往buffer中写了数据。*/						
					}
					else
					{
					}				
				}			
				else                 			
				{
					/************************************************************************************************************
					*该lsn没有命中，但是节点在buffer中，需要将这个lsn加到buffer的对应节点中
					*从buffer的末端找一个节点，将一个已经写回的lsn从节点中删除(如果找到的话)，更改这个节点的状态，同时将这个新的
					*lsn加到相应的buffer节点中，该节点可能在buffer头，不在的话，将其移到头部。如果没有找到已经写回的lsn，在buffer
					*节点找一个group整体写回，将这个子请求挂在这个请求上。可以提前挂在一个channel上。
					*第一步:将buffer队尾的已经写回的节点删除一个，为新的lsn腾出空间，这里需要修改队尾某节点的stored状态这里还需要
					*       增加，当没有可以之间删除的lsn时，需要产生新的写子请求，写回LRU最后的节点。
					*第二步:将新的lsn加到所述的buffer节点中。
					*************************************************************************************************************/	
					ssd->dram->buffer->write_miss_hit++;
					
					if(ssd->dram->buffer->buffer_sector_count>=ssd->dram->buffer->max_buffer_sector)
					{
						if (buffer_node==ssd->dram->buffer->buffer_tail)                   /*如果命中的节点是buffer中最后一个节点，交换最后两个节点*/
						{
							pt = ssd->dram->buffer->buffer_tail->LRU_link_pre;
							ssd->dram->buffer->buffer_tail->LRU_link_pre=pt->LRU_link_pre;
							ssd->dram->buffer->buffer_tail->LRU_link_pre->LRU_link_next=ssd->dram->buffer->buffer_tail;
							ssd->dram->buffer->buffer_tail->LRU_link_next=pt;
							pt->LRU_link_next=NULL;
							pt->LRU_link_pre=ssd->dram->buffer->buffer_tail;
							ssd->dram->buffer->buffer_tail=pt;
							
						}
						sub_req=NULL;
						sub_req_state=ssd->dram->buffer->buffer_tail->stored; 
						sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
						sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
						sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);

						if(req!=NULL)           
						{
							
						}
						else if(req==NULL)   
						{
							sub_req->next_subs=sub->next_subs;
							sub->next_subs=sub_req;
						}

						ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req->size;
						pt = ssd->dram->buffer->buffer_tail;	
						avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
							
						/************************************************************************/
						/* 改:  挂在了子请求，buffer的节点不应立即删除，						*/
						/*			需等到写回了之后才能删除									*/							
						/************************************************************************/
						if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL)
						{
							ssd->dram->buffer->buffer_head = NULL;
							ssd->dram->buffer->buffer_tail = NULL;
						}else{
							ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
							ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
						}
						pt->LRU_link_next=NULL;
						pt->LRU_link_pre=NULL;
						AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
						pt = NULL;	
					}

					                                                                     /*第二步:将新的lsn加到所述的buffer节点中*/	
					add_flag=0x00000001<<(lsn%ssd->parameter->subpage_page);
					
					if(ssd->dram->buffer->buffer_head!=buffer_node)                       /*如果该buffer节点不在buffer的队首，需要将这个节点提到队首*/
					{				
						if(ssd->dram->buffer->buffer_tail==buffer_node)
						{					
							buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;
						}			
						else						
						{			
							buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;						
							buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
						}								
						buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;			
						ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
						buffer_node->LRU_link_pre=NULL;	
						ssd->dram->buffer->buffer_head=buffer_node;							
					}					
					buffer_node->stored=buffer_node->stored|add_flag;		
					buffer_node->dirty_clean=buffer_node->dirty_clean|add_flag;	
					ssd->dram->buffer->buffer_sector_count++;
				}			

			}
		}
	}

	return ssd;
}


Status find_active_block(struct ssd_info *ssd,unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int page_group,struct local *location){

	unsigned int count =0;

	unsigned int active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	
	int last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;

	/*********************************************************
	 last_write_page记录着上次写入的位置, 可以依次增加+1.或者依次+4,
	 主要是根据last_write_page来判断
	**********************************************************/
	while((last_write_page== (ssd->parameter-> page_block / page_group-1) )&&(count<ssd->parameter->block_plane)) 
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;

		last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;
		
		count++;
	}

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
	//page_group; last_write_page表示组号，并不是page号
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page += 1;
	
	if(last_write_page > (int)(ssd->parameter-> page_block / page_group -1) )
	{
		printf("error! the last write page larger than the capacity of block!!\n");
	
		getchar();

		return ERROR;
	}

	if(count < ssd->parameter->block_plane)
	{	

		location->block = active_block;

		location->page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page *page_group ;

		return SUCCESS;
	}
	else
	{
		printf("%s\n","未找到符合要求的block");
		return FAILURE;
	}

}



Status find_active_block_baseline(struct ssd_info *ssd,struct local *location, struct sub_request *sub){

	unsigned int active_block,ppn=0;
	unsigned int free_page_num=0,last_write_page=0;
	unsigned int count=0,flag=1;
	unsigned int channel=location->channel, chip=location->chip, die=location->die, plane=location->plane;
	unsigned int page_group = 1;


	if(find_active_block(ssd,channel,chip,die,plane,page_group,location) != SUCCESS)  { printf("find_active_block error!\n"); flag =0;}



	//更新plane内剩余的free page,用作GC
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--; 
	// 修改plane block和page的状态
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[location->block].free_page_num--;
	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[location->block].page_head[location->page].written_count++;
	
	if(flag == 1) return SUCCESS;

	else return FAILURE;
}


Status  find_active_block_select(struct ssd_info *ssd,struct local *location, struct sub_request *sub){

	if(ssd->parameter->base_or_pro == 0)

		return find_active_block_baseline(ssd,location,sub);

	else if(ssd->parameter->base_or_pro == 1)

		return find_active_block_SD(ssd, location, sub);

	else { printf("%s\n"," no used scheme!"); return FAILURE;}
}


/**************************************************************************************
函数的功能是寻找活跃块，应为每个plane中都只有一个活跃块，只有这个活跃块中才能进行操作
传入变量location, location的来源有预处理和write sub,location里面的channel chip die plane 有效， block和page是无效的
同时负责管理每个块block内的写点位置last_write_page，free page， invalid_page_num是按照 +1 往上增加
***************************************************************************************/
Status find_active_block_SD(struct ssd_info *ssd,struct local *location, struct sub_request *sub){

	unsigned int active_block,ppn=0;
	unsigned int free_page_num=0,last_write_page=0;
	unsigned int count=0,flag=1;
	unsigned int channel=location->channel, chip=location->chip, die=location->die, plane=location->plane;
	unsigned int page_group = 4;

	if( sub == NULL){

		if(find_active_block(ssd,channel,chip,die,plane,page_group,location) != SUCCESS)  { printf("find_active_block error!\n"); flag =0;}
	
	}else if(ssd->dram->map->map_entry[sub->lpn].state!=0){

		ppn=ssd->dram->map->map_entry[sub->lpn].pn;
			
		struct local *tmp_local=find_location(ssd,ppn);

		location->block = tmp_local->block;

		location->page = tmp_local->page; //分配下一个page地址

		if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!= sub->lpn)
		{
			printf("\nError in find_active_block_SD(), the lpn isn't the same as the lpn stored in page \n");
			getchar();
		}else{
			//	printf("%s\n","it works normally" );
		}

		if( tmp_local->page%4 <0){

			printf(" tmp_local->page%4 :%d %s\n",tmp_local->page%4 ,"find_active_block_SD allocation error!");

			flag =0;

		}else if( tmp_local->page%4 <2){

			if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+1].free_state!= PG_SUB)
			{
				printf("\nError in find_active_block_SD(), the lpn isn't the same as the lpn stored in page \n");
				getchar();

			}else  location->page = tmp_local->page+1; //分配下一个page地址

		}else if( tmp_local->page%4 ==2){  
			// 如果当前group是在block的顶端 或者 它的上一级 group 位置已经无效, 则可写3号位置
			if ((location->page == (ssd->parameter->page_block-2))||(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+3].valid_state==0 )){

				if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+1].free_state!= PG_SUB)
				{
					printf("\nError in find_active_block_SD(), the lpn isn't the same as the lpn stored in page \n");
					getchar();

				}else  location->page = tmp_local->page+1; //分配下一个page地址

			} // end if
			else{

				if(find_active_block(ssd,channel,chip,die,plane,page_group,location) != SUCCESS) { printf("find_active_block error!\n"); flag =0;}		
				
				}
		}else if( tmp_local->page%4 ==3){

			if(find_active_block(ssd,channel,chip,die,plane,page_group,location) != SUCCESS)  { printf("find_active_block error!\n"); flag =0;}
		
		}else{

			printf("%s\n","find_active_block_SD allocation error!");
		}

		free(tmp_local);

		tmp_local = NULL;
	//初次写, 在plane当中找到符合条件的block,分配相应的page，并且更新block的信息,mapping table 记下 lpn对应的group内的位置信息
	}else{

		if(find_active_block(ssd,channel,chip,die,plane,page_group,location) != SUCCESS)  { printf("find_active_block error!\n"); flag =0;}
	}


	//更新plane内剩余的free page,用作GC
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--; 
	// 修改plane block和page的状态
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[location->block].free_page_num--;

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[location->block].page_head[location->page].written_count++;
	
	if(flag == 1) return SUCCESS;

	else return FAILURE;
}


/**********************************************
*这个函数的功能是根据lpn，size，state创建子请求
**********************************************/
struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation)
{
	struct sub_request* sub=NULL,* sub_r=NULL;

	struct channel_info * p_ch=NULL;

	struct local * loc=NULL;

	unsigned int flag=0;

	sub = (struct sub_request*)malloc(sizeof(struct sub_request));

	alloc_assert(sub,"sub_request");

	memset(sub,0, sizeof(struct sub_request));

	if ((sub==NULL)&&(sub->ppn!=0))
	{
		return NULL;
	}

	sub->location=NULL;

	sub->next_node=NULL;

	sub->next_subs=NULL;

	sub->update=NULL;
	
	if(req!=NULL)
	{
		sub->next_subs = req->subs;

		req->subs = sub;
	}
	
	/*************************************************************************************
	*在读操作的情况下，有一点非常重要就是要预先判断读子请求队列中是否有与这个子请求相同的，
	*有的话，新子请求就不必再执行了，将新的子请求直接赋为完成
	**************************************************************************************/
	if (operation == READ)
	{	
		loc = find_location(ssd,ssd->dram->map->map_entry[lpn].pn);
		sub->location=loc;
		sub->begin_time = ssd->current_time;
		sub->current_state = SR_WAIT;
		sub->current_time=MAX_INT64;
		sub->next_state = SR_R_C_A_TRANSFER;
		sub->next_state_predict_time=MAX_INT64;
		sub->lpn = lpn;
		sub->size=size;                                                              
		/*需要计算出该子请求的请求大小*/
		p_ch = &ssd->channel_head[loc->channel];	
		sub->ppn = ssd->dram->map->map_entry[lpn].pn;
		sub->operation = READ;
		sub->state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
		/*一下几行包括flag用于判断该读子请求队列中是否有与这个子请求相同的，有的话，将新的子请求直接赋为完成*/
		sub_r=p_ch->subs_r_head;                                                     
		flag=0;

		//读请求是在创造请求的请求的时候就 计算请求数，写请求在分配ppn的时候计算,累加
		ssd->read_count++;
		ssd->channel_head[loc->channel].read_count++;
		ssd->channel_head[loc->channel].chip_head[loc->chip].read_count++;
		ssd->channel_head[loc->channel].chip_head[loc->chip].die_head[loc->die].plane_head[loc->plane].read_count++;

		while (sub_r!=NULL)
		{
			if (sub_r->ppn==sub->ppn)
			{
				flag=1;
				break;
			}
			sub_r=sub_r->next_node;
		}
		if (flag==0)
		{
			if (p_ch->subs_r_tail!=NULL)
			{
				p_ch->subs_r_tail->next_node=sub;
				p_ch->subs_r_tail=sub;
			} 
			else
			{
				p_ch->subs_r_head=sub;
				p_ch->subs_r_tail=sub;
			}
		}
		else
		{
			sub->current_state = SR_R_DATA_TRANSFER;
			sub->current_time=ssd->current_time;
			sub->next_state = SR_COMPLETE;
			sub->next_state_predict_time=ssd->current_time+1000;
			sub->complete_time=ssd->current_time+1000;
		}
	}
	/*************************************************************************************
	*写请求的情况下，就需要利用到函数allocate_location(ssd ,sub)来处理静态分配和动态分配
	**************************************************************************************/
	else if(operation == WRITE)
	{                                
		sub->ppn=0;
		sub->operation = WRITE;
		sub->location=(struct local *)malloc(sizeof(struct local));
		alloc_assert(sub->location,"sub->location");
		memset(sub->location,0, sizeof(struct local));

		sub->current_state=SR_WAIT;
		sub->current_time=ssd->current_time;
		sub->lpn=lpn;
		sub->size=size;
		sub->state=state;
		sub->begin_time=ssd->current_time;
      
		if (allocate_location(ssd ,sub)==ERROR)
		{
			free(sub->location);
			sub->location=NULL;
			free(sub);
			sub=NULL;
			return NULL;
		}
			
	}
	else
	{
		free(sub->location);
		sub->location=NULL;
		free(sub);
		sub=NULL;
		printf("\nERROR ! Unexpected command.\n");
		return NULL;
	}
	
	return sub;
}


/******************************************************
*函数的功能是在给出的channel，chip，die上面寻找读子请求
*这个子请求的ppn要与相应的plane的寄存器里面的ppn相符
*******************************************************/
struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die)
{
	unsigned int plane=0;
	unsigned int address_ppn=0;
	struct sub_request *sub=NULL,* p=NULL;

	for(plane=0;plane<ssd->parameter->plane_die;plane++)
	{
		address_ppn=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].add_reg_ppn;
		if(address_ppn!=-1)
		{
			sub=ssd->channel_head[channel].subs_r_head;
			if(sub->ppn==address_ppn)
			{
				if(sub->next_node==NULL)
				{
					ssd->channel_head[channel].subs_r_head=NULL;
					ssd->channel_head[channel].subs_r_tail=NULL;
				}
				// read subrequest从channel 队列上删除
				ssd->channel_head[channel].subs_r_head=sub->next_node;  
			}
			while((sub->ppn!=address_ppn)&&(sub->next_node!=NULL))
			{
				if(sub->next_node->ppn==address_ppn)
				{
					p=sub->next_node;
					if(p->next_node==NULL)
					{
						sub->next_node=NULL;
						ssd->channel_head[channel].subs_r_tail=sub; 
					}
					else
					{
						sub->next_node=p->next_node;
					}
					sub=p;
					break;
				}
				sub=sub->next_node;
			}
			if(sub->ppn==address_ppn)
			{
				sub->next_node=NULL;
				return sub;
			}
			else 
			{
				printf("Error! Can't find the sub request.");
			}
		}
	}
	return NULL;
}

/*******************************************************************************
*函数的功能是寻找写子请求。
*分两种情况1，要是是完全动态分配就在ssd->subs_w_head队列上找
*2，要是不是完全动态分配那么就在ssd->channel_head[channel].subs_w_head队列上查找
********************************************************************************/
struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel)
{
	struct sub_request * sub=NULL,* p=NULL;
	if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))    
	{
		sub=ssd->subs_w_head;
		while(sub!=NULL)        							
		{
			if(sub->current_state==SR_WAIT)								
			{
				if (sub->update!=NULL)                                                     
				{
					if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))   
					{
						break;
					}
				} 
				else
				{
					break;
				}						
			}
			p=sub;
			sub=sub->next_node;							
		}

		if (sub==NULL)                                                                     
		{
			return NULL;
		}

		if (sub!=ssd->subs_w_head)
		{
			if (sub!=ssd->subs_w_tail)
			{
				p->next_node=sub->next_node;
			}
			else
			{
				ssd->subs_w_tail=p;
				ssd->subs_w_tail->next_node=NULL;
			}
		} 
		else
		{
			if (sub->next_node!=NULL)
			{
				ssd->subs_w_head=sub->next_node;
			} 
			else
			{
				ssd->subs_w_head=NULL;
				ssd->subs_w_tail=NULL;
			}
		}
		sub->next_node=NULL;
		if (ssd->channel_head[channel].subs_w_tail!=NULL)
		{
			ssd->channel_head[channel].subs_w_tail->next_node=sub;
			ssd->channel_head[channel].subs_w_tail=sub;
		} 
		else
		{
			ssd->channel_head[channel].subs_w_tail=sub;
			ssd->channel_head[channel].subs_w_head=sub;
		}
	}
	/**********************************************************
	*除了全动态分配方式，其他方式的请求已经分配到特定的channel，
	*就只需要在channel上找出准备服务的子请求
	***********************************************************/
	else            
	{
		sub=ssd->channel_head[channel].subs_w_head;
		while(sub!=NULL)        						
		{
			if(sub->current_state==SR_WAIT)								
			{
				if (sub->update!=NULL)    
				{
					if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))   //±»¸üÐÂµÄÒ³ÒÑ¾­±»¶Á³ö
					{
						break;
					}
				} 
				else
				{
					break;
				}						
			}
			p=sub;
			sub=sub->next_node;							
		}

		if (sub==NULL)
		{
			return NULL;
		}
	}
	
	return sub;
}



/*********************************************************************************************
*专门为读子请求服务的函数
*1，只有当读子请求的当前状态是SR_R_C_A_TRANSFER, chip内部数据传输,channel不影响
*2，读子请求的当前状态是SR_COMPLETE或者下一状态是SR_COMPLETE并且下一状态到达的时间比当前时间小
**********************************************************************************************/
Status services_2_r_cmd_trans_and_complete(struct ssd_info * ssd)
{
	unsigned int i=0;
	struct sub_request * sub=NULL, * p=NULL;
	 /*这个循环处理不需要channel的时间(读命令已经到达chip，chip由ready变为busy)，当读请求完成时，将其从channel的队列中取出*/
	for(i=0;i<ssd->parameter->channel_number;i++)                                      
	{
		sub=ssd->channel_head[i].subs_r_head;
		p=sub; //防止第一次有读请求完成, p为空的现象出现

		while(sub!=NULL)
		{/*读命令发送完毕，将对应的die置为busy，同时修改sub的状态; 这个部分专门处理读请求由当前状态为传命令变为die开始busy，die开始busy不需要channel为空，所以单独列出*/
			if(sub->current_state==SR_R_C_A_TRANSFER)                                  
			{
				if(sub->next_state_predict_time<=ssd->current_time)
				{
					/*状态跳变处理函数*/
					go_one_step(ssd, sub,NULL, SR_R_READ);                      
				}
			}
			else if((sub->current_state==SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))					
			{			
				if(sub!=ssd->channel_head[i].subs_r_head) /*if the request is completed, we delete it from read queue */							
				{		
					p->next_node=sub->next_node;						
				}			
				else					
				{	
					if (ssd->channel_head[i].subs_r_head!=ssd->channel_head[i].subs_r_tail){
						
						ssd->channel_head[i].subs_r_head=sub->next_node;
					} 
					else
					{
						ssd->channel_head[i].subs_r_head=NULL;
						ssd->channel_head[i].subs_r_tail=NULL;
					}							
				}		
			}
			p=sub;
			sub=sub->next_node;
		}
	}
	
	return SUCCESS;
}

/**************************************************************************
*这个函数也是只处理读子请求，处理chip当前状态是CHIP_WAIT，
*或者下一个状态是CHIP_DATA_TRANSFER并且下一状态的预计时间小于当前时间的chip
***************************************************************************/
Status services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag)
{
	int chip=0;
	unsigned int die=0,plane=0,address_ppn=0,die1=0;
	struct sub_request * sub=NULL, * p=NULL,*sub1=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	for(chip=0;chip<ssd->channel_head[channel].chip;chip++)           			    
	{				       		      
		if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_WAIT)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_DATA_TRANSFER)&&
			(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))					       					
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{
				/*在channel,chip,die中找到读子请求*/
				sub=find_read_sub_request(ssd,channel,chip,die);                   
				if(sub!=NULL)
				{
					break;
				}
			}

			if(sub==NULL)
			{
				continue;
			}
			
			/*如果ssd不支持高级命令那么就执行一个一个的执行读子请求*/					
			go_one_step(ssd, sub,NULL, SR_R_DATA_TRANSFER);

			*channel_busy_flag=1;	
			break;
		}		
	}
	return SUCCESS;
}


/******************************************************
*这个函数也是只服务读子请求，并且处于等待状态的读子请求
*******************************************************/
int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag)
{
	unsigned int plane=0,address_ppn=0;
	struct sub_request * sub=NULL, * p=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	
	sub=ssd->channel_head[channel].subs_r_head;

	/*******************************
	*ssd不能执行执行高级命令的情况下
	*******************************/

	while(sub!=NULL)  /*if there are read requests in queue, send one of them to target chip*/			
	{		
		if(sub->current_state==SR_WAIT)									
		{	                                                                       
			if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
				(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
			{	
				go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER);

				*channel_busy_flag=1;        /*已经占用了这个周期的总线，不用执行die中数据的回传*/
				break;										
			}	
		}						
		sub=sub->next_node;								
	}

	return SUCCESS;
}

/*********************************************************************
*当一个写子请求处理完后，要从请求队列上删除，这个函数就是执行这个功能
**********************************************************************/
int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub )
{
	struct sub_request * p=NULL;
	if (sub==ssd->channel_head[channel].subs_w_head)    /*将这个子请求从channel队列中删除*/
	{
		if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
		{
			ssd->channel_head[channel].subs_w_head=sub->next_node;
		} 
		else
		{
			ssd->channel_head[channel].subs_w_head=NULL;
			ssd->channel_head[channel].subs_w_tail=NULL;
		}
	}
	else
	{
		p=ssd->channel_head[channel].subs_w_head;
		while(p->next_node !=sub)
		{
			p=p->next_node;
		}

		if (sub->next_node!=NULL)
		{
			p->next_node=sub->next_node;
		} 
		else
		{
			p->next_node=NULL;
			ssd->channel_head[channel].subs_w_tail=p;
		}
	}
	
	return SUCCESS;	
}


/*****************
*静态写操作的实现
******************/
Status static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub)
{
	long long time=0;
	get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub); 

	ssd->parameter->time_characteristics.tPROG = page_program_time(ssd, sub->location->page);

	sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tPROG;

	sub->complete_time=sub->next_state_predict_time;		

    /****************************************************************
	*执行copyback高级命令时，需要修改channel，chip的状态，以及时间等
	*****************************************************************/
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time=sub->next_state_predict_time-ssd->parameter->time_characteristics.tPROG;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time= sub->complete_time;
	
	return SUCCESS;
}


Status  static_delete_select(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub){

	if(ssd->parameter->base_or_pro == 0)

		return static_delete_baseline(ssd,channel,chip,die,sub);

	else if(ssd->parameter->base_or_pro == 1)

		return static_delete_SD(ssd,channel,chip,die,sub);

	else { printf("%s\n"," no used scheme!"); return FAILURE;}
}


// proposed scheme 
Status static_delete_SD(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub)
{
	// 分别读取lpn所对应的ppn的地址,计算读取和迁移的延迟时间
	unsigned int read_count=0, program_count=0; //SUB REQUEST的目标page默认需要reprogram;

	struct local *location= sub->location, *upper_right_location=NULL;

	unsigned int ppn= find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);

	int transfer_size=0;

	//判断上下page是否有效
	// 因为四个page在两个cell中，最多reprogram两次即可
	// free_state !=PG_SUB 则表示不是未写过的page
	if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+2].free_state != PG_SUB)
			
		program_count = program_count+2; 
	
	else
	
		program_count = program_count+1;	
	

	// 判断受program disturb影响的page是否包含数据,包含数据则需要迁移
	if ((location->page+5 < ssd->parameter->page_block )&&(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+5].lpn!= 0 )){

		upper_right_location = copy_location(location,upper_right_location);

		upper_right_location->page = upper_right_location->page + 5;

		move_page(ssd, upper_right_location, &transfer_size);

		read_count++;

		program_count++;

		free(upper_right_location);

		upper_right_location=NULL;
	}

	// 记录所有的迁移page数
	ssd->live_copy_program = ssd->live_copy_program + program_count;

	ssd->live_copy_read = ssd->live_copy_read + read_count;

	// 记下读操作和写操作的时间
	int read_time = 7*ssd->parameter->time_characteristics.tWC+(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC +page_read_time(ssd, sub->location->page,0);
	
	int program_time = 7*ssd->parameter->time_characteristics.tWC+(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC+page_program_time(ssd, sub->location->page);

	// sub request的下一步时间
	sub->next_state_predict_time = ssd->current_time +  read_count*read_time + program_count*program_time;	
	
	sub->complete_time = sub->next_state_predict_time;	

    /****************************************************************
	 channel chip 的下一步的时间计算
	*****************************************************************/
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time= sub->complete_time - read_count*page_read_time(ssd, sub->location->page,0)  - program_count*page_program_time(ssd, sub->location->page) ;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time= sub->complete_time;

	return SUCCESS;
}


struct local *copy_location(struct local *slocation, struct local *dlocation){

	dlocation=(struct local *)malloc(sizeof(struct local));
    alloc_assert(dlocation,"dlocation");
	memset(dlocation,0, sizeof(struct local));

	dlocation->channel = slocation->channel;
	dlocation->chip = slocation->chip;
	dlocation->die = slocation->die;
	dlocation->plane=slocation->plane;
	dlocation->block = slocation->block;
	dlocation->page = slocation->page; 

	return dlocation;
}

// baseline的时间计算
Status static_delete_baseline(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub)
{
	
	// 分别读取lpn所对应的ppn的地址,计算读取和迁移的延迟时间

	long long time=0;

	int transfer_size=0;

	unsigned int ppn=1, pair_ppn=1, upper_left_ppn=1, upper_right_ppn=1, below_left_ppn=1, below_right_ppn=1,flag=0;
	
	unsigned int read_count=0,program_count=0; 

	struct local *location = sub->location,  *pair_location=NULL, *upper_left_location=NULL, *upper_right_location=NULL, *below_left_location=NULL, *below_right_location=NULL;


	// 若是第一个或者最后一个page，则只有一半的相邻page
	if(sub->location->page ==0 || sub->location->page ==1)
	{
		below_left_ppn=0;

		below_right_ppn=0;
	}

	if(sub->location->page ==ssd->parameter->page_block-1 || sub->location->page ==ssd->parameter->page_block-2)
	{
		upper_left_ppn = 0;

		upper_right_ppn= 0;	
	}

	//判断reprogram的page是LSB 还是MSB   0 = LSB 1=MSB
	if( sub->location->page%2 ==1 )   flag ==1; //MSB

	//判断上下page是否有效
	if(flag==0)
	{	// 根据page存储的lpn地址是否 =-1 判断是否需要迁移操作
		if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+1].lpn!=0){
			
			pair_location = copy_location(location,pair_location);

			pair_location->page = pair_location->page +1;
			
			move_page(ssd, pair_location, &transfer_size);

			read_count++;  

			program_count++;

			free(pair_location);

			pair_location=NULL;
		}


		if((below_left_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page-2].lpn  !=0)){
			
			below_left_location = copy_location(location,below_left_location);

			below_left_location->page = below_left_location->page -2;
			
			move_page(ssd, below_left_location, &transfer_size);

			read_count++; 
			
			program_count++;

			free(below_left_location);

			below_left_location=NULL;
		}
		

		if((below_right_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page-1].lpn  !=0)){

			below_right_location = copy_location(location,below_right_location);

			below_right_location->page = below_right_location->page -1;
			
			move_page(ssd, below_right_location, &transfer_size);

			read_count++; 
			
			program_count++;

			free(below_right_location);

			below_right_location=NULL;
		}
		
		
		if((upper_left_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+2].lpn  !=0 )){
			
			upper_left_location = copy_location(location,upper_left_location);
			
			upper_left_location->page = upper_left_location->page +2;

			move_page(ssd, upper_left_location, &transfer_size);

			read_count++; 
			
			program_count++;

			free(upper_left_location);

			upper_left_location=NULL;
		}

	
		if((upper_right_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+3].lpn !=0 )){	

			upper_right_location = copy_location(location,upper_right_location);

			upper_right_location->page = upper_right_location->page +3;
			
			move_page(ssd, upper_right_location, &transfer_size);		

			read_count++; 
			
			program_count++;

			free(upper_right_location);

			upper_right_location=NULL;
		}
		

	}else{

		if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page-1].lpn !=0)
		{
			pair_location = copy_location(location,pair_location);

			pair_location->page = pair_location->page -1;
			
			move_page(ssd, pair_location, &transfer_size);

			read_count++;  

			program_count++;

			free(pair_location);

			pair_location=NULL;
		}

		if((below_left_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page-3].lpn !=0)){
			
			below_left_location = copy_location(location,below_left_location);

			below_left_location->page = below_left_location->page -3;
			
			move_page(ssd, below_left_location, &transfer_size);

			read_count++; 
			
			program_count++;

			free(below_left_location);

			below_left_location=NULL;

		}

		if((below_right_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page-2].lpn !=0 )){
			
			below_right_location = copy_location(location,below_right_location);

			below_right_location->page = below_right_location->page -2;
			
			move_page(ssd, below_right_location, &transfer_size);

			read_count++; 
			
			program_count++;

			free(below_right_location);

			below_right_location=NULL;
		
		}

		if((upper_left_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+1].lpn !=0 )){

			upper_left_location = copy_location(location,upper_left_location);

			upper_left_location->page = upper_left_location->page +1;
			
			move_page(ssd, upper_left_location, &transfer_size);

			read_count++; 
			
			program_count++;

			free(upper_left_location);

			upper_left_location=NULL;
		}

		if((upper_right_ppn !=0)&&( ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page+2].lpn !=0 )){	

			upper_right_location = copy_location(location,upper_right_location);

			upper_right_location->page = upper_right_location->page +2;
			
			move_page(ssd, upper_right_location, &transfer_size);		

			read_count++; 
			
			program_count++;

			free(upper_right_location);

			upper_right_location=NULL;
		
		}

	}
	// 记录所有的迁移page数
	ssd->live_copy_program = ssd->live_copy_program + program_count +1;

	ssd->live_copy_read = ssd->live_copy_read + read_count;

	// 记下读操作和写操作的时间
	int read_time = 7*ssd->parameter->time_characteristics.tWC+(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC +page_read_time(ssd, sub->location->page,0);

	
	int program_time = 7*ssd->parameter->time_characteristics.tWC+(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC+page_program_time(ssd, sub->location->page);



	sub->next_state_predict_time = ssd->current_time+ program_time + read_count*read_time+ program_count *program_time;
	
	sub->complete_time = sub->next_state_predict_time;	



    /****************************************************************
	 channel chip 的下一步的时间计算
	*****************************************************************/
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time= sub->complete_time - (program_count+1)*page_program_time(ssd, sub->location->page) - read_count*page_read_time(ssd, sub->location->page,0);

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time= sub->complete_time;

	return SUCCESS;
}


int delete_del_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub )
{
	struct sub_request * p=NULL;
	if (sub==ssd->channel_head[channel].subs_d_head)                                   /*½«Õâ¸ö×ÓÇëÇó´Óchannel¶ÓÁÐÖÐÉ¾³ý*/
	{
		if (ssd->channel_head[channel].subs_d_head!=ssd->channel_head[channel].subs_d_tail)
		{
			ssd->channel_head[channel].subs_d_head=sub->next_node;
		} 
		else
		{
			ssd->channel_head[channel].subs_d_head=NULL;
			ssd->channel_head[channel].subs_d_tail=NULL;
		}
	}
	else
	{
		p=ssd->channel_head[channel].subs_d_head;
		while(p->next_node !=sub)
		{
			p=p->next_node;
		}

		if (sub->next_node!=NULL)
		{
			p->next_node=sub->next_node;
		} 
		else
		{
			p->next_node=NULL;
			ssd->channel_head[channel].subs_d_tail=p;
		}
	}
	
	return SUCCESS;	
}



Status services_2_delete(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag){

	int chip=0;
	struct sub_request * sub=NULL;

	if(ssd->channel_head[channel].subs_d_head==NULL)
	{
		return SUCCESS;
	}
	if (*channel_busy_flag==0)
	{
		                                                           
		sub=ssd->channel_head[channel].subs_d_head;
		
		while (sub!=NULL)
		{
			//if ((sub->current_state==SR_WAIT)&&(sub->location->channel==channel)&&(sub->location->chip==chip)&&(sub->location->die==die))      /*¸Ã×ÓÇëÇó¾ÍÊÇµ±Ç°dieµÄÇëÇó*/
			chip = sub->location->chip;
			if ((sub->current_state==SR_WAIT)&& ((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time))))
			{
				break;
			}
			sub=sub->next_node;
		}
		if (sub==NULL)
		{
			return SUCCESS;
		}

		if(sub->current_state==SR_WAIT)
		{
			sub->current_time=ssd->current_time;
			sub->current_state=SR_D_TRANSFER;
			sub->next_state=SR_COMPLETE;

			static_delete_select(ssd, sub->location->channel,sub->location->chip, sub->location->die,sub);   
					
			delete_del_sub_request(ssd,channel,sub);
			*channel_busy_flag=1;
		}		
	} 
}


/********************
写子请求的处理函数
*********************/
Status services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag)
{
	int j=0,chip=0;
	unsigned int k=0;
	unsigned int  old_ppn=0,new_ppn=0;
	unsigned int chip_token=0,die_token=0,plane_token=0,address_ppn=0;
	unsigned int  die=0,plane=0;
	long long time=0;
	struct sub_request * sub=NULL, * p=NULL;

    
	/************************************************************************************************************************
	*写子请求挂在两个地方一个是channel_head[channel].subs_w_head，另外一个是ssd->subs_w_head，所以要保证至少有一个队列不为空
	*同时子请求的处理还分为动态分配和静态分配
	*************************************************************************************************************************/
	if((ssd->channel_head[channel].subs_w_head!=NULL)||(ssd->subs_w_head!=NULL))      
	{
		if (ssd->parameter->allocation_scheme==0)                                      /*动态分配*/
		{
			for(j=0;j<ssd->channel_head[channel].chip;j++)					
			{		
				if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
				{
					break;
				}
				
				chip_token=ssd->channel_head[channel].token;                            /*令牌*/
				if (*channel_busy_flag==0)
				{
					if((ssd->channel_head[channel].chip_head[chip_token].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip_token].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip_token].next_state_predict_time<=ssd->current_time)))				
					{
							if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
							{
								break;
							}
							die_token=ssd->channel_head[channel].chip_head[chip_token].token;	

								sub=find_write_sub_request(ssd,channel);
								if(sub==NULL)
								{
									break;
								}
								
								if(sub->current_state==SR_WAIT)
								{
									plane_token=ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token;

									get_ppn(ssd,channel,chip_token,die_token,plane_token,sub);

									ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token=(ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token+1)%ssd->parameter->plane_die;

								
									go_one_step(ssd,sub,NULL,SR_W_TRANSFER);       /*执行普通的状态的转变。*/
									delete_w_sub_request(ssd,channel,sub);         /*删掉处理完后的写子请求*/
						
									*channel_busy_flag=1;
									/**************************************************************************
									*跳出for循环前，修改令牌
									*这里的token的变化完全取决于在这个channel chip die plane下写是否成功 
									*成功了就break 没成功token就要变化直到找到能写成功的channel chip die plane
									***************************************************************************/
									ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
									ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
									break;
								}
							

								
						ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
					}
				}
								
				ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
			}
		} 
		else if(ssd->parameter->allocation_scheme==1)                                     /*静态分配*/
		{
	
			if(ssd->channel_head[channel].subs_w_head==NULL)	return SUCCESS;

			if (*channel_busy_flag==0)
				{
							                                                            
					sub=ssd->channel_head[channel].subs_w_head;
					
					while (sub!=NULL)
					{
						chip = sub->location->chip;
						if ((sub->current_state==SR_WAIT)&& ((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time))))
						{
							break;
						}
						sub=sub->next_node;
					}

					if (sub==NULL)
					{
						return SUCCESS;
					}

					if(sub->current_state==SR_WAIT)
					{
						sub->current_time=ssd->current_time;
						sub->current_state=SR_W_TRANSFER;
						sub->next_state=SR_COMPLETE;
						/*处理静态分配下的写子请求*/ 
						static_write(ssd, sub->location->channel,sub->location->chip, sub->location->die,sub);  	
						
						delete_w_sub_request(ssd,channel,sub);
						*channel_busy_flag=1;
					}
							
				}
		}			
	}
	return SUCCESS;	
}


/********************************************************
*这个函数的主要功能是主控读子请求和写子请求的状态变化处理
*********************************************************/

struct ssd_info *process(struct ssd_info *ssd)   
{

	/*********************************************************************************************************
	*flag_die表示是否因为die的busy，阻塞了时间前进，-1表示没有，非-1表示有阻塞，
	*flag_die的值表示die号,old ppn记录在copyback之前的物理页号，用于判断copyback是否遵守了奇偶地址的限制；
	*two_plane_bit[8],two_plane_place[8]数组成员表示同一个channel上每个die的请求分配情况；
	*chg_cur_time_flag作为是否需要调整当前时间的标志位，当因为channel处于busy导致请求阻塞时，需要调整当前时间；
	*初始认为需要调整，置为1，当任何一个channel处理了传送命令或者数据时，这个值置为0，表示不需要调整；
	**********************************************************************************************************/
	int old_ppn=-1,flag_die=-1; 
	unsigned int i,chan,random_num;     
	unsigned int flag=0,new_write=0,flag2=0,flag_gc=0;       
	int64_t time, channel_time=MAX_INT64;
	struct sub_request *sub;          

#ifdef DEBUG
	printf("enter process,  current time:%lld\n",ssd->current_time);
#endif

	/*********************************************************
	*判断是否有读写子请求，如果有那么flag令为0，没有flag就为1
	*当flag为1时，若ssd中有gc操作这时就可以执行gc操作
	**********************************************************/
	for(i=0;i<ssd->parameter->channel_number;i++)
	{          
		if((ssd->channel_head[i].subs_r_head==NULL)&&(ssd->channel_head[i].subs_w_head==NULL)&&(ssd->subs_w_head==NULL))
		{
			flag=1;
		}
		else
		{
			flag=0;
			break;
		}
	}
	if(flag==1)
	{
		ssd->flag=1;
		/*SSD中有gc操作的请求*/                                                                
		if (ssd->gc_request>0) {	
			/*这个gc要求所有channel都必须遍历到*/
			gc(ssd,0,1);    
		}

		return ssd;
	}

	else { ssd->flag=0; }
		
	time = ssd->current_time;
	/*处理当前状态是SR_R_C_A_TRANSFER或者当前状态是SR_COMPLETE，或者下一状态是SR_COMPLETE并且下一状态预计时间小于当前状态时间*/
	services_2_r_cmd_trans_and_complete(ssd);
	/*产生一个随机数，保证每次从不同的channel开始查询*/  
	random_num=ssd->program_count%ssd->parameter->channel_number;                        

	/*****************************************
	*循环处理所有channel上的读写子请求
	*发读请求命令，传读写数据，都需要占用总线
	******************************************/
	for(chan=0;chan<ssd->parameter->channel_number;chan++)	     
	{
		i=(random_num+chan)%ssd->parameter->channel_number;
		flag=0;
		flag_gc=0; /*每次进入channel时，将gc的标志位置为0，默认认为没有进行gc操作*/
		if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))		
		{   
			/*有gc操作，需要进行一定的判断*/
			if (ssd->gc_request>0){

				if (ssd->channel_head[i].gc_command!=NULL){
					/*gc函数返回一个值，表示是否执行了gc操作，如果执行了gc操作，这个channel在这个时刻不能服务其他的请求*/
					flag_gc=gc(ssd,i,0);             
				}
				/*执行过gc操作，需要跳出此次循环*/
				if (flag_gc==1)  {  continue; }
			}

			/*处理处于等待状态的读子请求*/
			services_2_r_wait(ssd,i,&flag); 

			/*if there are no read request to take channel, we can serve write requests*/
			if(flag==0){	
				services_2_delete(ssd,i,&flag);
			}

			/*if there are no new read request and data is ready in some dies, send these data to controller and response this request*/
			if((flag==0)&&(ssd->channel_head[i].subs_r_head!=NULL)){

				services_2_r_data_trans(ssd,i,&flag);                    
			}
			/*if there are no read request to take channel, we can serve write requests*/ 	
			if(flag==0){	
				services_2_write(ssd,i,&flag);
			}

		}
	}
	return ssd;
}


/**************************************************************************
*这个函数非常重要，读子请求的状态转变，以及时间的计算都通过这个函数来处理
*还有写子请求的执行普通命令时的状态，以及时间的计算也是通过这个函数来处理的
****************************************************************************/
Status go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state)
{
	unsigned int i=0,j=0,k=0,m=0;
	long long time=0;
	struct sub_request * sub=sub1 ; 
	struct local * location=NULL;
	if(sub1==NULL)
	{
		return ERROR;
	}

	location=sub1->location;
	/***************************************************************************************************
	*处理普通命令时，读子请求的目标状态分为以下几种情况SR_R_READ，SR_R_C_A_TRANSFER，SR_R_DATA_TRANSFER
	*写子请求的目标状态只有SR_W_TRANSFER
	****************************************************************************************************/

	
	switch(aim_state)						
	{	
		case SR_R_READ:
		{   
			/*****************************************************************************************************
		    *这个目标状态是指flash处于读数据的状态，sub的下一状态就应该是传送数据SR_R_DATA_TRANSFER
		    *这时与channel无关，只与chip有关所以要修改chip的状态为CHIP_READ_BUSY，下一个状态就是CHIP_DATA_TRANSFER
		    ******************************************************************************************************/
			sub->current_time=ssd->current_time;
			sub->current_state=SR_R_READ;
			sub->next_state=SR_R_DATA_TRANSFER;
			ssd->parameter->time_characteristics.tR = page_read_time(ssd,sub->location->page,0);
			sub->next_state_predict_time=ssd->current_time+ssd->parameter->time_characteristics.tR;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_READ_BUSY;
			ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_DATA_TRANSFER;
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+ssd->parameter->time_characteristics.tR;

			break;
		}
		case SR_R_C_A_TRANSFER:
		{   
			/*******************************************************************************************************
			*目标状态是命令地址传输时，sub的下一个状态就是SR_R_READ
			*这个状态与channel，chip有关，所以要修改channel，chip的状态分别为CHANNEL_C_A_TRANSFER，CHIP_C_A_TRANSFER
			*下一状态分别为CHANNEL_IDLE，CHIP_READ_BUSY
			*******************************************************************************************************/
			sub->current_time=ssd->current_time;									
			sub->current_state=SR_R_C_A_TRANSFER;									
			sub->next_state=SR_R_READ;									
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;									
			sub->begin_time=ssd->current_time;

			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=sub->ppn;
			

			ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
			ssd->channel_head[location->channel].current_time=ssd->current_time;										
			ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
			ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
			ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
			ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;
			
			break;
		
		}
		case SR_R_DATA_TRANSFER:
		{   
			/**************************************************************************************************************
			*目标状态是数据传输时，sub的下一个状态就是完成状态SR_COMPLETE
			*这个状态的处理也与channel，chip有关，所以channel，chip的当前状态变为CHANNEL_DATA_TRANSFER，CHIP_DATA_TRANSFER
			*下一个状态分别为CHANNEL_IDLE，CHIP_IDLE。
			***************************************************************************************************************/
			sub->current_time=ssd->current_time;					
			sub->current_state=SR_R_DATA_TRANSFER;		
			sub->next_state=SR_COMPLETE;				
			sub->next_state_predict_time=ssd->current_time+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
			sub->complete_time=sub->next_state_predict_time;

			ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
			ssd->channel_head[location->channel].current_time=ssd->current_time;		
			ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
			ssd->channel_head[location->channel].next_state_predict_time=sub->next_state_predict_time;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
			ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
			ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub->next_state_predict_time;

			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;

			break;
		}
		case SR_W_TRANSFER:
		{
			/******************************************************************************************************
			*这是处理写子请求时，状态的转变以及时间的计算
			*虽然写子请求的处理状态也像读子请求那么多，但是写请求都是从上往plane中传输数据
			*这样就可以把几个状态当一个状态来处理，就当成SR_W_TRANSFER这个状态来处理，sub的下一个状态就是完成状态了
			*此时channel，chip的当前状态变为CHANNEL_TRANSFER，CHIP_WRITE_BUSY
			*下一个状态变为CHANNEL_IDLE，CHIP_IDLE
			*******************************************************************************************************/
			sub->current_time=ssd->current_time;
			sub->current_state=SR_W_TRANSFER;
			sub->next_state=SR_COMPLETE;
			ssd->parameter->time_characteristics.tPROG = page_program_time(ssd, sub->location->page);				
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG;
			sub->complete_time=sub->next_state_predict_time;		
			//time=sub->complete_time;
			time = sub->next_state_predict_time-ssd->parameter->time_characteristics.tPROG;



			ssd->channel_head[location->channel].current_state=CHANNEL_TRANSFER;										
			ssd->channel_head[location->channel].current_time=ssd->current_time;										
			ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;										
			ssd->channel_head[location->channel].next_state_predict_time=time;

			ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_WRITE_BUSY;										
			ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;									
			ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;										
			ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
			
			break;
		}
		default :  return ERROR;
		
	}//switch(aim_state)	

	return SUCCESS;
}
