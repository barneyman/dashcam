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
  `locked` TINYINT NOT NULL DEFAULT '0',
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
-- Placeholder table for view `dashcam`.`chapter_view`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `dashcam`.`chapter_view` (`journey` INT, `chapter` INT, `journey_start` INT, `journey_end` INT, `chapter_start` INT, `chapter_end` INT, `filepath` INT);

-- -----------------------------------------------------
-- function fn_addmilliseconds
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` FUNCTION `fn_addmilliseconds`(millis BIGINT, base_time DATETIME(3)) RETURNS datetime(3)
    NO SQL
BEGIN

	return cast( timestampadd(microsecond, millis*1000, base_time) as datetime(3));

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_admin_clean
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_admin_clean`()
BEGIN
	# SET SQL_SAFE_UPDATES=0;

	delete from dashcam.journey_chapters where id is not null;
    delete from dashcam.journeys where id is not null;

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_admin_list_journey_chapters
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_admin_list_journey_chapters`()
BEGIN

	select BIN_TO_UUID(journey) as journey,BIN_TO_UUID(journey_chapters.id) as chapter, start_time, end_time, filepath from journey_chapters
		inner join journeys on journey_chapters.journey=journeys.id
        order by journeys.start_time;

END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_admin_list_journeys
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_admin_list_journeys`()
BEGIN

	#select BIN_TO_UUID(id) as journey, start_time, end_time from journeys order by start_time asc;
    select BIN_TO_UUID(id) as journey, base_time, ifnull(end_time,
		fn_addmilliseconds((
		select end_time from journey_chapters where journey = journeys.id and end_time is not null order by start_time desc limit 1
        ), base_time)) as base_end_time from journeys order by start_time asc;

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
-- procedure sp_delete_chapter
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_delete_chapter`(in journey_id varchar(42), in chapter_id varchar(42))
BEGIN

	declare chapter_count bigint;
    
    #declare owner_journey BINARY(16);
	#select journey into owner_journey from journey_chapters where id=UUID_TO_BIN(chapter_id);
	
    delete from journey_chapters where journey_id=BIN_TO_UUID(journey) AND chapter_id=BIN_TO_UUID(id) and locked=0;
    
    select count(*) into chapter_count from journey_chapters where BIN_TO_UUID(journey)=journey_id;
    
    if chapter_count=0 then 
		delete from journeys where BIN_TO_UUID(id)=journey_id;
		end if;

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
-- procedure sp_get_chapter_list
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_get_chapter_list`(in start_time datetime(3), in end_time datetime(3))
BEGIN
	select *, cast( (TIMESTAMPDIFF(MICROSECOND, chapter_start, start_time)/1000) as SIGNED) as offset from dashcam.chapter_view
    # the required start is within the chapter boounds
    where ((chapter_start<start_time and chapter_end>start_time) or
    # or the required end is within the chapter bounds
		(chapter_start<end_time and chapter_end>end_time) or
	# the chapter bounds are within the requested range
        (chapter_start>start_time and chapter_end<end_time)) and
	# and it is completely contained in a journey
        start_time>journey_start and end_time<journey_end;
	
END$$

DELIMITER ;

-- -----------------------------------------------------
-- procedure sp_list_old_chapters
-- -----------------------------------------------------

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_list_old_chapters`(in cutoff datetime)
BEGIN
	SELECT * FROM dashcam.chapter_view
	where chapter_end < cutoff; #"2023-08-06 05:45:00";
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

-- -----------------------------------------------------
-- View `dashcam`.`chapter_view`
-- -----------------------------------------------------
DROP TABLE IF EXISTS `dashcam`.`chapter_view`;
USE `dashcam`;
CREATE  OR REPLACE ALGORITHM=UNDEFINED DEFINER=`root`@`%` SQL SECURITY DEFINER VIEW `dashcam`.`chapter_view` AS select bin_to_uuid(`dashcam`.`journeys`.`id`) AS `journey`,bin_to_uuid(`dashcam`.`journey_chapters`.`id`) AS `chapter`,`dashcam`.`journeys`.`base_time` AS `journey_start`,`FN_ADDMILLISECONDS`((select `dashcam`.`journey_chapters`.`end_time` from `dashcam`.`journey_chapters` where ((`dashcam`.`journey_chapters`.`journey` = `dashcam`.`journeys`.`id`) and (`dashcam`.`journey_chapters`.`end_time` is not null)) order by `dashcam`.`journey_chapters`.`start_time` desc limit 1),`dashcam`.`journeys`.`base_time`) AS `journey_end`,`FN_ADDMILLISECONDS`(`dashcam`.`journey_chapters`.`start_time`,`dashcam`.`journeys`.`base_time`) AS `chapter_start`,`FN_ADDMILLISECONDS`(`dashcam`.`journey_chapters`.`end_time`,`dashcam`.`journeys`.`base_time`) AS `chapter_end`,`dashcam`.`journey_chapters`.`filepath` AS `filepath` from (`dashcam`.`journeys` join `dashcam`.`journey_chapters`) where ((`dashcam`.`journeys`.`id` = `dashcam`.`journey_chapters`.`journey`) and (`dashcam`.`journey_chapters`.`end_time` is not null)) order by `FN_ADDMILLISECONDS`(`dashcam`.`journey_chapters`.`start_time`,`dashcam`.`journeys`.`base_time`);

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
