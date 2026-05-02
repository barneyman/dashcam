#!/bin/bash -e

cp files/chrony.tar.gz ${ROOTFS_DIR}/var/
cp files/docker-compose.yml ${ROOTFS_DIR}/var/firstboot/

echo "${ROOTFS_DIR}"
ls ${ROOTFS_DIR}/var/ -al

on_chroot << EOF

    ls /var/ -al

    docker load -i /var/chrony.tar.gz
    rm /var/chrony.tar.gz

EOF