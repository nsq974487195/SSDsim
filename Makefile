# ssdsim linux support
all:ssd 
	
clean:
	rm -f ssd *.o *~
.PHONY: clean

ssd: ssd.o avlTree.o flash.o initialize.o pagemap.o tools.o
	cc -g -lm -o ssd ssd.o avlTree.o flash.o initialize.o pagemap.o tools.o
#	rm -f *.o
ssd.o: flash.h initialize.h pagemap.h ssd.c
	gcc -c -g ssd.c 
flash.o: pagemap.h flash.c
	gcc -c -g flash.c
initialize.o: avlTree.h pagemap.h initialize.c
	gcc -c -g initialize.c
pagemap.o: initialize.h pagemap.c
	gcc -c -g pagemap.c
avlTree.o: avlTree.c
	gcc -c -g avlTree.c
tools.o: tools.h tools.c
	gcc -c -g tools.c

