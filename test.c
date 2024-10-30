#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
	int fd; int ret;
	int count = 9; // size of frame_buf 
	char buf[1024];       
	memset(buf, 0, 1024);

	fd = open("/dev/mfrc522dev0.0", O_RDWR); 

	ret = read(fd, buf, count); 
	for(int i = 0; i < count+3; i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");

	close(fd);
	return 0;
}
