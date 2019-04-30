/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º pagemap.h
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
 
#include "pagemap.h"


int page_type(struct ssd_info *ssd, unsigned int page_loc)
{
	int page_type_tlc=0;
	// if(page_loc == 0 || page_loc == 1)
	// 	page_type_tlc = 1;
	// else if(page_loc == 2 || page_loc == ssd->parameter->page_block-3)
	// 	page_type_tlc = 2;
	// else if(page_loc == ssd->parameter->page_block-2 || page_loc == ssd->parameter->page_block-1)
	// 	page_type_tlc = 3;
	// else 

	if(page_loc%3 == 0)         page_type_tlc = 1;

	else if(page_loc%3 == 1)    page_type_tlc = 2;

	else if(page_loc%3 == 2)    page_type_tlc = 3;

	if(page_type_tlc == 0)      { printf("tlc page type error\n"); getchar();}

	return page_type_tlc;
}


int page_program_time(struct ssd_info *ssd, unsigned int page_loc)
{
	int tlc_time=0;

	int page_type_tlc = 0;

	page_type_tlc = page_type(ssd,page_loc);

	if(page_type_tlc == 1)           tlc_time = ssd->parameter->time_characteristics.tpLSB;

	else if(page_type_tlc == 2 )     tlc_time = ssd->parameter->time_characteristics.tpCSB;

	else if(page_type_tlc == 3)      tlc_time = ssd->parameter->time_characteristics.tpMSB;

	if(tlc_time == 0)                { printf("tlc time error\n"); getchar(); }

	return tlc_time;
}



int page_read_time(struct ssd_info *ssd, unsigned int page_loc, int pv)
{
	int tlc_time=0;

	int page_type_tlc = 0;

	page_type_tlc = page_type(ssd,page_loc);

	if(page_type_tlc == 1)         tlc_time = ssd->parameter->time_characteristics.trLSB;

	else if(page_type_tlc == 2 )   tlc_time = ssd->parameter->time_characteristics.trCSB;

	else if(page_type_tlc == 3)    tlc_time = ssd->parameter->time_characteristics.trMSB;

	if(tlc_time == 0)              { printf("tlc time error\n"); getchar(); }

	return tlc_time;
}




/************************************************************************************
*函数的功能是根据物理页号ppn查找该物理页所在的channel，chip，die，plane，block，page
*得到的channel，chip，die，plane，block，page放在结构location中并作为返回值
*************************************************************************************/
struct local *find_location(struct ssd_info *ssd,unsigned int ppn)
{
	struct local *location=NULL;

	int page_plane=0,page_die=0,page_chip=0,page_channel=0;

#ifdef DEBUG
	printf("enter find_location\n");
#endif

	location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
	memset(location,0, sizeof(struct local));

	page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;

	page_die=page_plane*ssd->parameter->plane_die;

	page_chip=page_die*ssd->parameter->die_chip;

	page_channel=page_chip*ssd->parameter->chip_channel[0];
	
	/*******************************************************************************
	*page_channel是一个channel中page的数目， ppn/page_channel就得到了在哪个channel中
	*用同样的办法可以得到chip，die，plane，block，page
	********************************************************************************/
	location->channel = ppn/page_channel;

	location->chip = (ppn%page_channel)/page_chip;

	location->die = ((ppn%page_channel)%page_chip)/page_die;

	location->plane = (((ppn%page_channel)%page_chip)%page_die)/page_plane;

	location->block = ((((ppn%page_channel)%page_chip)%page_die)%page_plane)/ssd->parameter->page_block;

	location->page = (((((ppn%page_channel)%page_chip)%page_die)%page_plane)%ssd->parameter->page_block)%ssd->parameter->page_block;

	return location;
}


/*****************************************************************************
*这个函数的功能是根据参数channel，chip，die，plane，block，page，找到该物理页号
*函数的返回值就是这个物理页号
******************************************************************************/
unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
	unsigned int ppn=0;
	unsigned int i=0;
	int page_plane=0,page_die=0,page_chip=0;
	int page_channel[100];                 /*这个数组存放的是每个channel的page数目*/

#ifdef DEBUG
	printf("enter find_psn,channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n",channel,chip,die,plane,block,page);
#endif
    
	/*********************************************
	计算出plane，die，chip，channel中的page的数目
	**********************************************/
	page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
	page_die=page_plane*ssd->parameter->plane_die;
	page_chip=page_die*ssd->parameter->die_chip;

	while(i<ssd->parameter->channel_number)
	{
		page_channel[i]=ssd->parameter->chip_channel[i]*page_chip;
		i++;
	}
    /****************************************************************************
	*计算物理页号ppn，ppn是channel，chip，die，plane，block，page中page个数的总和
	*****************************************************************************/
	i=0;
	while(i<channel)
	{
		ppn=ppn+page_channel[i];
		i++;
	}
	ppn=ppn+page_chip*chip+page_die*die+page_plane*plane+block*ssd->parameter->page_block+page;
	
	return ppn;
}


