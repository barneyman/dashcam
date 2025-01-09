# breadcrumb https://github.com/docker/buildx/issues/574
ARG BUILDFROM=mysql
FROM ${BUILDFROM}

ARG ROOTPWD
ENV MYSQL_ROOT_PASSWORD=$ROOTPWD

# these have to be added so they are a layer each
ADD scripts/aaa-create-db.sql /docker-entrypoint-initdb.d/aaa.sql
ADD scripts/aab-create-users.sql /docker-entrypoint-initdb.d/aab.sql
ADD scripts/aac-add-grabs.sql /docker-entrypoint-initdb.d/aac.sql

