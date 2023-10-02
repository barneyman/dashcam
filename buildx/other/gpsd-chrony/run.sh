#!/bin/sh
docker stop gpsd

docker build -t bjf-gps .

docker run --privileged --net=host -d --name gpsd \
        -p 123:123/udp                                          \
        -it                                                     \
        --cap-add SYS_NICE                                      \
        --cap-add SYS_TIME                                      \
        --cap-add SYS_RESOURCE                                  \
        --restart unless-stopped                                \
        bjf-gps