/**************************************************
*读请求预处理函数，当读请求所读得页里面没有数据时，
*需要预处理网该页里面写数据，以保证能读到数据
***************************************************/
struct ssd_info *pre_process_page(struct ssd_info *ssd)
{

	unsigned int device=1,lsn=0,size=0,ope=0,lpn=0,full_page=0; //初始化
	unsigned int largest_lsn,sub_size,ppn,add_size=0;
	unsigned int i=0,j,k;
	int map_entry_new,map_entry_old,modify;
	int flag=0;
	char buffer_request[200];
	struct local *location;
	int64_t time;

	printf("begin pre_process_page.................\n");

	ssd->tracefile=fopen(ssd->tracefilename,"r");
 	/*打开trace文件从中读取请求*/
	if(ssd->tracefile == NULL )     
	{
		printf("the trace file can't open\n");

		return NULL;
	}

	full_page=~(0xffffffff<<(ssd->parameter->subpage_page)); // 每一个subpage用 1 bit表示
	/*计算出这个ssd的最大逻辑扇区号*/
	largest_lsn=(unsigned int )((ssd->parameter->chip_num*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));

	while(fgets(buffer_request,200,ssd->tracefile))
	{		
		read_data(ssd,buffer_request, &time, &lsn, &size, &ope);
		/*断言，当读到的time，device，lsn，size，ope不合法时就会处理*/
		trace_assert(time,device,lsn,size,ope);                         
		/*add_size是这个请求已经预处理的大小*/
		add_size=0;                                                     
		/*这里只是读请求的预处理，需要提前将相应位置的信息进行相应修改*/
		if(ope==1)                                                       
		{
			while(add_size<size)
			{	
				/*防止获得的lsn比最大的lsn还大*/
				lsn=lsn%largest_lsn;                                     

				sub_size=ssd->parameter->subpage_page-(lsn%ssd->parameter->subpage_page);	
				/*只有当一个请求的大小小于一个page的大小时或者是处理一个请求的最后一个page时会出现这种情况*/
				if(add_size+sub_size>=size)                            
				{		
					sub_size=size-add_size;		
					add_size+=sub_size;		
				}
				/*当预处理一个子大小时，这个大小大于一个page或是已经处理的大小大于size就报错*/
				if((sub_size>ssd->parameter->subpage_page)||(add_size>size))			
				{		
					printf("pre_process sub_size:%d\n",sub_size);		
				}

                /*******************************************************************************************************
				*利用逻辑扇区号lsn计算出逻辑页号lpn
				*判断这个dram中映射表map中在lpn位置的状态
				*A，这个状态==0，表示以前没有写过，现在需要直接将ub_size大小的子页写进去写进去
				*B，这个状态>0，表示，以前有写过，这需要进一步比较状态，因为新写的状态可以与以前的状态有重叠的扇区的地方
				********************************************************************************************************/
				lpn=lsn/ssd->parameter->subpage_page;
				if(ssd->dram->map->map_entry[lpn].state==0)                 /*状态为0的情况*/
				{
					/**************************************************************
					*获得利用get_ppn_for_pre_process函数获得ppn，再得到location
					*修改ssd的相关参数，dram的映射表map，以及location下的page的状态
					***************************************************************/
					ppn=get_ppn_for_pre_process(ssd,lsn);                  
					location=find_location(ssd,ppn);
					//更新mapping table的信息
					ssd->dram->map->map_entry[lpn].pn=ppn;	
					ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,lsn,sub_size);   //0001
					//ssd->dram->map->map_entry[lpn].pos = location->page%4;
					//更新page的信息
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=lpn;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=ssd->dram->map->map_entry[lpn].state;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);
					// 更新计数状态
					ssd->program_count++;	
					ssd->channel_head[location->channel].program_count++;
					ssd->channel_head[location->channel].chip_head[location->chip].program_count++;		
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].program_count++;

					free(location);
					location=NULL;
				}//if(ssd->dram->map->map_entry[lpn].state==0)
				else if(ssd->dram->map->map_entry[lpn].state>0){         /*状态不为0的情况*/
					/*得到新的状态，并与原来的状态相或的到一个状态*/
					map_entry_new=set_entry_state(ssd,lsn,sub_size);      
					map_entry_old=ssd->dram->map->map_entry[lpn].state;
                    modify=map_entry_new|map_entry_old;
                    ssd->dram->map->map_entry[lpn].state=modify;

					ppn=ssd->dram->map->map_entry[lpn].pn;
					location=find_location(ssd,ppn);
					
					//ssd->dram->map->map_entry[lpn].pos = location->page%4;


					if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!= lpn)
					{
						printf("\nError in pre_process_page(), the lpn isn't the same as the lpn stored in page \n");
						getchar();
					}

					ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state=modify; 
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=modify;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~modify)&full_page);
					
					ssd->program_count++;	
					ssd->channel_head[location->channel].program_count++;
					ssd->channel_head[location->channel].chip_head[location->chip].program_count++;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].program_count++;	

					free(location);
					location=NULL;
				}//else if(ssd->dram->map->map_entry[lpn].state>0)
				lsn=lsn+sub_size;                                         /*下个子请求的起始位置*/
				add_size+=sub_size;                                       /*已经处理了的add_size大小变化*/
			}//while(add_size<size)
		}//if(ope==1) 
	}	

	printf("pre_process is complete!\n");

	fclose(ssd->tracefile);

	// 写入到统计文件
	for(i=0;i<ssd->parameter->channel_number;i++)
    for(j=0;j<ssd->parameter->die_chip;j++)
	for(k=0;k<ssd->parameter->plane_die;k++)
	{
		fprintf(ssd->outputfile,"chip:%d,die:%d,plane:%d have free page: %d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);				
		fflush(ssd->outputfile);
	}
	
	return ssd;
}


