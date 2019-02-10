#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

int main()
{
	int i = 0;
	int count = 0;
	FILE * r_fd = NULL;
	FILE * w_fd = NULL;
	void * va[2] = {NULL};
	int status = 0;
	uint64_t	t1 = 0, t2 = 0;

	char data[4097] = {};


	t1 = clock_gettime_nsec_np(CLOCK_REALTIME);
	r_fd = fopen("reading_file","r");
	//w_fd = ef_open("writing_file",O_WRONLY|O_CREAT|O_APPEND, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//w_fd = ef_open("writing_file",O_WRONLY|O_CREAT, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//w_fd = ef_open("writing_file",O_RDWR|O_CREAT|O_APPEND, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//w_fd = ef_open("writing_file",O_RDWR|O_CREAT, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	//

	w_fd = fopen("writing_file","r+");

	int accum_ret = 0;
	int ret = 0;
	int j = 0;
	printf("Read from file_for_reading[\n");fflush(stdout);
		memset(data,0,4097);
	while (1) {
		j++;
		ret = 0;
		ret = fread(data,1024,1,r_fd);
		accum_ret += ret;
		if (!ret) {
			puts(data);
			fprintf(w_fd,"%s",data);
			fflush(w_fd);
			//fcntl(fileno_unlocked(w_fd),F_FULLFSYNC);
			break;
		}
		va[0] = data;
		fwrite(data,1024,1,w_fd);
		fflush(w_fd);
		if (accum_ret >= 4096 * 256) {
			//fcntl(fileno_unlocked(w_fd),F_FULLFSYNC);
			accum_ret = 0;
		}
		//if (j==10) break;
		//printf("%s",data);fflush(stdout);
		memset(data,0,4097);
	}
	printf("\n]\n");fflush(stdout);
	t2 = clock_gettime_nsec_np(CLOCK_REALTIME);

	printf("Time (t2-t1) in microsec = [%llu]\n",(t2-t1)/1000);

	fclose(r_fd);

	printf("[%s:%d] Reached\n",__FILE__,__LINE__);fflush(stdout);
	fclose(w_fd);

	//debug_queue(sg_fd_queue,print_queue_node);

	return 0;
}
