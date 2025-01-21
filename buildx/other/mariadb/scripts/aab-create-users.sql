use mysql;
CREATE USER 'dashcam'@'%' IDENTIFIED BY 'dashcam';
ALTER USER 'dashcam'@'%' IDENTIFIED BY 'dashcam';
GRANT EXECUTE ON dashcam.* TO 'dashcam'@'%';
FLUSH PRIVILEGES;