// 请求入队列时,分配channel chip die plane, 请求开始处理的时候分配 block和page
struct local *init_location(struct ssd_info *ssd,unsigned int lpn)
{

	unsigned int channel=0,chip=0,die=0,plane=0; 
	unsigned int ppn;
	unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;


	struct local *location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
	memset(location,0, sizeof(struct local));


	channel_num=ssd->parameter->channel_number;
	chip_num=ssd->parameter->chip_channel[0];
	die_num=ssd->parameter->die_chip;
	plane_num=ssd->parameter->plane_die;
	
	/*动态方式下获取ppn*/
	if (ssd->parameter->allocation_scheme==0)                           
	{
		/*表示全动态方式下，也就是channel，chip，die，plane，block等都是动态分配*/
		if (ssd->parameter->dynamic_allocation==0)                      
		{
			channel=ssd->token;
			ssd->token=(ssd->token+1)%ssd->parameter->channel_number;
			chip=ssd->channel_head[channel].token;
			ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
			die=ssd->channel_head[channel].chip_head[chip].token;
			ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
		}
		/*表示半动态方式，channel静态给出，package，die，plane动态分配*/ 
		else if (ssd->parameter->dynamic_allocation==1)                                  
		{
			channel=lpn%ssd->parameter->channel_number;
			chip=ssd->channel_head[channel].token;
			ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
			die=ssd->channel_head[channel].chip_head[chip].token;
			ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
		}
	}
	/*表示静态分配，同时也有0,1,2,3,4,5这6中不同静态分配方式*/ 
	else if (ssd->parameter->allocation_scheme==1)                       
	{
		switch (ssd->parameter->static_allocation)
		{
			
			case 0:         
			{
				channel=(lpn/(plane_num*die_num*chip_num))%channel_num;
				chip=lpn%chip_num;
				die=(lpn/chip_num)%die_num;
				plane=(lpn/(die_num*chip_num))%plane_num;
				break;
			}
		case 1:
			{
				channel=lpn%channel_num;
				chip=(lpn/channel_num)%chip_num;
				die=(lpn/(chip_num*channel_num))%die_num;
				plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
							
				break;
			}
		case 2:
			{
				channel=lpn%channel_num;
				chip=(lpn/(plane_num*channel_num))%chip_num;
				die=(lpn/(plane_num*chip_num*channel_num))%die_num;
				plane=(lpn/channel_num)%plane_num;
				break;
			}
		case 3:
			{
				channel=lpn%channel_num;
				chip=(lpn/(die_num*channel_num))%chip_num;
				die=(lpn/channel_num)%die_num;
				plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
				break;
			}
		case 4:  
			{
				channel=lpn%channel_num;
				chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
				die=(lpn/(plane_num*channel_num))%die_num;
				plane=(lpn/channel_num)%plane_num;
							
				break;
			}
		case 5:   
			{
				channel=lpn%channel_num;
				chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
				die=(lpn/channel_num)%die_num;
				plane=(lpn/(die_num*channel_num))%plane_num;
							
				break;
			}
		default : return 0;
		}
	}


	location->channel =channel;
	location->chip=chip;
	location->die=die;
	location->plane=plane;


	return location;
	
}

/**************************************
*函数功能是为预处理函数获取物理页号ppn
**************************************/
unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn)     
{
	unsigned int lpn,ppn;
	struct local *location=NULL;
	int last_write_page=0;

#ifdef DEBUG
	printf("enter get_psn_for_pre_process\n");
#endif

	lpn=lsn/ssd->parameter->subpage_page;
	//不同的分配方法计算出lpn的初始地址, block和page的地址由find_active_block_SD()地址计算函数完成
	location = init_location(ssd,lpn); 
    
	//根据上述分配方法找到channel，chip，die，plane后，再在这个里面找到active_block *接着获得ppn
	if(find_active_block_select(ssd,location,NULL)==FAILURE)  
	{
		printf("the read operation is expand the capacity of SSD\n");
		
		free(location);
	
		location=NULL;

		getchar();

		return 0;
	}

	if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!= -1)
	{
		printf("\nError in get_ppn_for_pre_process(), the page has been written\n");
		getchar();
	}

	ppn=find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);

	free(location);
	
	location=NULL;

	return ppn;
}

