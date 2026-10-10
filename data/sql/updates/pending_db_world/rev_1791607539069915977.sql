-- Captain Kromcrush: keep the Ogre Suit faction until King Gordok is defeated.
-- SmartAI event conditions use event ID + 1. The post-King actions are now
-- events 11 and 12; their conditions still reference the former events 19 and 20.
UPDATE `conditions` SET `SourceGroup` = 12
WHERE `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 20 AND `SourceEntry` = 14325
AND `SourceId` = 0 AND `ElseGroup` = 0 AND `ConditionTypeOrReference` = 13
AND `ConditionTarget` = 1 AND `ConditionValue1` = 3 AND `ConditionValue2` = 3
AND `ConditionValue3` = 0 AND `NegativeCondition` = 0;

UPDATE `conditions` SET `SourceGroup` = 13
WHERE `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 21 AND `SourceEntry` = 14325
AND `SourceId` = 0 AND `ElseGroup` = 0 AND `ConditionTypeOrReference` = 13
AND `ConditionTarget` = 1 AND `ConditionValue1` = 3 AND `ConditionValue2` = 3
AND `ConditionValue3` = 0 AND `NegativeCondition` = 0;
