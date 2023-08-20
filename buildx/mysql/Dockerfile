FROM mysql

WORKDIR /docker-entrypoint-initdb.d

ARG ROOTPWD

ENV MYSQL_ROOT_PASSWORD=$ROOTPWD

COPY scripts/* .

