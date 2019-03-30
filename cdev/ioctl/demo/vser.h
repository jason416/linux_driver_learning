/*************************************************************************
*    > File Name: vser.h
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sat 30 Mar 2019 10:22:18 AM CST
*    > License: GPL v2
*************************************************************************/

#ifndef _VSER_H
#define _VSER_H

enum parity{
	none_parity,	/* 0 */
	odd_parity,		/* 1 */
	even_parity
};

/* option struct for virtual serial, used in ioctl */
struct option {
	unsigned int data_bit;
	unsigned int parity;    /* none, odd, even */
	unsigned int stop_bit;
};

/* macros below, for ioctl, read ioctl-decoding.txt for more msg */

#define VS_MAGIC 's'

#define VS_SET_BAUD	_IOW(VS_MAGIC, 0, unsigned int)
#define VS_GET_BAUD	_IOR(VS_MAGIC, 1, unsigned int)
#define VS_SET_FFMT	_IOW(VS_MAGIC, 2, struct option)
#define VS_GET_FFMT	_IOR(VS_MAGIC, 3, struct option)

#endif
