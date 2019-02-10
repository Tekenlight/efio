all : thread_pool m_thread_pool build pq_thread_pool paq_thread_pool multi_thread_pool

build : libthrpool.a

libthrpool.a : thread_pool.o
	ar -rv libthrpool.a thread_pool.o

thread_pool.o: thread_pool.c thread_pool.h
	cc -O3 -I. -I../common -I../concurrent-DS -o thread_pool.o -c thread_pool.c

thread_pool : test_thread_pool.o
	cc -L. -O3 -o thread_pool test_thread_pool.o -L../concurrent-DS -levds

pq_thread_pool : test_pqthread_pool.o
	cc -L. -O3 -o pq_thread_pool test_pqthread_pool.o -L../concurrent-DS -levds

paq_thread_pool : test_paqthread_pool.o
	cc -L. -O3 -o paq_thread_pool test_paqthread_pool.o -L../concurrent-DS -levds

multi_thread_pool : test_multithread_pool.o
	cc -L. -O3 -o multi_thread_pool test_multithread_pool.o -L../concurrent-DS -levds

m_thread_pool : thread_pool.v1.o
	cc -L. -O3 -o m_thread_pool thread_pool.v1.o -L../concurrent-DS -levds

test_thread_pool.o : thread_pool.c thread_pool.h
	cc -o test_thread_pool.o -DNEVER -DEV_DEBUG -c -I. -I../common -I../concurrent-DS -O3 thread_pool.c

test_pqthread_pool.o : pq_thread_pool.c thread_pool.h
	cc -o test_pqthread_pool.o -DNEVER -c -I. -I../common -I../concurrent-DS -O3 pq_thread_pool.c

test_paqthread_pool.o : paq_thread_pool.c thread_pool.h
	cc -o test_paqthread_pool.o -DNEVER -c -I. -I../common -I../concurrent-DS -O3 paq_thread_pool.c

test_multithread_pool.o : multi_thread_pool.c thread_pool.h
	cc -o test_multithread_pool.o -DNEVER -DEV_DEBUG -c -I. -I../common -I../concurrent-DS -O3 multi_thread_pool.c

thread_pool.v1.o : thread_pool.v1.c thread_pool.h
	cc -DNEVER -c -I. -I../common -I../concurrent-DS -O3 thread_pool.v1.c

clean:
	rm -f *.o thread_pool m_thread_pool libthrpool.a multi_thread_pool paq_thread_pool pq_thread_pool

