/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º ssd.c
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

 

#include "ssd.h"



/********************************************************************************************************************************
1£¬mainº¯ÊýÖÐinitiatio()º¯ÊýÓÃÀ´³õÊ¼»¯ssd,£»2£¬make_aged()º¯ÊýÊ¹SSD³ÉÎªaged£¬agedµÄssdÏàµ±ÓÚÊ¹ÓÃ¹ýÒ»¶ÎÊ±¼äµÄssd£¬ÀïÃæÓÐÊ§Ð§Ò³£¬
non_agedµÄssdÊÇÐÂµÄssd£¬ÎÞÊ§Ð§Ò³£¬Ê§Ð§Ò³µÄ±ÈÀý¿ÉÒÔÔÚ³õÊ¼»¯²ÎÊýÖÐÉèÖÃ£»3£¬pre_process_page()º¯ÊýÌáÇ°É¨Ò»±é¶ÁÇëÇó£¬°Ñ¶ÁÇëÇó
µÄlpn<--->ppnÓ³Éä¹ØÏµÊÂÏÈ½¨Á¢ºÃ£¬Ð´ÇëÇóµÄlpn<--->ppnÓ³Éä¹ØÏµÔÚÐ´µÄÊ±ºòÔÙ½¨Á¢£¬Ô¤´¦Àítrace·ÀÖ¹¶ÁÇëÇóÊÇ¶Á²»µ½Êý¾Ý£»4£¬simulate()ÊÇ
ºËÐÄ´¦Àíº¯Êý£¬traceÎÄ¼þ´Ó¶Á½øÀ´µ½´¦ÀíÍê³É¶¼ÓÉÕâ¸öº¯ÊýÀ´Íê³É£»5£¬statistic_output()º¯Êý½«ssd½á¹¹ÖÐµÄÐÅÏ¢Êä³öµ½Êä³öÎÄ¼þ£¬Êä³öµÄÊÇ
Í³¼ÆÊý¾ÝºÍÆ½¾ùÊý¾Ý£¬Êä³öÎÄ¼þ½ÏÐ¡£¬trace_outputÎÄ¼þÔòºÜ´óºÜÏêÏ¸£»6£¬free_all_node()º¯ÊýÊÍ·ÅÕû¸ömainº¯ÊýÖÐÉêÇëµÄ½Úµã
*********************************************************************************************************************************/
int  main(int argc, char const *argv[])
{
	unsigned  int i,j,k;
	struct ssd_info *ssd;
	char outputfilename[200],statisticfilename[200];
	#ifdef DEBUG
	printf("enter main\n"); 
	#endif

	ssd=(struct ssd_info*)malloc(sizeof(struct ssd_info));
	alloc_assert(ssd,"ssd");
	memset(ssd,0, sizeof(struct ssd_info));

	

	if (argc>=2)
	{
		strncpy(ssd->parameterfilename,"page.parameters",16);
		if(argc ==3){
			strcpy(ssd->parameterfilename,argv[2]);
			printf("parameterfilename:%s\n",ssd->parameterfilename);
		}else if(argc >3){
			printf("%s\n","input error" );
			return 0;
		}
		//strcpy(ssd->tracefilename,"./trace/");
		strcpy(ssd->tracefilename,argv[1]);
		printf("filename:%s\n",ssd->tracefilename);


		char delims[] = "/";
   		char *path = NULL;
   		char *tracename = NULL;
   		char p[30];
   		strcpy(p,argv[1]);
   		path = strtok( p, delims );
   		tracename = strtok( NULL, delims );
   		printf("path:%s tracename:%s\n",path,tracename );




		strcpy(ssd->outputfilename,"result/");
		strcat(ssd->outputfilename,tracename);
		strcat(ssd->outputfilename,"_out");
		//strcpy(ssd->outputfilename,outputfilename);
		printf("outputfilename:%s\n",ssd->outputfilename); 

		strcpy(ssd->statisticfilename,"result/");
		strcat(ssd->statisticfilename,tracename);
		strcat(ssd->statisticfilename,"_st");
		//strcpy(ssd->statisticfilename,statisticfilename);
		printf("statisticfilename:%s\n",ssd->statisticfilename); 

		strcpy(ssd->gapfilename,"result/");
		strcat(ssd->gapfilename,tracename);
		strcat(ssd->gapfilename,"_g");
		//strcpy(ssd->statisticfilename,statisticfilename);
		printf("gapfilename:%s\n",ssd->gapfilename); 
	}
	else
	{
		printf("%s\n", "running as default");
		strncpy(ssd->tracefilename,"test",5);
		strncpy(ssd->outputfilename,"ex.out",7);
		strncpy(ssd->statisticfilename,"statistic10.dat",16);
		strncpy(ssd->parameterfilename,"page.parameters",16);

	}
	
	ssd=initiation(ssd);

	ssd->total_gc=0;
	make_aged(ssd);

	printf("warmed gc count %d\n",ssd->total_gc);
	
	pre_process_page(ssd);

//显示不同的plane具有的剩余free page
	// for (i=0;i<ssd->parameter->channel_number;i++)
	// {
	// 	for (j=0;j<ssd->parameter->die_chip;j++)
	// 	{
	// 		for (k=0;k<ssd->parameter->plane_die;k++)
	// 		{
	// 			printf("%d,0,%d,%d:  %5d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
	// 		}
	// 	}
	// }


	fprintf(ssd->outputfile,"\t\t\t\t\t\t\t\t\tOUTPUT\n");
	fprintf(ssd->outputfile,"****************** TRACE INFO ******************\n");

	ssd=simulate(ssd);
	statistic_output(ssd);  
	free_all_node(ssd);

	printf("\n");
	printf("the simulation is completed!\n");
	
	return 1;
 	//_CrtDumpMemoryLeaks(); 
}


/******************simulate() *********************************************************************
*simulate()ÊÇºËÐÄ´¦Àíº¯Êý£¬Ö÷ÒªÊµÏÖµÄ¹¦ÄÜ°üÀ¨
*1,´ÓtraceÎÄ¼þÖÐ»ñÈ¡Ò»ÌõÇëÇó£¬¹Òµ½ssd->request
*2£¬¸ù¾ÝssdÊÇ·ñÓÐdram·Ö±ð´¦Àí¶Á³öÀ´µÄÇëÇó£¬°ÑÕâÐ©ÇëÇó´¦Àí³ÉÎª¶ÁÐ´×ÓÇëÇó£¬¹Òµ½ssd->channel»òÕßssdÉÏ
*3£¬°´ÕÕÊÂ¼þµÄÏÈºóÀ´´¦ÀíÕâÐ©¶ÁÐ´×ÓÇëÇó¡£
*4£¬Êä³öÃ¿ÌõÇëÇóµÄ×ÓÇëÇó¶¼´¦ÀíÍêºóµÄÏà¹ØÐÅÏ¢µ½outputfileÎÄ¼þÖÐ
**************************************************************************************************/
struct ssd_info *simulate(struct ssd_info *ssd)
{
	int flag=1,flag1=0;
	double output_step=0;
	unsigned int a=0,b=0;
	//errno_t err;

	printf("\n");
	printf("begin simulating.......................\n");
	printf("\n");
	printf("\n");
	printf("   ^o^    OK, please wait a moment, and enjoy music and coffee   ^o^    \n");

	ssd->tracefile = fopen(ssd->tracefilename,"r");
	if(ssd->tracefile == NULL)
	{  
		printf("the trace file can't open\n");
		return NULL;
	}

	fprintf(ssd->outputfile,"      arrive           lsn     size ope     begin time    response time    process time\n");	
	fflush(ssd->outputfile);

	while(flag!=100)      
	{
        
		flag=get_requests(ssd);

		if(flag == 1)
		{   
			
			//printf("once\n");
			if (ssd->parameter->dram_capacity!=0)
			{
				buffer_management(ssd);  
				distribute(ssd); 
			} 
			else
			{
				no_buffer_distribute(ssd);
			}		
		}

		process(ssd);
		//trace_channel_length(ssd,1);    
		trace_output(ssd);
		#ifdef DEBUG
		printf("request queue:%d\n",ssd->request_queue_length );
		#endif
		if(flag == 0 && ssd->request_queue == NULL)
			flag = 100;
	}

	fclose(ssd->tracefile);
	return ssd;
}

