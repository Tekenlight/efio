all : ef_file build file 1ef_file 2ef_file

build : libevfile.a

libevfile.a : ef_file.o
	ar -rv libevfile.a ef_file.o

ef_file.o: ef_file.c ef_io.h ef_internals.h
	cc -o ef_file.o -I. -I../locks -I../concurrent-DS -I../thrpool -I../common -O3 -c -DEV_DEBUG ef_file.c

ef_file : test_ef_file.o
	cc -O3 -o ef_file test_ef_file.o -L../concurrent-DS -levds -L../common -levglobals \
		-L../thrpool -lthrpool -L../locks -levsync -L. -levfile

test_ef_file.o: test_ef_file.c ef_io.h ef_internals.h libevfile.a
	cc -o test_ef_file.o -DNEVER -DEV_DEBUG -c -I. -I../locks -I../concurrent-DS -I../thrpool -I../common  -O3 test_ef_file.c

1test_ef_file.o: 1test_ef_file.c ef_io.h ef_internals.h libevfile.a
	cc -o 1test_ef_file.o -DNEVER -DEV_DEBUG -c -I. -I../locks -I../concurrent-DS -I../thrpool -I../common  -O3 1test_ef_file.c

1ef_file : 1test_ef_file.o
	cc -O3 -o 1ef_file 1test_ef_file.o -L../concurrent-DS -levds -L../common -levglobals \
		-L../thrpool -lthrpool -L../locks -levsync -L. -levfile

2test_ef_file.o: 2test_ef_file.c ef_io.h ef_internals.h libevfile.a
	cc -o 2test_ef_file.o -DNEVER -DEV_DEBUG -c -I. -I../locks -I../concurrent-DS -I../thrpool -I../common  -O3 2test_ef_file.c

2ef_file : 2test_ef_file.o
	cc -O3 -o 2ef_file 2test_ef_file.o -L../concurrent-DS -levds -L../common -levglobals \
		-L../thrpool -lthrpool -L../locks -levsync -L. -levfile

file: file.o
	cc -o file -O3 file.o

file.0: file.c
	cc -o file.o -O3 -c file.c

clean:
	rm -f *.o ef_file ?ef_file libevfile.a

