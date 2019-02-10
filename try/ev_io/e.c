#include "ef_io.h"
#include <stdio.h>
#include <errno.h>

void call_back_func(int fd, int oper,  void *cb_data)
{
	char * operation = NULL;
	switch (oper) {
		case 0:
			operation = "OPEN";
			break;
		case 1:
			operation = "READ";
			break;
		case 2:
			operation = "CLOSE";
			break;

	}
	printf("File %s complete and ready for %d\n",operation,fd);
	return;
}

int main()
{
	char str[10000] = {'\0'};
	int read_fd[100] = {0};
	int write_fd[100] = {0};

	ef_init(NULL);
	ef_set_cb_func(call_back_func,NULL);
	for (int i = 0; i < 50 ; i++) {
		char file_name[200] = {'\0'};
		sprintf(file_name,"test/%d_reading_file",i);
		read_fd[i] = ef_open(file_name,O_RDONLY);
		sprintf(file_name,"output/%d_writing_file",i);
		write_fd[i] = ef_open(file_name,O_RDWR|O_CREAT|O_APPEND,0644);
	}

	for (int i = 0; i < 50; i++) {
		int ret = 0;
		while ((-1 == (ret =  ef_open_status(read_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENT WRONG"); EV_DBGP("i = %d\n",i); abort(); }
		while ((-1 == (ret =  ef_open_status(write_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENT WRONG"); EV_DBG(); abort(); }
	}

	for (int i = 0; i < 50; i++) {
		while (1) {
			int ret = 0;
			while (-1 == (ret = ef_read(read_fd[i],str,9999)) && errno == EAGAIN);
			if (-1 == ret) { perror("SOMETHING WENT WRONG"); EV_DBG(); abort(); }
			if (!ret) break;
			if (ret) ret = ef_write(write_fd[i], str, ret);
			if (-1 == ret) { perror("SOMETHING WENT WRONG");EV_DBG(); abort(); }
		}
	}

	for (int i = 0; i <50; i++) {
		ef_close_immediate(read_fd[i]);
	}

	sleep(10);

	for (int i = 0; i < 50 ; i++) {
		char file_name[200] = {'\0'};
		sprintf(file_name,"test/%d_reading_file",i);
		read_fd[i] = ef_open(file_name,O_RDONLY);
	}

	for (int i = 0; i < 50; i++) {
		int ret = 0;
		while ((-1 == (ret =  ef_open_status(read_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENT WRONG"); EV_DBGP("i = %d\n",i); abort(); }
	}

	for (int i = 0; i < 50; i++) {
		while (1) {
			int ret = 0;
			while (-1 == (ret = ef_read(read_fd[i],str,9999)) && errno == EAGAIN);
			if (-1 == ret) { perror("SOMETHING WENT WRONG"); EV_DBG(); abort(); }
			if (!ret) break;
			if (ret) ret = ef_write(write_fd[i], str, ret);
			if (-1 == ret) { perror("SOMETHING WENT WRONG");EV_DBG(); abort(); }
		}
	}

	sleep(10);

	return 0;




}