int read_ndata(char *buffer, int64_t *time_t, int *lsn, int *size, int *ope) //aligned trace
{
	double time_f;
	int disk;
	int lsn1;
	int size1;
    int rw;
    sscanf(buffer, "%lf  %d %d %d %d", &time_f, &disk, &lsn1, &size1, &rw);
	if(lsn1 == 0 && size1 == 0 && time_f == 0 )
	{
		
		return 0;
	}
	*ope = rw;
	*lsn = lsn1;
	*size = size1;
	//*time_t = (int64_t)(time_f * 1000000);
	*time_t = (int64_t)(time_f * 10000);
	return 1;
	
}



int read_fdata(char *buffer, int64_t *time_t, int *lsn, int *size, int *ope) //Financial
{
	int temp;
    char rw;
	double time_f;
	int lsn1;
	int size1;
    sscanf(buffer, "%d,%d,%d,%c,%lf", &temp, &lsn1, &size1, &rw, &time_f);
	if(temp == 0 && lsn1 == 0 && size1 == 0 && time_f == 0)
	{
		return 0;
	}
	if(rw == 'R' || rw == 'r')  *ope = 0;
	else *ope = 1;
	*lsn = lsn1;
	size1 = size1/512;
	*size = size1;

	if(size1%8 != 0)
		*size = (size1/8+1)*8;


	if(*size ==0)
		*size = 8;

	*time_t = (int64_t)(time_f * 1000000000);
	return 1;
	
}


int read_data(struct ssd_info *ssd,char *buffer, int64_t *time_t, int *lsn, int *size, int *ope)
{
	//read_wdata(buffer, time_t, lsn, size, ope);
	//return read_fdata(buffer, time_t, lsn, size, ope);
	//read_ddata(buffer, time_t, lsn, size, ope);
	return read_ndata(buffer, time_t, lsn, size, ope);
	
}



