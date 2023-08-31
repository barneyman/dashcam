FROM debian:bullseye-slim
RUN apt update
RUN apt install -y avahi-daemon wget
COPY bjf.service /etc/avahi/services/

ARG TARGETARCH
ARG TARGETVARIANT

WORKDIR /app

RUN wget https://github.com/bluenviron/mediamtx/releases/download/v1.0.1/mediamtx_v1.0.1_linux_$TARGETARCH$TARGETVARIANT.tar.gz && \
        tar -xvzf *.gz

COPY mediamtx.yml .

CMD ["./mediamtx"]