/***************************************************************************************************
**函数功能是在所给的channel，chip，die，plane里面找到一个active_block然后再在这个block里面找到一个页，
*再利用find_ppn找到ppn
****************************************************************************************************/
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)
{
	int old_ppn=-1;
	unsigned int ppn,lpn,full_page;
	unsigned int active_block;
	unsigned int block,page;

	unsigned int old_state=0,state=0;
	struct local *location=NULL;
	struct direct_erase *direct_erase_node=NULL,*new_direct_erase=NULL;
	struct gc_operation *gc_node=NULL;

	unsigned int i=0,j=0,k=0,l=0,m=0,n=0;

#ifdef DEBUG
	printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif

	full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
	lpn=sub->lpn;
    
	/*************************************************************************************
	*利用函数find_active_block在channel，chip，die，plane找到活跃block
	*并且修改这个channel，chip，die，plane，active_block下的last_write_page和free_page_num
	**************************************************************************************/
	if(find_active_block_select(ssd,sub->location,sub)==FAILURE)                      
	{
		printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
		printf("caused by static allocation_scheme, could be redirection to another location(channel,chip,die,plane)\n");	
		return ssd;
	}

	block = sub->location->block;
	page= sub->location->page;
	/*this is the first logical page*/
	if(ssd->dram->map->map_entry[lpn].state==0)                                       
	{
		if(ssd->dram->map->map_entry[lpn].pn!=0)
		{
			printf("mapping table Error in get_ppn()\n");
		}

		if(	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn!= -1)
		{
			printf("\nError in get_ppn(), the page has been written\n");
			getchar();
		}

		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state=sub->state;
		//ssd->dram->map->map_entry[lpn].pos = page%4;
	}
	else  /*这个逻辑页进行了更新，需要将原来的页置为失效*/
	{
		ppn=ssd->dram->map->map_entry[lpn].pn;
		location=find_location(ssd,ppn);
		// 校验lpn与ppn的一致性
		if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!=lpn)
		{
			printf("\nError in get_ppn(), the lpn isn't the same as the lpn stored in page \n");
			getchar();
		}

		//记录当前的历史ppn地址
		ssd->dram->map->map_entry[lpn].history_ppn[ssd->dram->map->map_entry[lpn].count]=ppn; 
		ssd->dram->map->map_entry[lpn].count++; //更新计数

		if(ssd->dram->map->map_entry[lpn].count==15){ //达到最大的历史数

			for (int i = 0; i < ssd->dram->map->map_entry[lpn].count-5; i++)
			{
				ssd->dram->map->map_entry[lpn].history_ppn[i] = ssd->dram->map->map_entry[lpn].history_ppn[i+5];
			}

			ssd->dram->map->map_entry[lpn].count = 10;
		}


		//无效老的physical page 表示某一页失效，同时标记valid和free状态都为0
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;             
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;              
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=-1;
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		
		/*******************************************************************************************
		*该block中全是invalid的页，可以直接删除，就在创建一个可擦除的节点，挂在location下的plane下面
		********************************************************************************************/
		if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==ssd->parameter->page_block)    
		{
			new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
            alloc_assert(new_direct_erase,"new_direct_erase");
			memset(new_direct_erase,0, sizeof(struct direct_erase));

			new_direct_erase->block=location->block;
			new_direct_erase->next_node=NULL;
			direct_erase_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
			if (direct_erase_node==NULL)
			{
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			} 
			else
			{
				new_direct_erase->next_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			}
		}

		free(location);
		location=NULL;

		// 更新位置信息,在group内的位置 0,1,2,3
		//ssd->dram->map->map_entry[lpn].pos = page%4;
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[lpn].state|sub->state);
	}

	/*修改sub子请求的ppn，location等变量*/
	sub->ppn=ssd->dram->map->map_entry[lpn].pn;   
	/*修改ssd的program_count,free_page等变量*/
	ssd->program_count++;                        
	ssd->write_flash_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].program_count++;
	
	// 更新最新的page的元数据信息
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn=lpn;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].valid_state=sub->state;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].written_count++;
	/*如果没有主动策略，只采用gc_hard_threshold，并且无法中断GC过程**/
    /*如果plane中的free_page的数目少于gc_hard_threshold所设定的阈值就产生gc操作*/
    //plane free page少于阈值 则提出GC
	if (ssd->parameter->active_write==0)                                            
	{                                                                              
		if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
		{
			gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
			alloc_assert(gc_node,"gc_node");
			memset(gc_node,0, sizeof(struct gc_operation));

			gc_node->next_node=NULL;
			gc_node->chip=chip;
			gc_node->die=die;
			gc_node->plane=plane;
			gc_node->block=0xffffffff;
			gc_node->page=0;
			gc_node->state=GC_WAIT;
			gc_node->priority=GC_UNINTERRUPT;
			//挂载在channel节点
			gc_node->next_node=ssd->channel_head[channel].gc_command;
			ssd->channel_head[channel].gc_command=gc_node;
			ssd->gc_request++;
		}
	} 

	return ssd;
}

