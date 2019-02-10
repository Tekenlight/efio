#include <stdio.h>
#include <string.h>
#include <ev.h>
#include <ef_io.h>
#include <time.h>

static void print_queue_node(void * data)
{
	int fd = (int)(long)data;
	EV_DBGP ("Fd in the queue = [%d]\n",fd);fflush(stdout);
	return;
}

int main()
{
	int i = 0;
	int count = 0;
	int r_fd = -1;
	int w_fd = 0;
	int status = 0;

	struct timespec tp1, tp2;

	uint64_t	t1 = 0, t2 = 0;
	uint64_t	t3 = 0, t4 = 0;
	
	long long LT = 0;

	memset(&tp1,0,sizeof(struct timespec));
	memset(&tp2,0,sizeof(struct timespec));

	char data[10000] = {};

	ef_init();

	t1 = clock_gettime_nsec_np(CLOCK_REALTIME);
	r_fd = ef_open("reading_file",O_RDONLY);
	w_fd = ef_open("writing_file",O_WRONLY|O_CREAT|O_APPEND, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//w_fd = ef_open("writing_file",O_WRONLY|O_CREAT, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//w_fd = ef_open("writing_file",O_RDWR|O_CREAT|O_APPEND, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//w_fd = ef_open("writing_file",O_RDWR|O_CREAT, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);

	//EV_DBGP("r_fd = %d w_fd = %d\n",r_fd,w_fd);

	status = ef_open_status(r_fd);
	while (-1 == status && errno == EAGAIN) {
		usleep(1);
		status = ef_open_status(r_fd);
	}
	if (status == -1) {
		EV_ABORT("Some error");
	}

	status = ef_open_status(w_fd);
	while (-1 == status && errno == EAGAIN) {
		usleep(1);
		status = ef_open_status(w_fd);
	}
	if (status == -1) {
		EV_ABORT("Some error");
	}

	off_t offset = lseek(4,0,SEEK_CUR);
	printf("offset = %lld\n",offset);

	int ret = 0;
	int j = 0;
	EV_DBGP("Read from file_for_reading[\n");fflush(stdout);
	while (1) {
		j++;
		ret = 0;
		//t3 = clock_gettime_nsec_np(CLOCK_REALTIME);
		while ((0>(ret = ef_read(r_fd,data,9999))) && errno == EAGAIN) {
			//i++;
			//usleep(10000);
		}
		//t4 = clock_gettime_nsec_np(CLOCK_REALTIME);
		//LT += (t4 - t3);
		//t4 = t3 = 0;
		//EV_DBGP("ret = [%d]\n",ret);
		if (!ret) break;
		//ef_write(w_fd,data,ret);
		ef_write(w_fd,data,ret);
		//if (j==10) break;
		//EV_DBGP("%s",data);fflush(stdout);
		memset(data,0,4097);
	}
	//EV_DBGP("[%s:%d] Reached\n",__FILE__,__LINE__);fflush(stdout);
	//EV_DBGP("[%s:%d] Reached [%d] Time spent for  read = [%lld]  \n",__FILE__,__LINE__,i,LT/1000);fflush(stdout);
	//EV_DBGP("[%s:%d] Reached [%d] Time spent within in read = [%lld]  \n",__FILE__,__LINE__,i,T/1000);fflush(stdout);

	

	EV_DBGP("\n]\n");fflush(stdout);
	t2 = clock_gettime_nsec_np(CLOCK_REALTIME);
	//usleep(1000);
	//
	//sleep(5);

	ef_close_immediate(w_fd);

	EV_DBGP("Time (t2-t1) in microsec = [%llu]\n",(t2-t1)/1000);

	ef_close_immediate(r_fd);

	//EV_DBGP("[%s:%d] Reached\n",__FILE__,__LINE__);fflush(stdout);

	//debug_queue(sg_fd_queue,print_queue_node);

	return 0;
}

