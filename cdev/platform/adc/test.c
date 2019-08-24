/*************************************************************************
*    > File Name: test.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sat 22 Aug 2019 10:06:21 PM CST
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
#include <time.h>

#include "adc.h"
#define ADC_PATH        "/dev/adc"
#define FULL_RESOLUTION ((1 << 12) - 1)
#define FULL_VOLATAGE   1.8

int main(int argc, char *argv[])
{
	int fd, ret;
    union channel_value cv;
	
	/* open devices */
    printf("opening %s\n", ADC_PATH);
	fd = open(ADC_PATH, O_RDWR);
	if(fd == -1)
		goto fail;

    printf("on itop4412, max volatage of ADC is: %.2fV.\n", FULL_VOLATAGE);
    while(1) {
        cv.channel = AIN0;
        ret = ioctl(fd, ADC_GET_VAL, &cv);
        if(ret != 0)
            break;
        printf("read volatage is: %.2fV\n", FULL_VOLATAGE * cv.value /FULL_RESOLUTION);
        sleep(1);
    }

fail:
	perror("ioctl");
    close(fd);
	exit(EXIT_FAILURE);
}
