#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/kdev_t.h>

#define ROTATE_FILE_NAME "/dev/rotate_driver"

int main(int argc, char **argv)
{
	int fd;
	int data;
	
	fd = open(ROTATE_FILE_NAME, O_RDWR);
	if(fd<0)
	{
		fprintf(stderr, "Can't open %s \n", ROTATE_FILE_NAME);
		return -1;
	}
	printf("data input : ");
	scanf("%d",&data);
	
	if(data == 0)
	{	// 60 up
		write(fd,&data, sizeof(char));
	}
		
	close(fd);
	return 0;
}


