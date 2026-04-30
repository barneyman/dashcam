#!/bin/bash -e

cp files/rtsp.tar.gz ${ROOTFS_DIR}/tmp
cp files/docker-compose.yml ${ROOTFS_DIR}/var/firstboot/

on_chroot << EOF

    docker load -i /tmp/rtsp.tar.gz
    rm /tmp/rtsp.tar.gz

EOF