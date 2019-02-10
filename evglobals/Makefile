all : libevglobals.a

libevglobals.a : ev_globals.o
	ar -rv libevglobals.a ev_globals.o

ev_globals.o : ev_globals.c ev_globals.h
	cc -o ev_globals.o -O3 -I. -c ev_globals.c


clean:
	rm -f libevgloabls.a *.o
