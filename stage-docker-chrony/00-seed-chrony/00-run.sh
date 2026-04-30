#!/bin/bash -e

cp files/chrony.tar.gz ${ROOTFS_DIR}/tmp/
cp files/docker-compose.yml ${ROOTFS_DIR}/var/firstboot/

tree ${ROOTFS_DIR}/tmp/

on_chroot << EOF

    docker load -i /tmp/chrony.tar.gz
    rm /tmp/chrony.tar.gz

EOF