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
-- Table `dashcam`.`grabs`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `dashcam`.`grabs` (
  `id` UUID NOT NULL,
  `time_in` DATETIME(3) NOT NULL,
  `time_out` DATETIME(3) NOT NULL,
  `result` INT NULL DEFAULT NULL,
  `grab_filename` VARCHAR(255) NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `TIMEIN` (`time_in` ASC) VISIBLE);



USE `dashcam` ;


-- -----------------------------------------------------
-- procedure sp_get_requested_grabs
-- -----------------------------------------------------
DROP PROCEDURE IF EXISTS `sp_get_requested_grabs`;
DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_get_requested_grabs`()
BEGIN

	select (id) as id, time_in, time_out from grabs where result is null;

END$$

DELIMITER ;


-- -----------------------------------------------------
-- procedure sp_request_grab
-- -----------------------------------------------------
DROP PROCEDURE IF EXISTS `sp_request_grab`;
DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_request_grab`(time_in datetime(3), time_out DATETIME(3),OUT returnCode int)
BEGIN


# sanity check
IF time_out > time_in THEN 

    # check that we have something in those bounds
    # ...


    # insert into the table
    INSERT INTO grabs (id, time_in, time_out) VALUES(UUID(), time_in, time_out);

    select 0 into returnCode;
ELSE


    select -1 into returnCode;

END IF;    


END$$

DELIMITER ;


-- -----------------------------------------------------
-- procedure sp_test_grabs
-- -----------------------------------------------------
DROP PROCEDURE IF EXISTS `sp_test_grabs`;
DELIMITER $$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_test_grabs`()
BEGIN

DECLARE cursor_start_time DATETIME; 
DECLARE cursor_end_time DATETIME; 
DECLARE grabStartsAt DATETIME;
DECLARE grabEndsAt DATETIME;
DECLARE journeyLength INT;
DECLARE journeyid UUID;

DECLARE loopcounter INT;
DECLARE rndStartOffsetSeconds INT;


DECLARE cursor_journey CURSOR FOR 
	SELECT DISTINCT(journey) , journey_start,journey_end FROM dashcam.chapter_view ;

OPEN cursor_journey;

FETCH cursor_journey INTO journeyid, cursor_start_time, cursor_end_time;

SET journeyLength=TIMESTAMPDIFF(SECOND,cursor_start_time, cursor_end_time);


SET loopcounter = 0;

  theLoop: LOOP

    SET loopcounter = loopcounter +1;

    set rndStartOffsetSeconds = RAND()*(journeyLength-3);

    set grabStartsAt=DATE_ADD(cursor_start_time, INTERVAL rndStartOffsetSeconds SECOND); 
    set grabEndsAt=DATE_ADD(grabStartsAt, INTERVAL 2 MINUTE);
    
    call sp_request_grab(grabStartsAt, grabEndsAt,@returncode);
    select grabStartsAt, grabEndsAt, @returncode;

    IF loopcounter =2 THEN
        LEAVE theLoop;
    END IF;
 END LOOP theLoop;



CLOSE cursor_journey;


END$$
DELIMITER ;
-- -----------------------------------------------------
-- procedure sp_update_requested_grabs
-- -----------------------------------------------------
DROP PROCEDURE IF EXISTS `sp_update_requested_grabs`;
DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_update_requested_grabs`(in grab_guid varchar(42),in grab_result smallint,in filepath varchar(255))
BEGIN
	update grabs set result=grab_result, grab_filename=filepath where id=(grab_guid);
END$$

DELIMITER ;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
