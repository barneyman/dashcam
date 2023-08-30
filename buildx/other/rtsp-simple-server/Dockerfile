FROM bluenviron/mediamtx

COPY rtsp-simple-server.yml /mediamtx.yml

RUN apt install -y avahi-daemon

COPY bjf.service /etc/avahi/services/