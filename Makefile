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

