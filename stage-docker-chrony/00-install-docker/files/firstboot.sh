#!/bin/sh
echo "Starting firstboot.sh ..." 2>&1
# leave a tell tale
touch firstbootdone
echo "Left breadcrumb behind" 2>&1
# load all the images
for f in ./*.gz; do echo Loading $f && echo "Loading $f docker image ..." 2>&1 && docker load < $f && rm $f; done
echo "Bringing docker up ..." 2>&1
# start docker
#/usr/bin/docker compose up -d
