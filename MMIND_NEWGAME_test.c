#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "readmymind_ioctl.h"

int main(int argc, char *argv[]){
	int secretNumber; 
	int status = -1;

	if(argc != 2 || atoi(argv[1]) <= 999 || atoi(argv[1]) > 9999){
		errno = EINVAL;
		perror("Secret number should be between [1000,9999]");
		
	}
	else{
		int fd = open("/dev/mastermind", O_RDWR);
		int status = ioctl(fd, MMIND_NEWGAME, argv[1]);
		if(status == -1)
			perror("Cannot open a new game.");
	}
	
	return status;
}