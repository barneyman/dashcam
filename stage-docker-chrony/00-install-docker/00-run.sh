#!/bin/bash -e

mkdir -p ${ROOTFS_DIR}/var/firstboot
docker pull ghcr.io/barneyman/gpsd-chrony-ntp
docker save ghcr.io/barneyman/gpsd-chrony-ntp | gzip > ${ROOTFS_DIR}/var/firstboot/1.tar.gz

#cp files/docker-compose.yml ${ROOTFS_DIR}/var/firstboot/
cp files/docker-compose-app.service ${ROOTFS_DIR}/etc/systemd/system/
cp files/firstboot.sh ${ROOTFS_DIR}/var/firstboot/


on_chroot << EOF
    # get the convenience script
    curl -fsSL https://get.docker.com -o get-docker.sh

    # and run it
    sh ./get-docker.sh

    # clean up
    rm ./get-docker.sh
    
    # add user
    usermod -aG docker ${FIRST_USER_NAME}    

    # enable my one-shot service
    systemctl enable docker-compose-app

EOF


### Doesn't work in actions
# ## point docker to my partition and pull

# # preserve any existing config
# if [ -e /etc/docker/daemon.json ]
# then
# 	mv /etc/docker/daemon.json  /etc/docker/daemon.json.old
# fi

# # point to my 'future' var/lib/docker
# cat > /etc/docker/daemon.json << EOF
# {
#   "data-root": "${ROOTFS_DIR}/var/lib/docker"
# }
# EOF

# # restart docker
# systemctl restart docker

# # pull image(s)
# docker pull ghcr.io/barneyman/gpsd-chrony-ntp

# # kill my hacked config
# rm /etc/docker/daemon.json
# # and reinstate the old config
# if [ -e /etc/docker/daemon.json.old ]
# then
#         mv /etc/docker/daemon.json.old  /etc/docker/daemon.json
# fi
# # and start the host docker again
# systemctl restart docker


# rpihaddev 3B - armv7l
# vpnhack 4 - aarch64
# rpi-pi-gen 400 - aarch64
# burner - aarch64