/*****************************************************************************************
*这个函数功能是为gc操作寻找新的ppn，因为在gc操作中需要找到新的物理块存放原来物理块上的数据
*在gc中寻找新物理块的函数，不会引起循环的gc操作
******************************************************************************************/
 unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	unsigned int ppn;
	unsigned int active_block,block,page;

	struct local *location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
	memset(location,0, sizeof(struct local));

	location->channel = channel;
	location->chip = chip;
	location->die = die;
	location->plane = plane;

#ifdef DEBUG
	printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif

	if(find_active_block_select(ssd,location,NULL)!=SUCCESS)
	{
		printf("\n\n Error int get_ppn_for_gc().\n");
		return 0xffffffff;
	}
    
	block= location->block;

	page= location->page;	

	ppn=find_ppn(ssd,channel,chip,die,plane,block,page);
	//更新 SSD内统计信息
	ssd->program_count++;
	ssd->write_flash_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].program_count++;

	//更新page的信息，可以用作wear leveling
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].written_count++;

	return ppn;
}


/*********************************************************************************************************************
* 函数的功能就是erase_operation擦除操作，把channel，chip，die，plane下的block擦除掉
*也就是初始化这个block的相关参数，eg：free_page_num=page_block，invalid_page_num=0，last_write_page=-1，erase_count++
*还有这个block下面的每个page的相关参数也要修改
*********************************************************************************************************************/
Status erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die ,unsigned int plane ,unsigned int block)
{
	unsigned int i=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num=ssd->parameter->page_block;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=-1;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;

	for (i=0;i<ssd->parameter->page_block;i++)
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state=PG_SUB;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state=0;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn=-1;
	}

	ssd->erase_count++;
	ssd->channel_head[channel].erase_count++;			
	ssd->channel_head[channel].chip_head[chip].erase_count++;

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page+=ssd->parameter->page_block;

	return SUCCESS;

}


/**************************************************************************************
*这个函数的功能是处理INTERLEAVE_TWO_PLANE，INTERLEAVE，TWO_PLANE，NORMAL下的擦除的操作
***************************************************************************************/
Status erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
	unsigned int die=0;
	unsigned int plane=0;
	unsigned int block=0;
	struct direct_erase *direct_erase_node=NULL;
	unsigned int block0=0xffffffff;
	unsigned int block1=0;

	if((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node==NULL)||               
		((command!=INTERLEAVE_TWO_PLANE)&&(command!=INTERLEAVE)&&(command!=TWO_PLANE)&&(command!=NORMAL)))      /*如果没有擦除操作，或者command不对，返回错误*/          
	{
		return ERROR;
	}

	/************************************************************************************************************
	**处理擦除操作时，首先要传送擦除命令，这是channel，chip处于传送命令的状态，即CHANNEL_TRANSFER，CHIP_ERASE_BUSY
	*下一状态是CHANNEL_IDLE，CHIP_IDLE
	*************************************************************************************************************/
	block1=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;
	
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

	direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;
	block=direct_erase_node->block;
	ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node=direct_erase_node->next_node;
	free(direct_erase_node);
	direct_erase_node=NULL;
	erase_operation(ssd,channel,chip,die1,plane1,block);
	// 不包括迁移page的擦除操作
	ssd->direct_erase_count++; 
	ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;       								
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+ssd->parameter->time_characteristics.tBERS;	

	direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;

	if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
	{
		return FAILURE; 
	}
	else
	{
		return SUCCESS;
	}
}


/*******************************************************************************************************************
*GC操作由某个plane的free块少于阈值进行触发，当某个plane被触发时，GC操作占据这个plane所在的die，因为die是一个独立单元。
*对一个die的GC操作，尽量做到四个plane同时erase，利用interleave erase操作。GC操作应该做到可以随时停止（移动数据和擦除
*时不行，但是间隙时间可以停止GC操作），以服务新到达的请求，当请求服务完后，利用请求间隙时间，继续GC操作。可以设置两个
*GC阈值，一个软阈值，一个硬阈值。软阈值表示到达该阈值后，可以开始主动的GC操作，利用间歇时间，GC可以被新到的请求中断；
*当到达硬阈值后，强制性执行GC操作，且此GC操作不能被中断，直到回到硬阈值以上。
*在这个函数里面，找出这个die所有的plane中，有没有可以直接删除的block，要是有的话，利用interleave two plane命令，删除
*这些block，否则有多少plane有这种直接删除的block就同时删除，不行的话，最差就是单独这个plane进行删除，连这也不满足的话，
*直接跳出，到gc_parallelism函数进行进一步GC操作。该函数寻找全部为invalid的块，直接删除，找到可直接删除的返回1，没有找
*到返回-1。
*********************************************************************************************************************/
int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	struct direct_erase * direct_erase_node1=NULL;

	direct_erase_node1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
	
	if (direct_erase_node1==NULL)  return FAILURE;
	/*不是每个plane都有可以直接删除的block，只对当前plane进行普通的erase操作，或者只能执行普通命令*/
	if (erase_planes(ssd,channel,chip,die,plane,NORMAL)==SUCCESS)      return SUCCESS;
	
	else return FAILURE; /*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作*/
}


