-- Guild Ban System Table
-- This table stores guild bans/blacklists

DROP TABLE IF EXISTS `guild_bans`;
CREATE TABLE `guild_bans` (
  `guildId` INT UNSIGNED NOT NULL COMMENT 'Guild ID',
  `guid` INT UNSIGNED NOT NULL COMMENT 'Banned character GUID',
  `accountId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Account ID (for account-wide bans)',
  `banDate` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Ban timestamp',
  `unbanDate` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Unban timestamp (0 = permanent)',
  `bannedBy` VARCHAR(50) NOT NULL DEFAULT '' COMMENT 'Name of player who issued the ban',
  `banReason` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'Reason for the ban',
  `banType` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = character ban, 1 = account ban',
  PRIMARY KEY (`guildId`, `guid`),
  KEY `idx_guildId` (`guildId`),
  KEY `idx_accountId` (`guildId`, `accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Guild ban/blacklist system';
