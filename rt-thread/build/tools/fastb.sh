#!/bin/bash

bootmode=`awk -F= '/CONFIG_FH_FAST_BOOT/{print $2}' .config`
if [ "x$bootmode" == "xy" ];then
    # fastboot
    cp build/fastload.ld build/baseos.ld
else
    # part load
    cp build/partload.ld build/baseos.ld
fi
