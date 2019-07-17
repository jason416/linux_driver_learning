#########################################################################
# File Name: test.sh
# Author: jason416
# mail: jason416@foxmail.com
# Created Time: 2019年07月18日 星期四 00时15分12秒
# License: GPL v2
#########################################################################
#!/bin/bash

echo "trun on led3"
echo 1 > /sys/class/leds/red\:user/brightness

sleep 2

echo "turn of led3"
echo 0 > /sys/class/leds/red\:user/brightness
