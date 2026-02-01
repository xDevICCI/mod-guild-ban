# mod-guild-ban

AzerothCore module that allows guild leaders to ban players from their guild, preventing them from rejoining.

## Features

- **Character Ban**: Ban a specific character from your guild
- **Account Ban**: Ban all characters from an account
- **Automatic Prevention**: Banned players are automatically removed when they try to join
- **Leader Notifications**: Guild leader receives notification when a banned player attempts to join
- **Configurable Permissions**: Option to allow officers to manage bans

## Requirements

- AzerothCore 3.3.5a

## Installation

1. Clone this repository into your `modules` folder:
```bash
cd /path/to/azerothcore/modules
git clone https://github.com/xDevICCI/mod-guild-ban.git
```

2. Re-run CMake and rebuild:
```bash
cd build
cmake ..
make -j$(nproc)
make install
```

3. Apply the SQL to your `characters` database:
```bash
mysql -u <user> -p <characters_db> < modules/mod-guild-ban/sql/characters/guild_bans.sql
```

4. Copy the configuration file:
```bash
cp etc/modules/mod_guild_ban.conf.dist etc/modules/mod_guild_ban.conf
```

5. Restart worldserver

## Commands

| Command | Description | Permission |
|---------|-------------|------------|
| `.gban character <player> [reason]` | Ban a character from your guild | Guild Leader |
| `.gban account <player> [reason]` | Ban entire account from your guild | Guild Leader |
| `.gban remove <player>` | Remove a ban | Guild Leader |
| `.gban list` | List all bans for your guild | Guild Leader |

## Configuration

Edit `mod_guild_ban.conf`:

```ini
# Enable/Disable the module
GuildBan.Enable = 1

# Allow officers (rank 1) to ban members
GuildBan.AllowOfficerBan = 0

# Notify guild leader when a banned player tries to join
GuildBan.NotifyOnBannedJoinAttempt = 1
```

## Database

The module creates a `guild_bans` table in the characters database:

| Column | Type | Description |
|--------|------|-------------|
| guildId | INT | Guild ID |
| guid | INT | Banned character GUID |
| accountId | INT | Banned account ID |
| bannedBy | VARCHAR(50) | Name of who issued the ban |
| banDate | INT | Unix timestamp of ban |
| unbanDate | INT | Unix timestamp for expiry (0 = permanent) |
| banReason | VARCHAR(255) | Reason for the ban |
| banType | TINYINT | 0 = Character, 1 = Account |

## Usage Examples

**Ban a player from your guild:**
```
.gban character Playername Toxic behavior
```

**Ban an entire account:**
```
.gban account Playername Cheating on multiple characters
```

**View all bans:**
```
.gban list
```

**Remove a ban:**
```
.gban remove Playername
```

## License

This module is released under the [GNU GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).
