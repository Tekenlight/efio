all: libcore_utils.dylib

clean:
	rm -f base64.o libcore_utils.dylib core.o hex.o float_limits.o

base64.o: base64.c
	cc -c base64.c -o base64.o

core.o: core.c
	cc -c core.c -o core.o

hex.o: hex.c
	cc -c hex.c -o hex.o

float_limits.o: float_limits.c
	cc -c float_limits.c -o float_limits.o

#libcore_utils.so: base64.o core.o
#	cc -bundle base64.o core.o -o libcore_utils.so

libcore_utils.dylib: base64.o core.o hex.o float_limits.o
	cc -o libcore_utils.dylib base64.o core.o hex.o float_limits.o -dynamiclib

