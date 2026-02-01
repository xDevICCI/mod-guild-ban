/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GUILD_BAN_H_
#define _GUILD_BAN_H_

#include "Common.h"
#include "ObjectGuid.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

enum GuildBanType
{
    GUILD_BAN_CHARACTER = 0,
    GUILD_BAN_ACCOUNT   = 1
};

struct GuildBanInfo
{
    uint32 guildId;
    uint32 guid;
    uint32 accountId;
    uint32 banDate;
    uint32 unbanDate;
    std::string bannedBy;
    std::string banReason;
    GuildBanType banType;
};

class GuildBanMgr
{
public:
    static GuildBanMgr* instance();

    void LoadFromDB();
    void SaveBanToDB(GuildBanInfo const& banInfo);
    void RemoveBanFromDB(uint32 guildId, uint32 guid);

    bool AddBan(uint32 guildId, uint32 guid, uint32 accountId, std::string const& bannedBy,
                std::string const& reason, uint32 duration, GuildBanType banType);
    bool RemoveBan(uint32 guildId, uint32 guid);

    bool IsCharacterBanned(uint32 guildId, uint32 guid) const;
    bool IsAccountBanned(uint32 guildId, uint32 accountId) const;
    bool IsBanned(uint32 guildId, uint32 guid, uint32 accountId) const;

    std::vector<GuildBanInfo> GetGuildBans(uint32 guildId) const;

    // Config
    bool IsEnabled() const { return _enabled; }
    bool AllowOfficerBan() const { return _allowOfficerBan; }
    bool NotifyOnBannedJoinAttempt() const { return _notifyOnBannedJoinAttempt; }
    void LoadConfig();

private:
    GuildBanMgr() = default;
    ~GuildBanMgr() = default;

    // guildId -> set of banned character guids
    std::unordered_map<uint32, std::unordered_set<uint32>> _characterBans;
    // guildId -> set of banned account ids
    std::unordered_map<uint32, std::unordered_set<uint32>> _accountBans;
    // Full ban info storage
    std::unordered_map<uint32, std::vector<GuildBanInfo>> _banInfo;

    bool _enabled = true;
    bool _allowOfficerBan = false;
    bool _notifyOnBannedJoinAttempt = true;
};

#define sGuildBanMgr GuildBanMgr::instance()

#endif // _GUILD_BAN_H_
