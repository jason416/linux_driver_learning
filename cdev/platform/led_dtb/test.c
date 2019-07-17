/*************************************************************************
*    > File Name: test.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sun 21 Apr 2019 10:06:21 AM CST
*    > License: GPL v2
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "led.h"

#define LED1_DEV_PATH "/dev/led1"
#define LED2_DEV_PATH "/dev/led2"

int main(int argc, char *argv[])
{
	int fd, ret;
	
	if(argc != 2) {
		fprintf(stderr, "prog_name <dev_inode path>, \n\texample: %s /dev/led1\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	/* open devices */
	fd = open(argv[1], O_RDWR);
	if(fd == -1)
		goto fail;

	/* turn on */
	ret = ioctl(fd, LED_ON, 0);
	if(ret == -1)
		goto fail;
	
	sleep(2);	
	/* turn off */
	ret = ioctl(fd, LED_OFF, 0);
	if(ret == -1)
		goto fail;

	sleep(2);
	
	printf("finish, fd = %d\n", fd);
	close(fd);
	exit(EXIT_SUCCESS);

fail:
	perror("ioctl");
	exit(EXIT_FAILURE);
}
