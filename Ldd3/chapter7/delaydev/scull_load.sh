#!/bin/bash

module="delaydev"
device="delay_dev"
mode="666"

insmod $module.ko $* || exit 1

rm -f /dev/${device}0

major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)

mknod /dev/${device}0 c $major 0

group="staff"

grep -q "^staff:" /etc/group || group="wheel"
chgrp $group /dev/${device}0
chmod $mode /dev/${device}0
