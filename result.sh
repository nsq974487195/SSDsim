#!/bin/sh
echo " 分析result文件夹的结果~"


#获取所有的trace名称
get_trace_file(){

	c=0
	for file in  `ls trace`
	do
	  filelist[$c]="$file"  
	  ((c++))
	done

	return filelist
}


#运行某个trace
run_one_trace( trace ){

   	filename=trace/${trace}
	echo $filename
	./ssd $filename
}

#利用awk命令抽取结果
analysis_result( trace ){

	trace_filename = ${trace}"_st" #拼接字符串
	read_latency = `awk ' $1=="read" && $2=="request" && "$3==average" && $4=="response" {print $6}'  ${trace_filename}`
	write_latency = `awk ' $1=="write" && $2=="request" && "$3==average" && $4=="response" {print $6}'   ${trace_filename}`
	delete_latency = `awk ' $1=="delete" && $2=="request" && "$3==average" && $4=="response" {print $6}'   ${trace_filename}`
	erase =  `awk ' $1=="erase" {print $2}'   ${trace_filename}`
	copy_page = `awk ' $1=="copy" && $2=="page:" {print $3}'  ${trace_filename}`

}


#
write_result(){



}



# main procedure


