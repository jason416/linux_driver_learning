/*************************************************************************
*    > File Name: led.h
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sun 21 Apr 2019 06:19:55 AM CST
*    > License: GPL v2
*************************************************************************/

#ifndef __LED_H__
#define __LED_H__

#define LED_MAGIC 'l'

#define LED_ON  _IOW(LED_MAGIC, 0, unsigned int)
#define LED_OFF _IOW(LED_MAGIC, 1, unsigned int)


#define LED1 1
#define LED2 2

#endif
