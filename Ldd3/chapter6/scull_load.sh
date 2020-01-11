#!/bin/sh

module="scullpipe"
device="scullpipe"
mode="666"

insmod $module.ko $* || exit 1

rm -f /dev/${device}[0-1]

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1

group="staff"

grep -q "^staff:" /etc/group || group="wheel"
chgrp $group /dev/${device}[0-1]
chmod $mode /dev/${device}[0-1]