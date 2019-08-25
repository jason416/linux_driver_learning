/*************************************************************************
*    > File Name: test.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sun 25 Aug 2019 10:06:21 AM CST
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
#include <string.h>
#include <time.h>

#include "pwm.h"

#define PWM_PATH        "/dev/pwm_demo"

int main(int argc, char *argv[])
{
    int i;
	int fd, ret;
    struct pwm_arg arg;
	
	/* open devices */
    printf("opening %s...\n", PWM_PATH);
	fd = open(PWM_PATH, O_RDWR);
    if(fd == -1) {
        perror("open");
		goto fail;
    }

    strcpy(arg.mode, NORMAL_MODE);
    arg.freq = 10;
    arg.duty_cyle = 20;
    ret = ioctl(fd, PWM_SET_FREQ, &arg);
    if(ret != 0) {
        perror("ioctl");
        goto fail;
    }

    printf("starting pwm timer...\n");
    ret = ioctl(fd, PWM_START);
    if(ret != 0) {
        perror("ioctl");
        goto fail;
    }

    usleep(500 * 1000);

    printf("stopping pwm timer...\n");
    ret = ioctl(fd, PWM_STOP);
    if(ret != 0) {
        perror("ioctl");
        goto fail;
    }

fail:
    close(fd);
	exit(EXIT_FAILURE);
}
