/*************************************************************************
 * *    > File Name: pwm.h
 * *    > Author: jason416
 * *    > Mail: jason416@foxmail.com 
 * *    > Created Time: Sat 24 Aug 2019 10:06:21 PM CST
 * *    > License: GPL v2
 * *************************************************************************/

#ifndef __PWM_H__
#define __PWM_H__

#define PWM_MAGIC   'p'

#define PWM_START   _IO(PWM_MAGIC, 0)
#define PWM_STOP    _IO(PWM_MAGIC, 1)

struct pwm_arg {
    unsigned int freq;      /* freq in Hz*/
    unsigned int duty_cyle; /* if in inverter mode: LOW interval; otherwise, HIGH interval */
    char mode[32];          /* pass mode stirng, can only be "normal" or "reversed" */
};

#define NORMAL_MODE     "normal"
#define INVERSED_MODE   "inversed"

/*
 * In normal mode,  default HIGH; STAY LOW once timer started, till TCNTn <= TCMPn;
 * In inversed mode,default LOW; STAY HIGH once timer started, till TCNTn <= TCMPn;
 */

#define PWM_SET_FREQ    _IOR(PWM_MAGIC, 2, struct pwm_arg)
#define PWM_GET_FREQ    _IOW(PWM_MAGIC, 3, struct pwm_arg)


#endif