Status move_page(struct ssd_info * ssd, struct local *location, unsigned int * transfer_size)
{
	struct local *new_location=NULL;
	unsigned int free_state=0,valid_state=0;
	unsigned int lpn=0,old_ppn=0,ppn=0;

	lpn=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
	valid_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
	free_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
	 /*记录这个有效移动页的ppn，对比map或者额外映射关系中的ppn，进行删除和添加操作*/
	old_ppn=find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);  
	/*找出来的ppn一定是在发生gc操作的plane中,才能使用copyback操作，为gc操作获取ppn*/    
	ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane);                
	/*根据新获得的ppn获取new_location*/   
	new_location=find_location(ssd,ppn); 
                                                               
	(* transfer_size)+=size(valid_state);
	
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;


	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=-1;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
 	/*修改映射表*/
	if (old_ppn==ssd->dram->map->map_entry[lpn].pn)                                                    
	{
		ssd->dram->map->map_entry[lpn].pn=ppn;
	}

	free(new_location);
	new_location=NULL;

	return SUCCESS;
}

/*******************************************************************************************************************************************
*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作，用在不能中断的gc操作中，成功删除一个块，返回1，没有删除一个块返回-1
*在这个函数中，不用考虑目标channel,die是否是空闲的,擦除invalid_page_num最多的block
********************************************************************************************************************************************/
int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)       
{
	unsigned int i=0,invalid_page=0;
	unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*记录失效页最多的块号*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;

	int LSB_count=0, MSB_count=0, CSB_count=0;
	int LSB_program=0, CSB_program=0, MSB_program=0;
	int LSB_read=0, CSB_read=0, MSB_read=0;

	int page_type_tlc=0;



 	/*获取活跃 可写块*/
	unsigned int free_page_num=0;
	unsigned int count=0;
	
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	//last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	while((free_page_num==0)&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		count++;
	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;

	if(count<ssd->parameter->block_plane)  return SUCCESS;
	
	else	return ERROR;

	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

	invalid_page=0;
	transfer_size=0;
	block=-1;
	/*查找最多invalid_page的块号，以及最大的invalid_page_num*/
	for(i=0;i<ssd->parameter->block_plane;i++)                                                            
	{	
		total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;

		if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
		{				
			invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
			block=i;						
		}
	}

	if (block==-1)     return ERROR;
	

	LSB_program = ssd->parameter->time_characteristics.tpLSB;
	CSB_program = ssd->parameter->time_characteristics.tpCSB;
	MSB_program = ssd->parameter->time_characteristics.tpMSB;

	LSB_read = ssd->parameter->time_characteristics.trLSB;
	CSB_read = ssd->parameter->time_characteristics.trCSB;
	MSB_read = ssd->parameter->time_characteristics.trMSB;

	free_page=0;
	/*逐个检查每个page，如果有有效数据的page需要移动到其他地方存储*/
	unsigned int mask=~(0xffffffff<<(ssd->parameter->subpage_page)); // 每1 bit表示 1个subpage

	for(i=0;i<ssd->parameter->page_block;i++)		                                                    
	{	
		// 表示当前为空
		if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==mask)
		{
			free_page++;
		}
		// if(free_page!=0)
		// {
		// 	printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
		// }
		/*该页是有效页，需要move_page操作*/	
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) 		
		{	
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));

			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block;
			location->page=i;

			page_type_tlc = page_type(ssd,i);

			if(page_type_tlc == 1)  LSB_count++;

			else if(page_type_tlc == 2)   CSB_count++;

			else if(page_type_tlc == 3)   MSB_count++;

			/*真实的move_page操作*/
			move_page( ssd, location, &transfer_size);                                                   
			
			page_move_count++;

			free(location);	
			location=NULL;
		}				
	}
	/*执行完move_page操作后，就立即执行block的擦除操作*/
	erase_operation(ssd,channel ,chip , die,plane ,block);	                                              
	ssd->channel_head[channel].current_state=CHANNEL_GC;

	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			
	
	/***************************************************************
	*在可执行COPYBACK高级命令与不可执行COPYBACK高级命令这两种情况下，
	*channel下个状态时间的计算，以及chip下个状态时间的计算
	***************************************************************/
	ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC)+LSB_count*(LSB_program+LSB_read)+MSB_count*(MSB_program+MSB_read)+CSB_count*(CSB_program+CSB_read)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC);					
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

	return 1;
}


