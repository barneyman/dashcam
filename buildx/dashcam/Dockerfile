FROM debian:bullseye-slim

RUN apt update && apt install -y gpg wget

COPY sources.list /etc/apt/
COPY raspi.list /etc/apt/sources.list.d/

ARG RELEASE=bullseye

RUN sed -i "s/RELEASE/$RELEASE/g" "${ROOTFS_DIR}/etc/apt/sources.list" && \
	sed -i "s/RELEASE/$RELEASE/g" "${ROOTFS_DIR}/etc/apt/sources.list.d/raspi.list"

COPY raspberrypi.gpg.key /tmp/raspberrypi.gpg.key
RUN cat /tmp/raspberrypi.gpg.key | gpg --dearmor > /etc/apt/trusted.gpg.d/raspberrypi-archive-stable.gpg

RUN wget https://archive.raspbian.org/raspbian.public.key -O - | apt-key add -

RUN apt update && apt dist-upgrade -y

# walk thru pi-gen
RUN apt install -y raspberrypi-archive-keyring \
                libraspberrypi0 
#                raspberrypi-sys-mods \
#                netbase raspi-copies-and-fills systemd-timesyncd

CMD "/bin/bash"


