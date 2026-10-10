-- Personal encounter credits belong to the character lifecycle, not a bot account.
-- Deleting a character cascades in the same transaction, including offline/admin
-- deletions. Reusing its numeric GUID cannot inherit its old encounter history.
CREATE TABLE IF NOT EXISTS `character_encounter_credit` (
  `guid` INT UNSIGNED NOT NULL,
  `scope` INT UNSIGNED NOT NULL,
  `encounter` INT UNSIGNED NOT NULL,
  `recorded_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`, `scope`, `encounter`),
  CONSTRAINT `fk_character_encounter_credit_owner`
    FOREIGN KEY (`guid`) REFERENCES `characters` (`guid`) ON DELETE CASCADE
) ENGINE=InnoDB;
