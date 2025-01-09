use mysql;
CREATE USER 'dashcam'@'%' IDENTIFIED BY 'dashcam';
ALTER USER 'dashcam'@'%' IDENTIFIED WITH caching_sha2_password BY 'dashcam';
GRANT EXECUTE ON dashcam.* TO 'dashcam'@'%';
FLUSH PRIVILEGES;