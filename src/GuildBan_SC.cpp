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

#include "GuildBan.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GuildScript.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

// Singleton implementation
GuildBanMgr* GuildBanMgr::instance()
{
    static GuildBanMgr instance;
    return &instance;
}

void GuildBanMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetOption<bool>("GuildBan.Enable", true);
    _allowOfficerBan = sConfigMgr->GetOption<bool>("GuildBan.AllowOfficerBan", false);
    _notifyOnBannedJoinAttempt = sConfigMgr->GetOption<bool>("GuildBan.NotifyOnBannedJoinAttempt", true);
}

void GuildBanMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();

    _characterBans.clear();
    _accountBans.clear();
    _banInfo.clear();

    QueryResult result = CharacterDatabase.Query("SELECT guildId, guid, accountId, banDate, unbanDate, bannedBy, banReason, banType FROM guild_bans");

    if (!result)
    {
        LOG_INFO("module", ">> Loaded 0 guild bans. Table `guild_bans` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        GuildBanInfo info;
        info.guildId    = fields[0].Get<uint32>();
        info.guid       = fields[1].Get<uint32>();
        info.accountId  = fields[2].Get<uint32>();
        info.banDate    = fields[3].Get<uint32>();
        info.unbanDate  = fields[4].Get<uint32>();
        info.bannedBy   = fields[5].Get<std::string>();
        info.banReason  = fields[6].Get<std::string>();
        info.banType    = static_cast<GuildBanType>(fields[7].Get<uint8>());

        // Check if ban has expired
        if (info.unbanDate != 0 && info.unbanDate < time(nullptr))
        {
            // Remove expired ban
            CharacterDatabase.Execute("DELETE FROM guild_bans WHERE guildId = {} AND guid = {}", info.guildId, info.guid);
            continue;
        }

        _characterBans[info.guildId].insert(info.guid);

        if (info.banType == GUILD_BAN_ACCOUNT && info.accountId > 0)
        {
            _accountBans[info.guildId].insert(info.accountId);
        }

        _banInfo[info.guildId].push_back(info);
        ++count;

    } while (result->NextRow());

    LOG_INFO("module", ">> Loaded {} guild bans in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void GuildBanMgr::SaveBanToDB(GuildBanInfo const& banInfo)
{
    CharacterDatabase.Execute(
        "REPLACE INTO guild_bans (guildId, guid, accountId, banDate, unbanDate, bannedBy, banReason, banType) "
        "VALUES ({}, {}, {}, {}, {}, '{}', '{}', {})",
        banInfo.guildId, banInfo.guid, banInfo.accountId, banInfo.banDate, banInfo.unbanDate,
        banInfo.bannedBy, banInfo.banReason, static_cast<uint8>(banInfo.banType));
}

void GuildBanMgr::RemoveBanFromDB(uint32 guildId, uint32 guid)
{
    CharacterDatabase.Execute("DELETE FROM guild_bans WHERE guildId = {} AND guid = {}", guildId, guid);
}

bool GuildBanMgr::AddBan(uint32 guildId, uint32 guid, uint32 accountId, std::string const& bannedBy,
                          std::string const& reason, uint32 duration, GuildBanType banType)
{
    GuildBanInfo info;
    info.guildId    = guildId;
    info.guid       = guid;
    info.accountId  = accountId;
    info.banDate    = time(nullptr);
    info.unbanDate  = duration > 0 ? (time(nullptr) + duration) : 0;
    info.bannedBy   = bannedBy;
    info.banReason  = reason;
    info.banType    = banType;

    _characterBans[guildId].insert(guid);

    if (banType == GUILD_BAN_ACCOUNT && accountId > 0)
    {
        _accountBans[guildId].insert(accountId);
    }

    _banInfo[guildId].push_back(info);
    SaveBanToDB(info);

    return true;
}

bool GuildBanMgr::RemoveBan(uint32 guildId, uint32 guid)
{
    auto charIt = _characterBans.find(guildId);
    if (charIt != _characterBans.end())
    {
        charIt->second.erase(guid);
    }

    auto infoIt = _banInfo.find(guildId);
    if (infoIt != _banInfo.end())
    {
        auto& bans = infoIt->second;
        for (auto it = bans.begin(); it != bans.end(); ++it)
        {
            if (it->guid == guid)
            {
                if (it->banType == GUILD_BAN_ACCOUNT && it->accountId > 0)
                {
                    auto accIt = _accountBans.find(guildId);
                    if (accIt != _accountBans.end())
                    {
                        accIt->second.erase(it->accountId);
                    }
                }
                bans.erase(it);
                break;
            }
        }
    }

    RemoveBanFromDB(guildId, guid);
    return true;
}

bool GuildBanMgr::IsCharacterBanned(uint32 guildId, uint32 guid) const
{
    auto it = _characterBans.find(guildId);
    if (it != _characterBans.end())
    {
        return it->second.find(guid) != it->second.end();
    }
    return false;
}

bool GuildBanMgr::IsAccountBanned(uint32 guildId, uint32 accountId) const
{
    auto it = _accountBans.find(guildId);
    if (it != _accountBans.end())
    {
        return it->second.find(accountId) != it->second.end();
    }
    return false;
}

bool GuildBanMgr::IsBanned(uint32 guildId, uint32 guid, uint32 accountId) const
{
    return IsCharacterBanned(guildId, guid) || IsAccountBanned(guildId, accountId);
}

std::vector<GuildBanInfo> GuildBanMgr::GetGuildBans(uint32 guildId) const
{
    auto it = _banInfo.find(guildId);
    if (it != _banInfo.end())
    {
        return it->second;
    }
    return {};
}

// Guild Script to intercept player joining
class GuildBan_GuildScript : public GuildScript
{
public:
    GuildBan_GuildScript() : GuildScript("GuildBan_GuildScript") { }

    void OnAddMember(Guild* guild, Player* player, uint8& /*plRank*/) override
    {
        if (!sGuildBanMgr->IsEnabled() || !player || !guild)
            return;

        uint32 guildId = guild->GetId();
        uint32 guid = player->GetGUID().GetCounter();
        uint32 accountId = player->GetSession()->GetAccountId();

        if (sGuildBanMgr->IsBanned(guildId, guid, accountId))
        {
            // Schedule removal for next update (can't remove during add)
            player->GetSession()->GetPlayer()->m_Events.AddEventAtOffset([guildId, playerGuid = player->GetGUID()]()
            {
                if (Player* p = ObjectAccessor::FindPlayer(playerGuid))
                {
                    if (Guild* g = sGuildMgr->GetGuildById(guildId))
                    {
                        g->DeleteMember(playerGuid, false, false, false);
                        ChatHandler(p->GetSession()).PSendSysMessage("You are banned from this guild and cannot join.");
                    }
                }
            }, 100ms);

            // Notify guild leader if configured
            if (sGuildBanMgr->NotifyOnBannedJoinAttempt())
            {
                if (Player* leader = ObjectAccessor::FindPlayer(guild->GetLeaderGUID()))
                {
                    ChatHandler(leader->GetSession()).PSendSysMessage(
                        "|cffff0000[Guild Ban]|r Banned player %s attempted to join the guild.",
                        player->GetName().c_str());
                }
            }
        }
    }
};

// World Script for loading config and data
class GuildBan_WorldScript : public WorldScript
{
public:
    GuildBan_WorldScript() : WorldScript("GuildBan_WorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sGuildBanMgr->LoadConfig();
    }

    void OnStartup() override
    {
        sGuildBanMgr->LoadFromDB();
    }
};

void AddGuildBanScripts()
{
    new GuildBan_GuildScript();
    new GuildBan_WorldScript();
}
