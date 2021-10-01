CFLAGS=-std=c11 -g -static

zumacc: zumacc.c

test: zumacc
	./test.sh

clean: 
	rm -f zumacc *.o *~ tmp*

.PHONY: test clean
