# ssdsim linux support
all:ssd 
	
clean:
	rm -f ssd *.o *~
.PHONY: clean

ssd: ssd.o  tools.o avlTree.o flash.o initialize.o pagemap.o
	cc -g -lm -o ssd ssd.o avlTree.o flash.o initialize.o pagemap.o tools.o 
#	rm -f *.o
ssd.o: flash.h initialize.h pagemap.h ssd.c
	gcc -c -g ssd.c 
flash.o: pagemap.h flash.c 
	gcc -c -g flash.c
initialize.o: avlTree.h pagemap.h initialize.c initialize.h
	gcc -c -g initialize.c
pagemap.o: initialize.h pagemap.c pagemap.h
	gcc -c -g pagemap.c
avlTree.o: avlTree.c  avlTree.h 
	gcc -c -g avlTree.c
tools.o: tools.h tools.c initialize.h
	gcc -c -lm -g  tools.c

run: ssd.exe
	echo $(word 1,$^)
	./ssd.exe trace/HM_0
	#./ssd.exe trace/HM_1
	#./ssd.exe trace/PRN_0
	#./ssd.exe trace/PRN_1
	#./ssd.exe trace/PROJ_1
	#./ssd.exe trace/PROJ_3
	#./ssd.exe trace/PROJ_4
	#./ssd.exe trace/PRXY_0
	#./ssd.exe trace/PSRCH
	#./ssd.exe trace/SRC1_2
	#./ssd.exe trace/STG_0
	#./ssd.exe trace/USR_0
	#./ssd.exe trace/WDEV_0

tracename=HM_0 HM_1 PRN_0 PRN_1  PROJ_1  PROJ_3 PROJ_4 PRXY_0 PSRCH SRC1_2 STG_0 USR_0  WDEV_0

run_all: 
	 for trace in `echo $(tracename)`;\
	do \
		./ssd.exe trace/$$trace;\
	done  

HM_0: ssd.exe
	./ssd.exe trace/HM_0

