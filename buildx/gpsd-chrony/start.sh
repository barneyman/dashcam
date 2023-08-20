#!/bin/sh

gpsd -D 1 -N -n /dev/ttyS0
chronyd -d -s




