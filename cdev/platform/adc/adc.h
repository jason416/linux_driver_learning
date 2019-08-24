/*************************************************************************
 * *    > File Name: adc.h
 * *    > Author: jason416
 * *    > Mail: jason416@foxmail.com 
 * *    > Created Time: Sat 22 Aug 2019 10:06:21 PM CST
 * *    > License: GPL v2
 * *************************************************************************/

#ifndef __ADC_H__
#define __ADC_H__

#define ADC_MAGIC   'a'

/*
 * union, send arg(channel number) to driver and get result(AIN value) from driver
 */

union channel_value {
    unsigned int channel;
    unsigned int value;
};

#define ADC_GET_VAL _IOWR(ADC_MAGIC, 0, union channel_value)

/*
 *  Exynos4412 only have 4 channels for ADC.
 */
#define AIN0    0
#define AIN1    1
#define AIN2    2
#define AIN3    3


#endif
