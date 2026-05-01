#!/bin/bash -e

cp files/chrony.tar.gz ${ROOTFS_DIR}/tmp/
cp files/docker-compose.yml ${ROOTFS_DIR}/var/firstboot/

ls ${ROOTFS_DIR}/tmp/ -al

on_chroot << EOF

    ls /tmp/ -al

    docker load -i /tmp/chrony.tar.gz
    rm /tmp/chrony.tar.gz

EOF