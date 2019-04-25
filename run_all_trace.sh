#!/bin/sh
echo "Hello World !"

trace=""
for trace in `ls trace`
do
	filename=trace/${trace}
	echo $filename
	./ssd $filename
done
