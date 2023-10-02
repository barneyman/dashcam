#!/bin/sh

gpsd -G -D 1 -n /dev/ttyS0 /dev/pps0
chronyd -d -s
