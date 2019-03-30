/*************************************************************************
*    > File Name: test.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sat 30 Mar 2019 11:38:35 AM CST
*    > License: GPL v2
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "vser.h"

#define VSER_DEV_PATH "/dev/vser"

int main(int agrc, char *argv[])
{
	int fd, ret;
	unsigned int baud;
	struct option opt = {8, odd_parity, 1};
	
	if(agrc < 2)
	{
		fprintf(stderr, "Usage:\n    %s <device file's path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* open devices */
	fd = open(argv[1], O_RDWR);
	if(fd == -1)
		goto fail;

	/* set baud */
	baud = 9600;
	ret = ioctl(fd, VS_SET_BAUD, baud);
	if(ret == -1)
		goto fail;

	/* get baud */
	ret = ioctl(fd, VS_GET_BAUD, baud);
	if(ret == -1)
		goto fail;

	/* set frame format */
	ret = ioctl(fd, VS_SET_FFMT, &opt);
	if(ret == -1)
		goto fail;

	/* get frame format */
	ret = ioctl(fd, VS_GET_FFMT, &opt);
	if(ret == -1)
		goto fail;

	/* print value get from driver */
	printf("baud rate: %u\n", baud);
	printf("frame format: %u%c%u\n", opt.data_bit, opt.parity == none_parity ? 'N':\
			opt.parity == odd_parity ? 'O':'E', opt.stop_bit);

	close(fd);
	exit(EXIT_SUCCESS);

fail:
	perror("ioctl");
	exit(EXIT_FAILURE);
}
