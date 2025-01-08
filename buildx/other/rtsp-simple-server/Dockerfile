FROM bluenviron/mediamtx:latest-rpi


RUN apt update
RUN apt install -y avahi-daemon

# dashcam service
COPY bjf.service /etc/avahi/services/

# disable dbus
RUN sed -i 's/.*enable-dbus=.*/enable-dbus=no/' /etc/avahi/avahi-daemon.conf

COPY mediamtx.yml /mediamtx.yml
COPY start.sh .

ENTRYPOINT ["sh", "./start.sh"]
