all : libevsync.a

libevsync.a : ev_mcs_lock.o ev_spin_lock.h ev_mcs_lock.h ev_spin_lock.o
	ar -rv libevsync.a ev_mcs_lock.o ev_spin_lock.o

ev_mcs_lock.o : ev_mcs_lock.c ev_mcs_lock.h
	cc -o ev_mcs_lock.o -O3 -I. -I../common -c ev_mcs_lock.c

ev_spin_lock.o : ev_spin_lock.c ev_spin_lock.h
	cc -o ev_spin_lock.o -O3 -I. -I../common -c ev_spin_lock.c

clean : 
	rm -f libevsync.a *.o