/********    get_request    ******************************************************
*	1.get requests that arrived already
*	2.add those request node to ssd->reuqest_queue
*	return	0: reach the end of the trace
*			-1: no request has been added
*			1: add one request to list
*SSDÄ£ÄâÆ÷ÓÐÈýÖÖÇý¶¯·½Ê½:Ê±ÖÓÇý¶¯(¾«È·£¬Ì«Âý) ÊÂ¼þÇý¶¯(±¾³ÌÐò²ÉÓÃ) traceÇý¶¯()£¬
*Á½ÖÖ·½Ê½ÍÆ½øÊÂ¼þ£ºchannel/chip×´Ì¬¸Ä±ä¡¢traceÎÄ¼þÇëÇó´ïµ½¡£
*channel/chip×´Ì¬¸Ä±äºÍtraceÎÄ¼þÇëÇóµ½´ïÊÇÉ¢²¼ÔÚÊ±¼äÖáÉÏµÄµã£¬Ã¿´Î´Óµ±Ç°×´Ì¬µ½´ï
*ÏÂÒ»¸ö×´Ì¬¶¼Òªµ½´ï×î½üµÄÒ»¸ö×´Ì¬£¬Ã¿µ½´ïÒ»¸öµãÖ´ÐÐÒ»´Îprocess
********************************************************************************/
int get_requests(struct ssd_info *ssd)  
{  
	char buffer[200];
	unsigned int lsn=0;
	int device=-1,  size=0, ope=0,large_lsn=0, i = 0,j=0;
	struct request *request1;
	int flag = 1;
	long filepoint; 
	int64_t time_t = 0;
	int64_t nearest_event_time;    
	int result=0;

	#ifdef DEBUG
	printf("enter get_requests,  current time:%lld\n",ssd->current_time);
	#endif


	filepoint = ftell(ssd->tracefile);
	fgets(buffer, 200, ssd->tracefile);

	result = read_data(ssd,buffer, &time_t, &lsn, &size, &ope);


	//printf("feop3:%ld  %ld  %ld  %ld  %d  %d \n", feof(ssd->tracefile),time_t, lsn, size, ope,ssd->request_queue_length );
    
	// if ((lsn<0)||(size<=0)||(ope<0))
	// {
	// 	printf("lsn:%d   size:%d   ope:%d \n",lsn,size,ope );
	// 	return 100;
	// }
	if ((device<0)&&(lsn<0)&&(size<0)&&(ope<0))
	{
		return 100;
	}


	nearest_event_time=find_nearest_event(ssd);

	if(feof(ssd->tracefile))
	 {
	 	ssd->current_time=nearest_event_time;
	 	request1=NULL;
	 	return 0;
	 }

	if(time_t < 0)
	{
		printf("error!\n");
		while(1){}
	}

	/******************************************************************************************************
	*ÉÏ²ãÎÄ¼þÏµÍ³·¢ËÍ¸øSSDµÄÈÎºÎ¶ÁÐ´ÃüÁî°üÀ¨Á½¸ö²¿·Ö£¨LSN£¬size£© LSNÊÇÂß¼­ÉÈÇøºÅ£¬¶ÔÓÚÎÄ¼þÏµÍ³¶øÑÔ£¬ËüËù¿´µ½µÄ´æ
	*´¢¿Õ¼äÊÇÒ»¸öÏßÐÔµÄÁ¬Ðø¿Õ¼ä¡£ÀýÈç£¬¶ÁÇëÇó£¨260£¬6£©±íÊ¾µÄÊÇÐèÒª¶ÁÈ¡´ÓÉÈÇøºÅÎª260µÄÂß¼­ÉÈÇø¿ªÊ¼£¬×Ü¹²6¸öÉÈÇø¡£
	*large_lsn: channelÏÂÃæÓÐ¶àÉÙ¸ösubpage£¬¼´¶àÉÙ¸ösector¡£overprovideÏµÊý£ºSSDÖÐ²¢²»ÊÇËùÓÐµÄ¿Õ¼ä¶¼¿ÉÒÔ¸øÓÃ»§Ê¹ÓÃ£¬
	*±ÈÈç32GµÄSSD¿ÉÄÜÓÐ10%µÄ¿Õ¼ä±£ÁôÏÂÀ´Áô×÷ËûÓÃ£¬ËùÒÔ³ËÒÔ1-provide
	***********************************************************************************************************/

	


	if (nearest_event_time==MAX_INT64)
	{
		ssd->current_time=time_t;           
		                                                  
		//if (ssd->request_queue_length>ssd->parameter->queue_length)    //Èç¹ûÇëÇó¶ÓÁÐµÄ³¤¶È³¬¹ýÁËÅäÖÃÎÄ¼þÖÐËùÉèÖÃµÄ³¤¶È                     
		//{
			//printf("error in get request , the queue length is too long\n");
		//}
	}
	else
	{   
		if(nearest_event_time<time_t)
		{
			/*******************************************************************************
			*»Ø¹ö£¬¼´Èç¹ûÃ»ÓÐ°Ñtime_t¸³¸øssd->current_time£¬ÔòtraceÎÄ¼þÒÑ¶ÁµÄÒ»Ìõ¼ÇÂ¼»Ø¹ö
			*filepoint¼ÇÂ¼ÁËÖ´ÐÐfgetsÖ®Ç°µÄÎÄ¼þÖ¸ÕëÎ»ÖÃ£¬»Ø¹öµ½ÎÄ¼þÍ·+filepoint´¦
			*int fseek(FILE *stream, long offset, int fromwhere);º¯ÊýÉèÖÃÎÄ¼þÖ¸ÕëstreamµÄÎ»ÖÃ¡£
			*Èç¹ûÖ´ÐÐ³É¹¦£¬stream½«Ö¸ÏòÒÔfromwhere£¨Æ«ÒÆÆðÊ¼Î»ÖÃ£ºÎÄ¼þÍ·0£¬µ±Ç°Î»ÖÃ1£¬ÎÄ¼þÎ²2£©Îª»ù×¼£¬
			*Æ«ÒÆoffset£¨Ö¸ÕëÆ«ÒÆÁ¿£©¸ö×Ö½ÚµÄÎ»ÖÃ¡£Èç¹ûÖ´ÐÐÊ§°Ü(±ÈÈçoffset³¬¹ýÎÄ¼þ×ÔÉí´óÐ¡)£¬Ôò²»¸Ä±ästreamÖ¸ÏòµÄÎ»ÖÃ¡£
			*ÎÄ±¾ÎÄ¼þÖ»ÄÜ²ÉÓÃÎÄ¼þÍ·0µÄ¶¨Î»·½Ê½£¬±¾³ÌÐòÖÐ´ò¿ªÎÄ¼þ·½Ê½ÊÇ"r":ÒÔÖ»¶Á·½Ê½´ò¿ªÎÄ±¾ÎÄ¼þ	
			**********************************************************************************/
			fseek(ssd->tracefile,filepoint,0); 
			if(ssd->current_time<=nearest_event_time)
				ssd->current_time=nearest_event_time;
			return -1;

		}
		else
		{
			if (ssd->request_queue_length>=ssd->parameter->queue_length)
			{
				fseek(ssd->tracefile,filepoint,0);
				ssd->current_time=nearest_event_time;
				return -1;
			} 
			else
			{
				ssd->current_time=time_t;
			}
		}
	}





	if (lsn<ssd->min_lsn) 
		ssd->min_lsn=lsn;
	if (lsn>ssd->max_lsn)
		ssd->max_lsn=lsn;
	large_lsn=(int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
	lsn = lsn%large_lsn;



	request1 = (struct request*)malloc(sizeof(struct request));
	alloc_assert(request1,"request");
	memset(request1,0, sizeof(struct request));

	request1->time = time_t;
	request1->lsn = lsn;
	request1->size = size;
	request1->operation = ope;	
	request1->begin_time = time_t;
	request1->response_time = 0;	
	request1->energy_consumption = 0;	
	request1->next_node = NULL;
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count=0;         //record the count of lsn served by buffer
	filepoint = ftell(ssd->tracefile);		// set the file point

	if(ssd->request_queue == NULL)          //The queue is empty
	{
		ssd->request_queue = request1;
		ssd->request_tail = request1;
		ssd->request_queue_length++;
	}
	else
	{			
		(ssd->request_tail)->next_node = request1;	
		ssd->request_tail = request1;			
		ssd->request_queue_length++;
	}

	if (request1->operation==1)             //¼ÆËãÆ½¾ùÇëÇó´óÐ¡ 1Îª¶Á 0ÎªÐ´
	{
		ssd->ave_read_size=(ssd->ave_read_size*ssd->read_request_count+request1->size)/(ssd->read_request_count+1);
	} 
	else
	{
		ssd->ave_write_size=(ssd->ave_write_size*ssd->write_request_count+request1->size)/(ssd->write_request_count+1);
	}


	#ifdef DEBUG
    printf("%ld new request continue to be added to SSD\n",ssd->count++);
    #endif
	// filepoint = ftell(ssd->tracefile);	
	// fgets(buffer, 200, ssd->tracefile);    //Ñ°ÕÒÏÂÒ»ÌõÇëÇóµÄµ½´ïÊ±¼ä
	// sscanf(buffer,"%lld %d %d %d %d",&time_t,&device,&lsn,&size,&ope);
	// ssd->next_request_time=time_t;
	// fseek(ssd->tracefile,filepoint,0);

	return 1;
}

/**********************************************************************************************************************************************
*Ê×ÏÈbufferÊÇ¸öÐ´buffer£¬¾ÍÊÇÎªÐ´ÇëÇó·þÎñµÄ£¬ÒòÎª¶ÁflashµÄÊ±¼ätRÎª20us£¬Ð´flashµÄÊ±¼ätprogÎª200us£¬ËùÒÔÎªÐ´·þÎñ¸üÄÜ½ÚÊ¡Ê±¼ä
*  ¶Á²Ù×÷£ºÈç¹ûÃüÖÐÁËbuffer£¬´Óbuffer¶Á£¬²»Õ¼ÓÃchannelµÄI/O×ÜÏß£¬Ã»ÓÐÃüÖÐbuffer£¬´Óflash¶Á£¬Õ¼ÓÃchannelµÄI/O×ÜÏß£¬µ«ÊÇ²»½øbufferÁË
*  Ð´²Ù×÷£ºÊ×ÏÈrequest·Ö³Ésub_request×ÓÇëÇó£¬Èç¹ûÊÇ¶¯Ì¬·ÖÅä£¬sub_request¹Òµ½ssd->sub_requestÉÏ£¬ÒòÎª²»ÖªµÀÒªÏÈ¹Òµ½ÄÄ¸öchannelµÄsub_requestÉÏ
*          Èç¹ûÊÇ¾²Ì¬·ÖÅäÔòsub_request¹Òµ½channelµÄsub_requestÁ´ÉÏ,Í¬Ê±²»¹Ü¶¯Ì¬·ÖÅä»¹ÊÇ¾²Ì¬·ÖÅäsub_request¶¼Òª¹Òµ½requestµÄsub_requestÁ´ÉÏ
*		   ÒòÎªÃ¿´¦ÀíÍêÒ»¸örequest£¬¶¼ÒªÔÚtraceoutputÎÄ¼þÖÐÊä³ö¹ØÓÚÕâ¸örequestµÄÐÅÏ¢¡£´¦ÀíÍêÒ»¸ösub_request,¾Í½«Æä´ÓchannelµÄsub_requestÁ´
*		   »òssdµÄsub_requestÁ´ÉÏÕª³ý£¬µ«ÊÇÔÚtraceoutputÎÄ¼þÊä³öÒ»ÌõºóÔÙÇå¿ÕrequestµÄsub_requestÁ´¡£
*		   sub_requestÃüÖÐbufferÔòÔÚbufferÀïÃæÐ´¾ÍÐÐÁË£¬²¢ÇÒ½«¸Ãsub_pageÌáµ½bufferÁ´Í·(LRU)£¬ÈôÃ»ÓÐÃüÖÐÇÒbufferÂú£¬ÔòÏÈ½«bufferÁ´Î²µÄsub_request
*		   Ð´Èëflash(Õâ»á²úÉúÒ»¸ösub_requestÐ´ÇëÇó£¬¹Òµ½Õâ¸öÇëÇórequestµÄsub_requestÁ´ÉÏ£¬Í¬Ê±ÊÓ¶¯Ì¬·ÖÅä»¹ÊÇ¾²Ì¬·ÖÅä¹Òµ½channel»òssdµÄ
*		   sub_requestÁ´ÉÏ),ÔÚ½«ÒªÐ´µÄsub_pageÐ´ÈëbufferÁ´Í·
***********************************************************************************************************************************************/
struct ssd_info *buffer_management(struct ssd_info *ssd)
{   
	unsigned int j,lsn,lpn,last_lpn,first_lpn,index,complete_flag=0, state,full_page;
	unsigned int flag=0,need_distb_flag,lsn_flag,flag1=1,active_region_flag=0;           
	struct request *new_request;
	struct buffer_group *buffer_node,key;
	unsigned int mask=0,offset1=0,offset2=0;

	#ifdef DEBUG
	printf("enter buffer_management,  current time:%lld\n",ssd->current_time);
	#endif
	ssd->dram->current_time=ssd->current_time;
	full_page=~(0xffffffff<<ssd->parameter->subpage_page);
	
	new_request=ssd->request_tail;
	lsn=new_request->lsn;
	lpn=new_request->lsn/ssd->parameter->subpage_page;
	last_lpn=(new_request->lsn+new_request->size-1)/ssd->parameter->subpage_page;
	first_lpn=new_request->lsn/ssd->parameter->subpage_page;

	new_request->need_distr_flag=(unsigned int*)malloc(sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
	alloc_assert(new_request->need_distr_flag,"new_request->need_distr_flag");
	memset(new_request->need_distr_flag, 0, sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
	
	if(new_request->operation==READ) 
	{		
		while(lpn<=last_lpn)      		
		{
			/************************************************************************************************
			 *need_distb_flag±íÊ¾ÊÇ·ñÐèÒªÖ´ÐÐdistributionº¯Êý£¬1±íÊ¾ÐèÒªÖ´ÐÐ£¬bufferÖÐÃ»ÓÐ£¬0±íÊ¾²»ÐèÒªÖ´ÐÐ
             *¼´1±íÊ¾ÐèÒª·Ö·¢£¬0±íÊ¾²»ÐèÒª·Ö·¢£¬¶ÔÓ¦µã³õÊ¼È«²¿¸³Îª1
			*************************************************************************************************/
			need_distb_flag=full_page;   
			key.group=lpn;
			buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);		// buffer node 

			while((buffer_node!=NULL)&&(lsn<(lpn+1)*ssd->parameter->subpage_page)&&(lsn<=(new_request->lsn+new_request->size-1)))             			
			{             	
				lsn_flag=full_page;
				mask=1 << (lsn%ssd->parameter->subpage_page);
				if(mask>31)
				{
					printf("the subpage number is larger than 32!add some cases");
					getchar(); 		   
				}
				else if((buffer_node->stored & mask)==mask)
				{
					flag=1;
					lsn_flag=lsn_flag&(~mask);
				}

				if(flag==1)				
				{	//Èç¹û¸Ãbuffer½Úµã²»ÔÚbufferµÄ¶ÓÊ×£¬ÐèÒª½«Õâ¸ö½ÚµãÌáµ½¶ÓÊ×£¬ÊµÏÖÁËLRUËã·¨£¬Õâ¸öÊÇÒ»¸öË«Ïò¶ÓÁÐ¡£		       		
					if(ssd->dram->buffer->buffer_head!=buffer_node)     
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
					ssd->dram->buffer->read_hit++;					
					new_request->complete_lsn_count++;											
				}		
				else if(flag==0)
					{
						ssd->dram->buffer->read_miss_hit++;
					}

				need_distb_flag=need_distb_flag&lsn_flag;
				
				flag=0;		
				lsn++;						
			}	
				
			index=(lpn-first_lpn)/(32/ssd->parameter->subpage_page); 			
			new_request->need_distr_flag[index]=new_request->need_distr_flag[index]|(need_distb_flag<<(((lpn-first_lpn)%(32/ssd->parameter->subpage_page))*ssd->parameter->subpage_page));	
			lpn++;
			
		}
	}  
	else if(new_request->operation==WRITE)
	{
		while(lpn<=last_lpn)           	
		{	
			need_distb_flag=full_page;
			mask=~(0xffffffff<<(ssd->parameter->subpage_page));
			state=mask;

			if(lpn==first_lpn)
			{
				offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-new_request->lsn);
				state=state&(0xffffffff<<offset1);
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(new_request->lsn+new_request->size));
				state=state&(~(0xffffffff<<offset2));
			}
			
			ssd=insert2buffer(ssd, lpn, state,NULL,new_request);
			lpn++;
		}
	}
	complete_flag = 1;
	for(j=0;j<=(last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32;j++)
	{
		if(new_request->need_distr_flag[j] != 0)
		{
			complete_flag = 0;
		}
	}

	/*************************************************************
	*Èç¹ûÇëÇóÒÑ¾­±»È«²¿ÓÉbuffer·þÎñ£¬¸ÃÇëÇó¿ÉÒÔ±»Ö±½ÓÏìÓ¦£¬Êä³ö½á¹û
	*ÕâÀï¼ÙÉèdramµÄ·þÎñÊ±¼äÎª1000ns
	**************************************************************/
	if((complete_flag == 1)&&(new_request->subs==NULL))               
	{
		new_request->begin_time=ssd->current_time;
		new_request->response_time=ssd->current_time+1000;            
	}

	return ssd;
}

/*****************************
*lpnÏòppnµÄ×ª»»
******************************/
unsigned int lpn2ppn(struct ssd_info *ssd,unsigned int lsn)
{
	int lpn, ppn;	
	struct entry *p_map = ssd->dram->map->map_entry;
#ifdef DEBUG
	printf("enter lpn2ppn,  current time:%lld\n",ssd->current_time);
#endif
	lpn = lsn/ssd->parameter->subpage_page;			//lpn
	ppn = (p_map[lpn]).pn;
	return ppn;
}

/**********************************************************************************
*¶ÁÇëÇó·ÖÅä×ÓÇëÇóº¯Êý£¬ÕâÀïÖ»´¦Àí¶ÁÇëÇó£¬Ð´ÇëÇóÒÑ¾­ÔÚbuffer_management()º¯ÊýÖÐ´¦ÀíÁË
*¸ù¾ÝÇëÇó¶ÓÁÐºÍbufferÃüÖÐµÄ¼ì²é£¬½«Ã¿¸öÇëÇó·Ö½â³É×ÓÇëÇó£¬½«×ÓÇëÇó¶ÓÁÐ¹ÒÔÚchannelÉÏ£¬
*²»Í¬µÄchannelÓÐ×Ô¼ºµÄ×ÓÇëÇó¶ÓÁÐ
**********************************************************************************/

struct ssd_info *distribute(struct ssd_info *ssd) 
{
	unsigned int start, end, first_lsn,last_lsn,lpn,flag=0,flag_attached=0,full_page;
	unsigned int j, k, sub_size;
	int i=0;
	struct request *req;
	struct sub_request *sub;
	int* complt;

	#ifdef DEBUG
	printf("enter distribute,  current time:%lld\n",ssd->current_time);
	#endif
	full_page=~(0xffffffff<<ssd->parameter->subpage_page);

	req = ssd->request_tail;
	if(req->response_time != 0){
		return ssd;
	}
	if (req->operation==WRITE)
	{
		return ssd;
	}

	if(req != NULL)
	{
		if(req->distri_flag == 0)
		{
			//Èç¹û»¹ÓÐÒ»Ð©¶ÁÇëÇóÐèÒª´¦Àí
			if(req->complete_lsn_count != ssd->request_tail->size)
			{		
				first_lsn = req->lsn;				
				last_lsn = first_lsn + req->size;
				complt = req->need_distr_flag;
				start = first_lsn - first_lsn % ssd->parameter->subpage_page;
				end = (last_lsn/ssd->parameter->subpage_page + 1) * ssd->parameter->subpage_page;
				i = (end - start)/32;	

				while(i >= 0)
				{	
					/*************************************************************************************
					*Ò»¸ö32Î»µÄÕûÐÍÊý¾ÝµÄÃ¿Ò»Î»´ú±íÒ»¸ö×ÓÒ³£¬32/ssd->parameter->subpage_page¾Í±íÊ¾ÓÐ¶àÉÙÒ³£¬
					*ÕâÀïµÄÃ¿Ò»Ò³µÄ×´Ì¬¶¼´æ·ÅÔÚÁË req->need_distr_flagÖÐ£¬Ò²¾ÍÊÇcompltÖÐ£¬Í¨¹ý±È½ÏcompltµÄ
					*Ã¿Ò»ÏîÓëfull_page£¬¾Í¿ÉÒÔÖªµÀ£¬ÕâÒ»Ò³ÊÇ·ñ´¦ÀíÍê³É¡£Èç¹ûÃ»´¦ÀíÍê³ÉÔòÍ¨¹ýcreat_sub_request
					º¯Êý´´½¨×ÓÇëÇó¡£
					*************************************************************************************/
					for(j=0; j<32/ssd->parameter->subpage_page; j++)
					{	
						k = (complt[((end-start)/32-i)] >>(ssd->parameter->subpage_page*j)) & full_page;	
						if (k !=0)
						{
							lpn = start/ssd->parameter->subpage_page+ ((end-start)/32-i)*32/ssd->parameter->subpage_page + j;
							sub_size=transfer_size(ssd,k,lpn,req);    
							if (sub_size==0) 
							{
								continue;
							}
							else
							{
								sub=creat_sub_request(ssd,lpn,sub_size,0,req,req->operation);
							}	
						}
					}
					i = i-1;
				}

			}
			else
			{
				req->begin_time=ssd->current_time;
				req->response_time=ssd->current_time+1000;   
			}

		}
	}
	return ssd;
}


/**********************************************************************
*trace_output()º¯ÊýÊÇÔÚÃ¿Ò»ÌõÇëÇóµÄËùÓÐ×ÓÇëÇó¾­¹ýprocess()º¯Êý´¦ÀíÍêºó£¬
*´òÓ¡Êä³öÏà¹ØµÄÔËÐÐ½á¹ûµ½outputfileÎÄ¼þÖÐ£¬ÕâÀïµÄ½á¹ûÖ÷ÒªÊÇÔËÐÐµÄÊ±¼ä
**********************************************************************/
void trace_output(struct ssd_info* ssd){
	int flag = 1;	
	int64_t start_time, end_time;
	struct request *req, *pre_node;
	struct sub_request *sub, *tmp;

#ifdef DEBUG
	printf("enter trace_output,  current time:%lld\n",ssd->current_time);
#endif

	pre_node=NULL;
	req = ssd->request_queue;
	start_time = 0;
	end_time = 0;

	if(req == NULL)
		return;

	while(req != NULL)	
	{
		sub = req->subs;
		flag = 1;
		start_time = 0;
		end_time = 0;
		if(req->response_time != 0)
		{
			fprintf(ssd->outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, req->response_time-req->time);
			fflush(ssd->outputfile);

			if(req->response_time-req->begin_time==0)
			{
				printf("first:the response time is 0?? \n");
				getchar();
			}

			if (req->operation==READ)
			{
				ssd->read_request_count++;
				ssd->read_avg=ssd->read_avg+(req->response_time-req->time);
			} 
			else if(req->operation==WRITE)
			{
				ssd->write_request_count++;
				ssd->write_avg=ssd->write_avg+(req->response_time-req->time);

			} else if(req->operation==DELETE){

				ssd->ave_delete_size=(ssd->ave_delete_size*ssd->delete_request_count+req->size)/(ssd->delete_request_count+1);
				ssd->delete_request_count++;
				ssd->delete_avg=ssd->delete_avg+(req->response_time-req->time);

			}else{

				printf("undefined operation\n");
			}

			if(pre_node == NULL)
			{
				if(req->next_node == NULL)
				{
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free(req);
					req = NULL;
					ssd->request_queue = NULL;
					ssd->request_tail = NULL;
					ssd->request_queue_length--;
				}
				else
				{
					ssd->request_queue = req->next_node;
					pre_node = req;
					req = req->next_node;
					free(pre_node->need_distr_flag);
					pre_node->need_distr_flag=NULL;
					free((void *)pre_node);
					pre_node = NULL;
					ssd->request_queue_length--;
				}
			}
			else
			{
				if(req->next_node == NULL)
				{
					pre_node->next_node = NULL;
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free(req);
					req = NULL;
					ssd->request_tail = pre_node;
					ssd->request_queue_length--;
				}
				else
				{
					pre_node->next_node = req->next_node;
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free((void *)req);
					req = pre_node->next_node;
					ssd->request_queue_length--;
				}
			}
		}
		else
		{
			flag=1;
			while(sub != NULL)
			{
				if(start_time == 0)
					start_time = sub->begin_time;
				if(start_time > sub->begin_time)
					start_time = sub->begin_time;
				if(end_time < sub->complete_time)
					end_time = sub->complete_time;
				if((sub->current_state == SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))	// if any sub-request is not completed, the request is not completed
				{
					sub = sub->next_subs;
				}
				else
				{
					flag=0;
					break;
				}
				
			}

			if (flag == 1)
			{		
				//fprintf(ssd->outputfile,"%10I64u %10u %6u %2u %16I64u %16I64u %10I64u\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
				fprintf(ssd->outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
				fflush(ssd->outputfile);

				if(end_time-start_time <=0)
				{
					printf("second:the response time is 0?? \n");
					getchar();
				}

				if (req->operation==READ)
				{
					ssd->read_request_count++;
					ssd->read_avg=ssd->read_avg+(end_time-req->time);
				} 
				else if(req->operation==WRITE)
				{
					ssd->write_request_count++;
					ssd->write_avg=ssd->write_avg+(end_time-req->time);

					//printf("ssd->write_avg:%lld, req->response_time-req->time:%lld \n",ssd->write_avg, req->response_time-req->time);

				} else if(req->operation==DELETE){

					ssd->ave_delete_size=(ssd->ave_delete_size*ssd->delete_request_count+req->size)/(ssd->delete_request_count+1);
					ssd->delete_request_count++;
					ssd->delete_avg=ssd->delete_avg+(end_time-req->time);

				}else{

					printf("undefined operation\n");
				}

				while(req->subs!=NULL)
				{
					tmp = req->subs;
					req->subs = tmp->next_subs;
					if (tmp->update!=NULL)
					{
						free(tmp->update->location);
						tmp->update->location=NULL;
						free(tmp->update);
						tmp->update=NULL;
					}
					free(tmp->location);
					tmp->location=NULL;
					free(tmp);
					tmp=NULL;
					
				}
				
				if(pre_node == NULL)
				{
					if(req->next_node == NULL)
					{
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = NULL;
						ssd->request_queue = NULL;
						ssd->request_tail = NULL;
						ssd->request_queue_length--;
					}
					else
					{
						ssd->request_queue = req->next_node;
						pre_node = req;
						req = req->next_node;
						free(pre_node->need_distr_flag);
						pre_node->need_distr_flag=NULL;
						free(pre_node);
						pre_node = NULL;
						ssd->request_queue_length--;
					}
				}
				else
				{
					if(req->next_node == NULL)
					{
						pre_node->next_node = NULL;
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = NULL;
						ssd->request_tail = pre_node;	
						ssd->request_queue_length--;
					}
					else
					{
						pre_node->next_node = req->next_node;
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = pre_node->next_node;
						ssd->request_queue_length--;
					}

				}
			}
			else
			{	
				pre_node = req;
				req = req->next_node;
			}
		}		
	}
}





/*******************************************************************************
*statistic_output()º¯ÊýÖ÷ÒªÊÇÊä³ö´¦ÀíÍêÒ»ÌõÇëÇóºóµÄÏà¹Ø´¦ÀíÐÅÏ¢¡£
*1£¬¼ÆËã³öÃ¿¸öplaneµÄ²Á³ý´ÎÊý¼´plane_eraseºÍ×ÜµÄ²Á³ý´ÎÊý¼´erase
*2£¬´òÓ¡min_lsn£¬max_lsn£¬read_count£¬program_countµÈÍ³¼ÆÐÅÏ¢µ½ÎÄ¼þoutputfileÖÐ¡£
*3£¬´òÓ¡ÏàÍ¬µÄÐÅÏ¢µ½ÎÄ¼þstatisticfileÖÐ
*******************************************************************************/
void statistic_output(struct ssd_info *ssd)
{
	unsigned int lpn_count=0,i,j,k,m,erase=0,plane_erase=0;
	double gc_energy=0.0;
#ifdef DEBUG
	printf("enter statistic_output,  current time:%lld\n",ssd->current_time);
#endif

	for(i=0;i<ssd->parameter->channel_number;i++)
	{
		for(j=0;j<ssd->parameter->chip_channel[0];j++)
		{
			for(k=0;k<ssd->parameter->die_chip;k++)
			{
				for(int p=0;p<ssd->parameter->plane_die;p++)
				{			
					plane_erase=0;
					for(m=0;m<ssd->parameter->block_plane;m++)
					{
						if(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count>0)
						{
							erase=erase+ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count;
							plane_erase+=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count;
						}
					}
					fprintf(ssd->outputfile,"the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",i,j,k,p,plane_erase);
					fprintf(ssd->statisticfile,"the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",i,j,k,p,plane_erase);
					fprintf(ssd->statisticfile,"the %d channel, %d chip, %d die, %d plane has : %13d write operations\n",i,j,k,p,ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].program_count);
					fprintf(ssd->statisticfile,"the %d channel, %d chip, %d die, %d plane has : %13d read operations\n",i,j,k,p,ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].read_count);
				}
			}

			fprintf(ssd->statisticfile,"the %d channel, %d chip, has : %13d read operations\n",i,j,ssd->channel_head[i].chip_head[j].read_count);
			fprintf(ssd->statisticfile,"the %d channel, %d chip, has : %13d write operations\n",i,j,ssd->channel_head[i].chip_head[j].program_count);
		}
	}

	fprintf(ssd->outputfile,"\n");
	fprintf(ssd->outputfile,"\n");
	fprintf(ssd->outputfile,"---------------------------statistic data---------------------------\n");	 
	fprintf(ssd->outputfile,"min lsn: %13d\n",ssd->min_lsn);	
	fprintf(ssd->outputfile,"max lsn: %13d\n",ssd->max_lsn);
	fprintf(ssd->outputfile,"read count: %13d\n",ssd->read_count);	  
	fprintf(ssd->outputfile,"program count: %13d",ssd->program_count);	
	fprintf(ssd->outputfile,"                        include the flash write count leaded by read requests,func get_ppn_for_pre_process()\n");
	fprintf(ssd->outputfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
	fprintf(ssd->outputfile,"erase count: %13d\n",ssd->erase_count);
	fprintf(ssd->outputfile,"direct erase count: %13d\n",ssd->direct_erase_count);
	fprintf(ssd->outputfile,"copy back count: %13d\n",ssd->copy_back_count);
	fprintf(ssd->outputfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
	fprintf(ssd->outputfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
	fprintf(ssd->outputfile,"interleave write count: %13d\n",ssd->interleave_count);
	fprintf(ssd->outputfile,"interleave read count: %13d\n",ssd->interleave_read_count);
	fprintf(ssd->outputfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
	fprintf(ssd->outputfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
	fprintf(ssd->outputfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
	fprintf(ssd->outputfile,"write flash count: %13d\n",ssd->write_flash_count);
	fprintf(ssd->outputfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
	fprintf(ssd->outputfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
	fprintf(ssd->outputfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
	fprintf(ssd->outputfile,"read request count: %13d\n",ssd->read_request_count);
	fprintf(ssd->outputfile,"write request count: %13d\n",ssd->write_request_count);
	fprintf(ssd->outputfile,"delete request count: %13d\n",ssd->delete_request_count);

	fprintf(ssd->outputfile,"read request average size: %13f\n",ssd->ave_read_size);
	fprintf(ssd->outputfile,"write request average size: %13f\n",ssd->ave_write_size);
	fprintf(ssd->outputfile,"delete request average size: %13f\n",ssd->ave_delete_size);

 	if(ssd->read_request_count ==0)
		fprintf(ssd->outputfile,"read request average response time: %lld\n",0);
 	else
		fprintf(ssd->outputfile,"read request average response time: %lld\n",ssd->read_avg/ssd->read_request_count);

	if(ssd->write_request_count ==0)
		fprintf(ssd->outputfile,"write request average response time: %lld\n",0);
	else
		fprintf(ssd->outputfile,"write request average response time: %lld\n",ssd->write_avg/ssd->write_request_count);

	//printf("write request:%lld   write request count: %lld\n", ssd->write_avg,ssd->write_request_count);
	if(ssd->delete_request_count ==0)
		fprintf(ssd->outputfile,"delete request average response time: %lld\n",0);
	else
		fprintf(ssd->outputfile,"delete request average response time: %lld\n",ssd->delete_avg/ssd->delete_request_count);



	fprintf(ssd->outputfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
	fprintf(ssd->outputfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
	fprintf(ssd->outputfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
	fprintf(ssd->outputfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
	fprintf(ssd->outputfile,"erase: %13d\n",erase);
	fflush(ssd->outputfile);

	fclose(ssd->outputfile);


	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"---------------------------statistic data---------------------------\n");	
	fprintf(ssd->statisticfile,"min lsn: %13d\n",ssd->min_lsn);	
	fprintf(ssd->statisticfile,"max lsn: %13d\n",ssd->max_lsn);
	fprintf(ssd->statisticfile,"read count: %13d\n",ssd->read_count);	  
	fprintf(ssd->statisticfile,"program count: %13d",ssd->program_count);	  
	fprintf(ssd->statisticfile,"                        include the flash write count leaded by read requests\n");
	fprintf(ssd->statisticfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
	fprintf(ssd->statisticfile,"erase count: %13d\n",ssd->erase_count);	  
	fprintf(ssd->statisticfile,"direct erase count: %13d\n",ssd->direct_erase_count);
	fprintf(ssd->statisticfile,"copy back count: %13d\n",ssd->copy_back_count);
	fprintf(ssd->statisticfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
	fprintf(ssd->statisticfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
	fprintf(ssd->statisticfile,"interleave count: %13d\n",ssd->interleave_count);
	fprintf(ssd->statisticfile,"interleave read count: %13d\n",ssd->interleave_read_count);
	fprintf(ssd->statisticfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
	fprintf(ssd->statisticfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
	fprintf(ssd->statisticfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
	fprintf(ssd->statisticfile,"write flash count: %13d\n",ssd->write_flash_count);
	fprintf(ssd->statisticfile,"waste page count: %13d\n",ssd->waste_page_count);
	fprintf(ssd->statisticfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
	fprintf(ssd->statisticfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
	fprintf(ssd->statisticfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
	fprintf(ssd->statisticfile,"read request count: %13d\n",ssd->read_request_count);
	fprintf(ssd->statisticfile,"write request count: %13d\n",ssd->write_request_count);
	fprintf(ssd->statisticfile,"delete request count: %13d\n",ssd->delete_request_count);

	fprintf(ssd->statisticfile,"read request average size: %13f\n",ssd->ave_read_size);
	fprintf(ssd->statisticfile,"write request average size: %13f\n",ssd->ave_write_size);
	fprintf(ssd->statisticfile,"delete request average size: %13f\n",ssd->ave_delete_size);

	if(ssd->read_request_count ==0)
		fprintf(ssd->statisticfile,"read request average response time: %lld\n",0);
	else
		fprintf(ssd->statisticfile,"read request average response time: %lld\n",ssd->read_avg/ssd->read_request_count);

	if(ssd->write_request_count ==0)
		fprintf(ssd->statisticfile,"write request average response time: %lld\n",0);
	else
		fprintf(ssd->statisticfile,"write request average response time: %lld\n",ssd->write_avg/ssd->write_request_count);


	if(ssd->delete_request_count ==0)
		fprintf(ssd->statisticfile,"delete request average response time: %lld\n",0);
	else
		fprintf(ssd->statisticfile,"delete request average response time: %lld\n",ssd->delete_avg/ssd->delete_request_count);


	fprintf(ssd->statisticfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
	fprintf(ssd->statisticfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
	fprintf(ssd->statisticfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
	fprintf(ssd->statisticfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
	fprintf(ssd->statisticfile,"erase: %13d\n",erase);
	fprintf(ssd->statisticfile,"copy page: %13d\n",ssd->live_copy);
	fflush(ssd->statisticfile);

	fclose(ssd->statisticfile);
}





/*********************************************************
*transfer_size()函数的作用就是计算出子请求的需要处理的size
*函数中单独处理了first_lpn，last_lpn这两个特别情况，因为这
*两种情况下很有可能不是处理一整页而是处理一页的一部分，因
*为lsn有可能不是一页的第一个子页
*********************************************************/
unsigned int transfer_size(struct ssd_info *ssd,int need_distribute,unsigned int lpn,struct request *req)
{
	unsigned int first_lpn,last_lpn,state,trans_size;
	unsigned int mask=0,offset1=0,offset2=0;

	first_lpn=req->lsn/ssd->parameter->subpage_page;
	last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;

	mask=~(0xffffffff<<(ssd->parameter->subpage_page));
	state=mask;
	if(lpn==first_lpn)
	{
		offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
		state=state&(0xffffffff<<offset1);
	}
	if(lpn==last_lpn)
	{
		offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
		state=state&(~(0xffffffff<<offset2));
	}

	trans_size=size(state&need_distribute);

	return trans_size;
}


/**********************************************************************************************************  
*int64_t find_nearest_event(struct ssd_info *ssd)       
*Ñ°ÕÒËùÓÐ×ÓÇëÇóµÄ×îÔçµ½´ïµÄÏÂ¸ö×´Ì¬Ê±¼ä,Ê×ÏÈ¿´ÇëÇóµÄÏÂÒ»¸ö×´Ì¬Ê±¼ä£¬Èç¹ûÇëÇóµÄÏÂ¸ö×´Ì¬Ê±¼äÐ¡ÓÚµÈÓÚµ±Ç°Ê±¼ä£¬
*ËµÃ÷ÇëÇó±»×èÈû£¬ÐèÒª²é¿´channel»òÕß¶ÔÓ¦dieµÄÏÂÒ»×´Ì¬Ê±¼ä¡£Int64ÊÇÓÐ·ûºÅ 64 Î»ÕûÊýÊý¾ÝÀàÐÍ£¬ÖµÀàÐÍ±íÊ¾Öµ½éÓÚ
*-2^63 ( -9,223,372,036,854,775,808)µ½2^63-1(+9,223,372,036,854,775,807 )Ö®¼äµÄÕûÊý¡£´æ´¢¿Õ¼äÕ¼ 8 ×Ö½Ú¡£
*channel,dieÊÇÊÂ¼þÏòÇ°ÍÆ½øµÄ¹Ø¼üÒòËØ£¬ÈýÖÖÇé¿ö¿ÉÒÔÊ¹ÊÂ¼þ¼ÌÐøÏòÇ°ÍÆ½ø£¬channel£¬die·Ö±ð»Øµ½idle×´Ì¬£¬dieÖÐµÄ
*¶ÁÊý¾Ý×¼±¸ºÃÁË
***********************************************************************************************************/
int64_t find_nearest_event(struct ssd_info *ssd) 
{
	unsigned int i,j;
	int64_t time=MAX_INT64;
	int64_t time1=MAX_INT64;
	int64_t time2=MAX_INT64;

	for (i=0;i<ssd->parameter->channel_number;i++)
	{
		if (ssd->channel_head[i].next_state==CHANNEL_IDLE)
			if(time1>ssd->channel_head[i].next_state_predict_time)
				if (ssd->channel_head[i].next_state_predict_time>ssd->current_time)    
					time1=ssd->channel_head[i].next_state_predict_time;
		for (j=0;j<ssd->parameter->chip_channel[i];j++)
		{
			if ((ssd->channel_head[i].chip_head[j].next_state==CHIP_IDLE)||(ssd->channel_head[i].chip_head[j].next_state==CHIP_DATA_TRANSFER))
				if(time2>ssd->channel_head[i].chip_head[j].next_state_predict_time)
					if (ssd->channel_head[i].chip_head[j].next_state_predict_time>ssd->current_time)    
						time2=ssd->channel_head[i].chip_head[j].next_state_predict_time;	
		}   
	} 

	/*****************************************************************************************************
	 *timeÎªËùÓÐ A.ÏÂÒ»×´Ì¬ÎªCHANNEL_IDLEÇÒÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼ä´óÓÚssdµ±Ç°Ê±¼äµÄCHANNELµÄÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼ä
	 *           B.ÏÂÒ»×´Ì¬ÎªCHIP_IDLEÇÒÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼ä´óÓÚssdµ±Ç°Ê±¼äµÄDIEµÄÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼ä
	 *		     C.ÏÂÒ»×´Ì¬ÎªCHIP_DATA_TRANSFERÇÒÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼ä´óÓÚssdµ±Ç°Ê±¼äµÄDIEµÄÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼ä
	 *CHIP_DATA_TRANSFER¶Á×¼±¸ºÃ×´Ì¬£¬Êý¾ÝÒÑ´Ó½éÖÊ´«µ½ÁËregister£¬ÏÂÒ»×´Ì¬ÊÇ´Óregister´«ÍùbufferÖÐµÄ×îÐ¡Öµ 
	 *×¢Òâ¿ÉÄÜ¶¼Ã»ÓÐÂú×ãÒªÇóµÄtime£¬ÕâÊ±time·µ»Ø0x7fffffffffffffff ¡£
	*****************************************************************************************************/
	time=(time1>time2)?time2:time1;
	return time;
}

/***********************************************
*free_all_node()º¯ÊýµÄ×÷ÓÃ¾ÍÊÇÊÍ·ÅËùÓÐÉêÇëµÄ½Úµã
************************************************/
void free_all_node(struct ssd_info *ssd)
{
	unsigned int i,j,k,l,n;
	struct buffer_group *pt=NULL;
	struct direct_erase * erase_node=NULL;
	for (i=0;i<ssd->parameter->channel_number;i++)
	{
		for (j=0;j<ssd->parameter->chip_channel[0];j++)
		{
			for (k=0;k<ssd->parameter->die_chip;k++)
			{
				for (l=0;l<ssd->parameter->plane_die;l++)
				{
					for (n=0;n<ssd->parameter->block_plane;n++)
					{
						free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
						ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head=NULL;
					}
					free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
					ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head=NULL;
					while(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node!=NULL)
					{
						erase_node=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
						ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=erase_node->next_node;
						free(erase_node);
						erase_node=NULL;
					}
				}
				
				free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head);
				ssd->channel_head[i].chip_head[j].die_head[k].plane_head=NULL;
			}
			free(ssd->channel_head[i].chip_head[j].die_head);
			ssd->channel_head[i].chip_head[j].die_head=NULL;
		}
		free(ssd->channel_head[i].chip_head);
		ssd->channel_head[i].chip_head=NULL;
	}
	free(ssd->channel_head);
	ssd->channel_head=NULL;

	avlTreeDestroy( ssd->dram->buffer);
	ssd->dram->buffer=NULL;
	
	free(ssd->dram->map->map_entry);
	ssd->dram->map->map_entry=NULL;
	free(ssd->dram->map);
	ssd->dram->map=NULL;
	free(ssd->dram);
	ssd->dram=NULL;
	free(ssd->parameter);
	ssd->parameter=NULL;

	free(ssd);
	ssd=NULL;
}


/*****************************************************************************
*make_aged()º¯ÊýµÄ×÷ÓÃ¾ÍËÀÄ£ÄâÕæÊµµÄÓÃ¹ýÒ»¶ÎÊ±¼äµÄssd£¬
*ÄÇÃ´Õâ¸össdµÄÏàÓ¦µÄ²ÎÊý¾ÍÒª¸Ä±ä£¬ËùÒÔÕâ¸öº¯ÊýÊµÖÊÉÏ¾ÍÊÇ¶ÔssdÖÐ¸÷¸ö²ÎÊýµÄ¸³Öµ¡£
******************************************************************************/
	struct ssd_info *make_aged(struct ssd_info *ssd)
	{
		unsigned int i,j,k,l,m,n,ppn;
		int threshould,flag=0;
		int valid_tag = 0;
		unsigned int lpn, lsn, full_page, sub_size, largest_lsn;
		int invalid_num = 0;
		int valid_num = 0;
		int threshould_block = 0;
		int rand_value;
		char buffer[20];
		long filepoint; 
		unsigned int active_blk0,active_blk1;
		unsigned int count=0;
		float rand_rat;
		int rand_mark;
		
		struct direct_erase *direct_erase_node,*new_direct_erase,*new_direct_erase2;
		struct gc_operation *gc_node, *gc_node2;
	
		full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
		srand((int)time(0));	
		largest_lsn=(unsigned int )((ssd->parameter->chip_num*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));
	
			
		if (ssd->parameter->aged==1)
		{
			//thresholdÂ±Ã­ÃŠÂ¾Ã’Â»Â¸Ã¶planeÃ–ÃÃ“ÃÂ¶Ã Ã‰Ã™Ã’Â³ÃÃ¨Ã’ÂªÃŒÃ¡Ã‡Â°Ã–ÃƒÃŽÂªÃŠÂ§ÃÂ§
			threshould=(int)(ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->aged_ratio);  
			threshould_block=(int)(ssd->parameter->page_block*ssd->parameter->aged_ratio);	
			for (i=0;i<ssd->parameter->channel_number;i++)
				for (j=0;j<ssd->parameter->chip_channel[i];j++)
					for (k=0;k<ssd->parameter->die_chip;k++)
						for (l=0;l<ssd->parameter->plane_die;l++)
						{  
							flag=0;
							rand_rat =(double)(rand()%3)/100; 
							rand_mark =(int)rand()%2;							
							if(rand_mark== 0){
								threshould=(int)(ssd->parameter->block_plane*ssd->parameter->page_block*(ssd->parameter->aged_ratio-rand_rat));
							}
							else
								threshould=(int)(ssd->parameter->block_plane*ssd->parameter->page_block*(ssd->parameter->aged_ratio+rand_rat));  
							
							for (m=0;m<ssd->parameter->block_plane;m++)
							{  
								if (flag>threshould)	//flag æ— æ•ˆä¸ªæ•°ï¼Œè‹¥å½“å‰æ— æ•ˆä¸ªæ•°å¤§äºŽç­‰äºŽé˜ˆå€¼ï¼Œåˆ™ä¸å†è¿›è¡Œwarm up
								{
									break;
								}
								for (n=0;n<ssd->parameter->page_block;n++)
								{  
									rand_value = rand()%10;
									valid_tag = ((rand_value>2)? 1 : 0);	//0 for invalid   1 for valid	70% æœ‰æ•ˆ 30%æ— æ•ˆæ•°æ®
									
									
									if((valid_tag == 0) /*&& (invalid_num< (0.2*threshould+ 1)))||(valid_num >0.8*threshould)*/)
									{
										
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state=0; 	   //Â±Ã­ÃŠÂ¾Ã„Â³Ã’Â»Ã’Â³ÃŠÂ§ÃÂ§Â£Â¬ÃÂ¬ÃŠÂ±Â±ÃªÂ¼Ã‡validÂºÃfreeÃ—Â´ÃŒÂ¬Â¶Â¼ÃŽÂª0
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state=0;		   //Â±Ã­ÃŠÂ¾Ã„Â³Ã’Â»Ã’Â³ÃŠÂ§ÃÂ§Â£Â¬ÃÂ¬ÃŠÂ±Â±ÃªÂ¼Ã‡validÂºÃfreeÃ—Â´ÃŒÂ¬Â¶Â¼ÃŽÂª0
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn=0;  //Â°Ã‘valid_state free_state lpnÂ¶Â¼Ã–ÃƒÃŽÂª0Â±Ã­ÃŠÂ¾Ã’Â³ÃŠÂ§ÃÂ§Â£Â¬Â¼Ã¬Â²Ã¢ÂµÃ„ÃŠÂ±ÂºÃ²ÃˆÃ½ÃÃ®Â¶Â¼Â¼Ã¬Â²Ã¢Â£Â¬ÂµÂ¥Â¶Ã€lpn=0Â¿Ã‰Ã’Ã”ÃŠÃ‡Ã“ÃÃÂ§Ã’Â³
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num++;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
										flag++;
										invalid_num++;
										ppn=find_ppn(ssd,i,j,k,l,m,n);
										
									}
									else
									{
										
										lpn = count;	//é¡ºåºäº§ç”Ÿè¿žç»­lpnï¼Œä»…å¯¹æœ‰æ•ˆæ•°æ®è¿›è¡Œlpné€’å¢ž
										count++;
										lsn = lpn * ssd->parameter->subpage_page;
										lsn=lsn%largest_lsn;  
										sub_size=ssd->parameter->subpage_page-(lsn%ssd->parameter->subpage_page);
									
										ppn=find_ppn(ssd,i,j,k,l,m,n);
										ssd->channel_head[i].program_count++;
										ssd->channel_head[i].chip_head[j].program_count++;		
										ssd->dram->map->map_entry[lpn].pn=ppn;	
										ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,lsn,sub_size);   //0001
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn=lpn;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state=ssd->dram->map->map_entry[lpn].state;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].make_aged=1;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
										flag++; //æ— æ•ˆä¸ªæ•°
										valid_num++;
	
									}
									if (flag>threshould)
									{
										break;
									}
								}
	
	
								if (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num==ssd->parameter->page_block)	 
								{
									new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
									alloc_assert(new_direct_erase,"new_direct_erase");
									memset(new_direct_erase,0, sizeof(struct direct_erase));
	
									new_direct_erase->block=m;
									new_direct_erase->next_node=NULL;
									direct_erase_node=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
									if (direct_erase_node==NULL)
									{
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=new_direct_erase;
									} 
									else
									{
										new_direct_erase->next_node=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
										ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=new_direct_erase;
									}
									ssd->tmp_count++;
									ssd->total_gc++;
								}
								
							} 
	
							if (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
							{
								gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
								alloc_assert(gc_node,"gc_node");
								memset(gc_node,0, sizeof(struct gc_operation));
	
								gc_node->next_node=NULL;
								gc_node->chip=j;
								gc_node->die=k;
								gc_node->plane=l;
								gc_node->block=0xffffffff;
								gc_node->page=0;
								gc_node->state=GC_WAIT;
								gc_node->priority=GC_UNINTERRUPT;
								gc_node->next_node=ssd->channel_head[i].gc_command;
								ssd->channel_head[i].gc_command=gc_node;
								ssd->gc_request++;
								ssd->total_gc++;
							}
	
							
						}	 
		}  
		else
		{
			return ssd;
		}
	
		printf("aging is completed!\n");
		return ssd;
	}

/*********************************************************************************************
*no_buffer_distribute()º¯ÊýÊÇ´¦Àíµ±ssdÃ»ÓÐdramµÄÊ±ºò£¬
*ÕâÊÇ¶ÁÐ´ÇëÇó¾Í²»±ØÔÙÐèÒªÔÚbufferÀïÃæÑ°ÕÒ£¬Ö±½ÓÀûÓÃcreat_sub_request()º¯Êý´´½¨×ÓÇëÇó£¬ÔÙ´¦Àí¡£
*********************************************************************************************/
struct ssd_info *no_buffer_distribute(struct ssd_info *ssd)
{
	unsigned int lsn,lpn,last_lpn,first_lpn,complete_flag=0, state;
	unsigned int flag=0,flag1=1,active_region_flag=0;           //to indicate the lsn is hitted or not
	struct request *req=NULL;
	struct sub_request *sub=NULL,*sub_r=NULL,*update=NULL;
	struct local *loc=NULL;
	struct channel_info *p_ch=NULL;

	
	unsigned int mask=0; 
	unsigned int offset1=0, offset2=0;
	unsigned int sub_size=0;
	unsigned int sub_state=0;

	ssd->dram->current_time=ssd->current_time;
	req=ssd->request_tail;       
	lsn=req->lsn;
	lpn=req->lsn/ssd->parameter->subpage_page;
	last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;
	first_lpn=req->lsn/ssd->parameter->subpage_page;

	if(req->operation==READ)        
	{		
		while(lpn<=last_lpn) 		
		{
			sub_state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
			sub_size=size(sub_state);
			sub=creat_sub_request(ssd,lpn,sub_size,sub_state,req,req->operation);
			lpn++;
		}

	}
	else if(req->operation==WRITE)
	{

		while(lpn<=last_lpn)     	
		{	
			mask=~(0xffffffff<<(ssd->parameter->subpage_page));
			state=mask;
			if(lpn==first_lpn)
			{
				offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
				state=state&(0xffffffff<<offset1);
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
				state=state&(~(0xffffffff<<offset2));
			}
			sub_size=size(state);
			sub=creat_sub_request(ssd,lpn,sub_size,state,req,req->operation);
			lpn++;
		}
	}

	// 删除操作

	//创建 del_request_queue 

	if( ssd->delete_count== ssd->gap){

		ssd->gap= possion();

		ssd->delete_count=0;

		create_del_request(ssd);
	
	}else{

		ssd->delete_count++;
	}

	return ssd;
}




int	create_del_request(struct ssd_info *ssd){
	// 新建删除请求操作
	struct request *request1;
	struct sub_request *sub=NULL;
	int large_lsn=(int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
	srand((int)time(0));
	int lsn = rand()%large_lsn; //随机生成要删除的目标lsn

	while(ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state ==0 ){

		lsn = rand()%large_lsn;
	}

	int size = ((rand()%48)/8+1)*8; //随机生成删除目标的size
	unsigned int  lpn = lsn/ssd->parameter->subpage_page;
	unsigned int  last_lpn = (lsn+size-1)/ssd->parameter->subpage_page;




	request1 = (struct request*)malloc(sizeof(struct request));
	alloc_assert(request1,"request");
	memset(request1,0, sizeof(struct request));

	request1->time = ssd->current_time;
	request1->lsn = lsn;
	request1->size = size;
	request1->operation = DELETE;	
	request1->begin_time = ssd->current_time;
	request1->response_time = 0;	
	request1->energy_consumption = 0;	
	request1->next_node = NULL;
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count=0;         //record the count of lsn served by buffer




	if(ssd->request_queue == NULL)          //The queue is empty
	{
		ssd->request_queue = request1;
		ssd->request_tail = request1;
		ssd->request_queue_length++;
	}
	else
	{			
		(ssd->request_tail)->next_node = request1;	
		ssd->request_tail = request1;			
		ssd->request_queue_length++;
	}


	while(lpn<=last_lpn) 		
	{
		if(ssd->dram->map->map_entry[lpn].state!=0){

			sub=create_del_sub_request(ssd, lpn, ssd->dram->map->map_entry[lpn].pn, request1);

			for (int i = 0; i < ssd->dram->map->map_entry[lpn].count/3; ++i)
			{
				if (ssd->dram->map->map_entry[sub->lpn].history_ppn[i] !=0 )
					sub=create_del_sub_request(ssd, lpn, ssd->dram->map->map_entry[lpn].history_ppn[i], request1); //删除历史副本数据
			}
		}
		lpn++;
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




