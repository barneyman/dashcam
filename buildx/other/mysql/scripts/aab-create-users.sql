use mysql;
CREATE USER 'dashcam'@'%' IDENTIFIED BY 'dashcam';
ALTER USER 'dashcam'@'%' IDENTIFIED WITH mysql_native_password BY 'dashcam';
GRANT EXECUTE ON dashcam.* TO 'dashcam'@'%';
FLUSH PRIVILEGES;