/*******************************************************************************************************************************************
*目标的plane没有可以直接删除的block，需要寻找目标擦除块后在实施擦除操作，用在可以中断的gc操作中，成功删除一个块，返回1，没有删除一个块返回-1
*在这个函数中，不用考虑目标channel,die是否是空闲的
********************************************************************************************************************************************/
int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node)        
{
	unsigned int i,block,active_block,transfer_size,invalid_page=0;
	struct local *location;

	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	transfer_size=0;
	if (gc_node->block>=ssd->parameter->block_plane)
	{
		for(i=0;i<ssd->parameter->block_plane;i++)
		{			
			if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
			{				
				invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
				block=i;						
			}
		}
		gc_node->block=block;
	}

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num!=ssd->parameter->page_block)     /*»¹ÐèÒªÖ´ÐÐcopyback²Ù×÷*/
	{
		for (i=gc_node->page;i<ssd->parameter->page_block;i++)
		{
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].page_head[i].valid_state>0)
			{
				location=(struct local * )malloc(sizeof(struct local ));
				alloc_assert(location,"location");
				memset(location,0, sizeof(struct local));

				location->channel=channel;
				location->chip=chip;
				location->die=die;
				location->plane=plane;
				location->block=block;
				location->page=i;
				transfer_size=0;

				move_page( ssd, location, &transfer_size);

				free(location);
				location=NULL;

				gc_node->page=i+1;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num++;
				ssd->channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[channel].current_time=ssd->current_time;										
				ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[channel].chip_head[chip].current_state=CHIP_COPYBACK_BUSY;								
				ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
				ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;		

				if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
				{					
					ssd->channel_head[channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC;		
					ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
				} 
				else
				{	
					ssd->channel_head[channel].next_state_predict_time=ssd->current_time+(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC;					
					ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
				}
				return 0;    
			}
		}
	}
	else
	{
		erase_operation(ssd,channel ,chip, die,plane,gc_node->block);	

		ssd->channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;								
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;							
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

		return 1;                                                                      /*该gc操作完成，返回1，可以将channel上的gc请求节点删除*/
	}

	printf("there is a problem in interrupt_gc\n");
	return 1;
}

/*************************************************************
*函数的功能是当处理完一个gc操作时，需要把gc链上的gc_node删除掉
**************************************************************/
int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node)
{
	struct gc_operation *gc_pre=NULL;
	if(gc_node==NULL)                                                                  
	{
		return ERROR;
	}

	if (gc_node==ssd->channel_head[channel].gc_command)
	{
		ssd->channel_head[channel].gc_command=gc_node->next_node;
	}
	else
	{
		gc_pre=ssd->channel_head[channel].gc_command;
		while (gc_pre->next_node!=NULL)
		{
			if (gc_pre->next_node==gc_node)
			{
				gc_pre->next_node=gc_node->next_node;
				break;
			}
			gc_pre=gc_pre->next_node;
		}
	}
	free(gc_node);
	gc_node=NULL;
	ssd->gc_request--;
	return SUCCESS;
}

