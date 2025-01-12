CREATE TABLE `dashcam`.`grabs` (
  `id` BINARY(16) NOT NULL,
  `time_in` DATETIME(3) NOT NULL,
  `time_out` DATETIME(3) NOT NULL,
  `result` INT NULL DEFAULT NULL,
  `grab_filename` VARCHAR(255) NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  INDEX `TIMEIN` (`time_in` ASC) VISIBLE);

USE `dashcam`;
DROP procedure IF EXISTS `sp_get_requested_grabs`;

USE `dashcam`;
DROP procedure IF EXISTS `dashcam`.`sp_get_requested_grabs`;
;

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_get_requested_grabs`()
BEGIN

	select BIN_TO_UUID(id) as id, time_in, time_out from grabs where result is null;

END$$

DELIMITER ;

USE `dashcam`;
DROP procedure IF EXISTS `sp_update_requested_grabs`;

USE `dashcam`;
DROP procedure IF EXISTS `dashcam`.`sp_update_requested_grabs`;
;

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_update_requested_grabs`(in grab_guid varchar(42),in grab_result smallint,in filepath varchar(255))
BEGIN
	update grabs set result=grab_result, grab_filename=filepath where id=UUID_TO_BIN(grab_guid);
END$$

DELIMITER ;
;

USE `dashcam`;
DROP procedure IF EXISTS `sp_get_chapter_list`;

USE `dashcam`;
DROP procedure IF EXISTS `dashcam`.`sp_get_chapter_list`;
;

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_get_chapter_list`(in start_time datetime(3), in end_time datetime(3))
BEGIN

	select filepath, cast( (TIMESTAMPDIFF(MICROSECOND, chapter_start, start_time)/1000) as SIGNED) as offset from dashcam.chapter_view
    # the required start is within the chapter boounds
    where ((chapter_start<start_time and chapter_end>start_time) or
    # or the required end is within the chapter bounds
		(chapter_start<end_time and chapter_end>end_time) or
	# the chapter bounds are within the requested range
        (chapter_start>start_time and chapter_end<end_time)) and
	# and it's completely contained in a journey
        start_time>journey_start and end_time<journey_end;
	
END$$

DELIMITER ;
;

USE `dashcam`;
DROP procedure IF EXISTS `sp_get_chapter_list`;

USE `dashcam`;
DROP procedure IF EXISTS `dashcam`.`sp_get_chapter_list`;
;

DELIMITER $$
USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_get_chapter_list`(in start_time datetime(3), in end_time datetime(3))
BEGIN

	select filepath, cast( (TIMESTAMPDIFF(MICROSECOND, chapter_start, start_time)/1000) as SIGNED) as offsetms,
    cast( (TIMESTAMPDIFF(MICROSECOND, start_time, end_time)/1000) as SIGNED) as lengthms
     from dashcam.chapter_view
    # the required start is within the chapter boounds
    where ((chapter_start<start_time and chapter_end>start_time) or
    # or the required end is within the chapter bounds
		(chapter_start<end_time and chapter_end>end_time) or
	# the chapter bounds are within the requested range
        (chapter_start>start_time and chapter_end<end_time)) and
	# and it's completely contained in a journey
        start_time>journey_start and end_time<journey_end;
	
END$$




DELIMITER ;
;

USE `dashcam`;
DROP procedure IF EXISTS `sp_request_grab`;

DELIMITER $$

USE `dashcam`$$
CREATE DEFINER=`root`@`%` PROCEDURE `sp_request_grab`(time_in datetime(3), time_out DATETIME(3))
BEGIN

# check that we have something in those bounds
# ...


# insert into the table
INSERT INTO grabs (id, time_in, time_out) VALUES(UUID_TO_BIN(UUID()), time_in, time_out);

END$$


DELIMITER ;
;

