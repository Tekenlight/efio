#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
	off_t offset = 0;
	int fd = open("writing_file",O_RDWR|O_CREAT, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);
	offset = lseek(fd,0,SEEK_CUR);
	printf("Offset = [%lld]\n",offset);
	write(fd,"CHA CHI\nCHA CHI\n",16);
	close(fd);
	return 0;
}