/***************************************
*这个函数的功能是处理channel的每个gc操作
****************************************/
Status gc_for_channel(struct ssd_info *ssd, unsigned int channel)
{
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int chip,die,plane,flag_priority=0;
	unsigned int current_state=0, next_state=0;
	long long next_state_predict_time=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;

	/*******************************************************************************************
	*查找每一个gc_node，获取gc_node所在的chip的当前状态，下个状态，下个状态的预计时间
	*如果当前状态是空闲，或是下个状态是空闲而下个状态的预计时间小于当前时间，并且是不可中断的gc
	*那么就flag_priority令为1，否则为0
	********************************************************************************************/
	gc_node=ssd->channel_head[channel].gc_command;
	while (gc_node!=NULL)
	{
		current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
		next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
		next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
		if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))
		{
			if (gc_node->priority==GC_UNINTERRUPT)                                    /*这个gc请求是不可中断的，优先服务这个gc操作*/
			{
				flag_priority=1;
				break;
			}
		}
		gc_node=gc_node->next_node;
	}
	if (flag_priority!=1)                                                              /*没有找到不可中断的gc请求，首先执行队首的gc请求*/
	{
		gc_node=ssd->channel_head[channel].gc_command;
		while (gc_node!=NULL)
		{
			current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
			next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
			next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
			 /**********************************************
			 *需要gc操作的目标chip是空闲的，才可以进行gc操作
			 ***********************************************/
			if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))   
			{
				break;
			}
			gc_node=gc_node->next_node;
		}

	}


	if(gc_node==NULL)
	{
		return FAILURE;
	}

	
	// 获取GC的chip die
	chip=gc_node->chip;
	die=gc_node->die;
	plane=gc_node->plane;

	if (gc_node->priority==GC_UNINTERRUPT)
	{
		flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
		if (flag_direct_erase!=SUCCESS)
		{
			flag_gc=uninterrupt_gc(ssd,channel,chip,die,plane);                         /*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
			if (flag_gc==1)
			{
				delete_gc_node(ssd,channel,gc_node);
			}
		}
		else
		{
			delete_gc_node(ssd,channel,gc_node);
		}


		return SUCCESS;
	}
	/*******************************************************************************
	*可中断的gc请求，需要首先确认该channel上没有子请求在这个时刻需要使用这个channel，
	*没有的话，在执行gc操作，有的话，不执行gc操作
	 这部分的代码一直没有被执行?????
	********************************************************************************/
	else        
	{
		printf("gc_node: %d\n", gc_node->priority);
		/*判断是否有子请求需要channel，如果有子请求需要这个channel，那么这个gc操作就被中断*/
		flag_invoke_gc=decide_gc_invoke(ssd,channel);                                  

		if (flag_invoke_gc==1)
		{
			flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
			if (flag_direct_erase==-1)
			{
				/*当一个完整的gc操作完成时（已经擦除一个块，回收了一定数量的flash空间），返回1，将channel上相应的gc操作请求节点删除*/
				flag_gc=interrupt_gc(ssd,channel,chip,die,plane,gc_node);             
				if (flag_gc==1)
				{
					delete_gc_node(ssd,channel,gc_node);
				}
			}
			else if (flag_direct_erase==1)
			{
				delete_gc_node(ssd,channel,gc_node);
			}
			return SUCCESS;
		} 
		else
		{
			return FAILURE;
		}		
	}
}



/************************************************************************************************************
*flag用来标记gc函数是在ssd整个都是idle的情况下被调用的（1），还是确定了channel，chip，die，plane被调用（0）
*进入gc函数，需要判断是否是不可中断的gc操作，如果是，需要将一整块目标block完全擦除后才算完成；如果是可中断的，
*在进行GC操作前，需要判断该channel，die是否有子请求在等待操作，如果没有则开始一步一步的操作，找到目标
*块后，一次执行一个copyback操作，跳出gc函数，待时间向前推进后，再做下一个copyback或者erase操作
*进入gc函数不一定需要进行gc操作，需要进行一定的判断，当处于硬阈值以下时，必须进行gc操作；当处于软阈值以下时，
*需要判断，看这个channel上是否有子请求在等待(有写子请求等待就不行，gc的目标die处于busy状态也不行)，如果
*有就不执行gc，跳出，否则可以执行一步操作
************************************************************************************************************/
unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag)
{
	unsigned int i;
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int flag_priority=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;

	if (flag==1)                                                                       /*整个ssd都是IDEL的情况*/
	{
		for (i=0;i<ssd->parameter->channel_number;i++)
		{
			flag_priority=0;
			flag_direct_erase=1;
			flag_gc=1;
			flag_invoke_gc=1;
			gc_node=NULL;
			gc_p=NULL;
			if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))
			{
				channel=i;
				if (ssd->channel_head[channel].gc_command!=NULL) 
				{
					gc_for_channel(ssd, channel);
				}
			}
		}
		return SUCCESS;

	} 
	else                                                                                 /*只需针对某个特定的channel，chip，die进行gc请求的操作(只需对目标die进行判定，看是不是idle）*/
	{
		if ((ssd->parameter->allocation_scheme==1)||((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==1)))
		{
			if ((ssd->channel_head[channel].subs_r_head!=NULL)||(ssd->channel_head[channel].subs_w_head!=NULL))    /*队列上有请求，先服务请求*/
			{
				return 0;
			}
		}

		gc_for_channel(ssd,channel);
		return SUCCESS;
	}
} 



/**********************************************************
*判断是否有子请求channel，若果没有返回1就可以发送gc操作
*如果有返回0，就不能执行gc操作，gc操作被中断
***********************************************************/
int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel)      
{
	struct sub_request *sub;
	struct local *location;

	if ((ssd->channel_head[channel].subs_r_head==NULL)&&(ssd->channel_head[channel].subs_w_head==NULL))    /*这里查找读写子请求是否需要占用这个channel，不用的话才能执行GC操作*/
	{
		return 1;                                                                        /*表示当前时间这个channel没有子请求需要占用channel*/
	}
	else
	{
		if (ssd->channel_head[channel].subs_w_head!=NULL)
		{
			return 0;
		}
		else if (ssd->channel_head[channel].subs_r_head!=NULL)
		{
			sub=ssd->channel_head[channel].subs_r_head;
			while (sub!=NULL)
			{
				if (sub->current_state==SR_WAIT)                                         /*这个读请求是处于等待状态，如果它的目标die处于idle，则不能执行gc操作，返回0*/
				{
					location=find_location(ssd,sub->ppn);
					if ((ssd->channel_head[location->channel].chip_head[location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[location->channel].chip_head[location->chip].next_state==CHIP_IDLE)&&
						(ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)))
					{
						free(location);
						location=NULL;
						return 0;
					}
					free(location);
					location=NULL;
				}
				else if (sub->next_state==SR_R_DATA_TRANSFER)
				{
					location=find_location(ssd,sub->ppn);
					if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)
					{
						free(location);
						location=NULL;
						return 0;
					}
					free(location);
					location=NULL;
				}
				sub=sub->next_node;
			}
		}
		return 1;
	}
}