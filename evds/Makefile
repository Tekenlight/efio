all : libevds.a stack queue paqueue pqueue piqueue mpiqueue spiqueue

ev_pqueue.o : ev_pqueue.c ev_pqueue.h
	cc -o ev_pqueue.o -O3 -I. -I../common -c ev_pqueue.c

ev_stack.o : ev_stack.c ev_stack.h
	cc -I. -I../locks -I../common -o ev_stack.o -O3 -c ev_stack.c

ev_queue.o : ev_queue.c ev_queue.h
	cc -DEV_DEBUG -I. -I../locks -I../common -o ev_queue.o -O3 -c ev_queue.c

ev_paqueue.o : ev_paqueue.c ev_paqueue.h
	cc -I. -I../locks -I../common -o ev_paqueue.o -O3 -c ev_paqueue.c

ev_piqueue.o : ev_piqueue.c ev_piqueue.h
	cc -I. -I../locks -I../common -o ev_piqueue.o -O3 -c ev_piqueue.c

libevds.a : ev_stack.o ev_queue.o ev_pqueue.o ev_piqueue.o ev_paqueue.o
	ar -rv libevds.a ev_stack.o ev_queue.o ev_pqueue.o ev_paqueue.o

test_ev_stack.o : ev_stack.c ev_stack.h
	cc -DNEVER -o test_ev_stack.o -I. -I../locks -I../common -O3 -c ev_stack.c

test_ev_queue.o : ev_queue.c ev_queue.h
	cc -DNEVER -o test_ev_queue.o -I. -I../locks -I../common -O3 -c ev_queue.c

test_ev_paqueue.o : ev_paqueue.c ev_paqueue.h
	cc -DNEVER -DEV_DEBUG -o test_ev_paqueue.o -I. -I../locks -I../common -O3 -c ev_paqueue.c

test_ev_pqueue.o : ev_pqueue.c ev_pqueue.h
	cc -DNEVER -DEV_DEBUG -o test_ev_pqueue.o -O3 -I. -I../locks -I../common -c ev_pqueue.c

test_ev_piqueue.o : ev_piqueue.c ev_piqueue.h
	cc -DNEVER -DEV_DEBUG -o test_ev_piqueue.o -O3 -I. -I../locks -I../common -c ev_piqueue.c

test_ev_spiqueue.o : ev_spiqueue.c ev_piqueue.h
	cc -DNEVER -DEV_DEBUG -o test_ev_spiqueue.o -O3 -I. -I../locks -I../common -c ev_spiqueue.c

test_ev_mpiqueue.o : ev_mpiqueue.c ev_piqueue.h
	cc -DNEVER -DEV_DEBUG -o test_ev_mpiqueue.o -O3 -I. -I../locks -I../common -c ev_mpiqueue.c

stack : test_ev_stack.o
	cc -o stack -O3 test_ev_stack.o

paqueue : test_ev_paqueue.o
	cc -o paqueue -O3 test_ev_paqueue.o

queue : test_ev_queue.o
	cc -o queue -O3 test_ev_queue.o

pqueue : test_ev_pqueue.o
	cc -o pqueue -O3 test_ev_pqueue.o

piqueue : test_ev_piqueue.o ev_queue.o
	cc -o piqueue -O3 test_ev_piqueue.o ev_queue.o

mpiqueue : test_ev_mpiqueue.o ev_queue.o
	cc -o mpiqueue -O3 test_ev_mpiqueue.o ev_queue.o

spiqueue : test_ev_spiqueue.o ev_queue.o
	cc -o spiqueue -O3 test_ev_spiqueue.o ev_queue.o -L../locks -levsync

clean :
	rm -f libevds.a stack queue *.o pqueue piqueue  mpiqueue spiqueue paqueue
