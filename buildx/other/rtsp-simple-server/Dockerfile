FROM balenalib/raspberry-pi:bullseye-run-20230712
RUN apt update
RUN apt install -y avahi-daemon wget libcamera0 libfreetype6 
COPY bjf.service /etc/avahi/services/

RUN update-rc.d avahi-daemon defaults && update-rc.d avahi-daemon defaults

ARG TARGETARCH
ARG TARGETVARIANT

WORKDIR /app

RUN wget https://github.com/bluenviron/mediamtx/releases/download/v1.0.1/mediamtx_v1.0.1_linux_$TARGETARCH$TARGETVARIANT.tar.gz && \
        tar -xvzf *.gz

COPY mediamtx.yml .

CMD ["./mediamtx"]

