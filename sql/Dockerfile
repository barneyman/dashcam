FROM mysql:8 as baseSQL
ENV MYSQL_ROOT_PASSWORD=zqzxjvbvt

# anything in /docker-entrypoint-initdb.d will re run, alpha order
ADD scripts/dashcam_a.sql /docker-entrypoint-initdb.d/a.sql

