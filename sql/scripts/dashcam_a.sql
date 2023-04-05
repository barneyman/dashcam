-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------
-- -----------------------------------------------------
-- Schema dashcam
-- -----------------------------------------------------

-- -----------------------------------------------------
-- Schema dashcam
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `dashcam` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci ;
USE `dashcam` ;

-- -----------------------------------------------------
-- Table `dashcam`.`journeys`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `dashcam`.`journeys` (
  `id` BINARY(16) NOT NULL,
  `start_time` DATETIME NOT NULL,
  `end_time` DATETIME NULL DEFAULT NULL,
  `base_time` DATETIME(3) NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `TIME` (`start_time` ASC) VISIBLE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `dashcam`.`journey_chapters`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `dashcam`.`journey_chapters` (
  `id` BINARY(16) NOT NULL,
  `journey` BINARY(16) NOT NULL,
  `start_time` BIGINT NOT NULL,
  `end_time` BIGINT NULL DEFAULT NULL,
  `filepath` VARCHAR(255) NOT NULL,
  PRIMARY KEY (`id`),
  INDEX `time` (`start_time` ASC) VISIBLE,
  INDEX `journey` (`journey` ASC) VISIBLE,
  CONSTRAINT `fk_journey_chapters_1`
    FOREIGN KEY (`journey`)
    REFERENCES `dashcam`.`journeys` (`id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;

USE `dashcam` ;

-- -----------------------------------------------------
-- procedure sp_admin_list_journey_chapters
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_admin_list_journey_chapters`()
BEGIN

	select BIN_TO_UUID(id), BIN_TO_UUID(journey), start_time, end_time, filepath from journey_chapters
        order by start_time;

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_admin_list_journeys
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_admin_list_journeys`()
BEGIN

	select BIN_TO_UUID(id), start_time, end_time from journeys order by start_time asc;

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_close_journey_chapter
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_close_journey_chapter`(in journey_offset_ms BIGINT, in journey_guid varchar(42), in chapter_guid varchar(42))
BEGIN

	declare journey_bin_guid BINARY(16);
	declare chapter_bin_guid BINARY(16);
    
	SET journey_bin_guid=UUID_TO_BIN(journey_guid);
	SET chapter_bin_guid=UUID_TO_BIN(chapter_guid);    
    
    update journey_chapters set end_time=journey_offset_ms where id=chapter_bin_guid;

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_create_journey_chapter
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_create_journey_chapter`(in journey_offset_ms BIGINT, in journey_guid varchar(42), in chapter_guid varchar(42), in filepath varchar(255))
BEGIN

	declare journey_bin_guid BINARY(16);
	declare chapter_bin_guid BINARY(16);
    
	SET journey_bin_guid=UUID_TO_BIN(journey_guid);
	SET chapter_bin_guid=UUID_TO_BIN(chapter_guid);    

	insert into journey_chapters (id, journey, start_time, filepath) 
		values(chapter_bin_guid,journey_bin_guid,journey_offset_ms,filepath);
    
    
END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_end_journey
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_end_journey`(in journey_end datetime, in journey_guid varchar(42))
BEGIN
	declare journey_bin_guid BINARY(16);
	SET journey_bin_guid=UUID_TO_BIN(journey_guid);
    
	update journeys set end_time=journey_end where id=journey_bin_guid;

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_new_journey
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_new_journey`(in journey_start datetime, in journey_guid varchar(42))
BEGIN

# store the guid as a binary
declare journey_bin_guid BINARY(16);
SET journey_bin_guid=UUID_TO_BIN(journey_guid);
# and then chuck it in, simples
insert into journeys (id, start_time) values(journey_bin_guid,journey_start);

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_set_journey_basetime
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_set_journey_basetime`(in journey_base_time datetime(3), in journey_guid varchar(42))
BEGIN
	declare journey_bin_guid BINARY(16);
	SET journey_bin_guid=UUID_TO_BIN(journey_guid);
    
	update journeys set base_time=journey_base_time where id=journey_bin_guid;
END$$

DELIMITER ;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;



-- users

use mysql;
CREATE USER 'dashcam'@'%' IDENTIFIED BY 'dashcam';
ALTER USER 'dashcam'@'%' IDENTIFIED WITH mysql_native_password BY 'dashcam';
GRANT ALL PRIVILEGES ON dashcam.* TO 'dashcam'@'%';
FLUSH PRIVILEGES;