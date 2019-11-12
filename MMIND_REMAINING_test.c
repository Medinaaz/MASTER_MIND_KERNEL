#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "readmymind_ioctl.h"

int main(int argc, char *argv[]){
	int status = -1;
	
	int fd = open("/dev/mastermind", O_RDWR);
	int status = ioctl(fd, MMIND_REMAINING, 0);
	
	if(status == -1)
		perror("Cannot return the number of remaining guesses.");
		
	return status;
}
