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
#include "CharacterCache.h"
#include "CommandScript.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "Timer.h"

using namespace Acore::ChatCommands;

class GuildBan_CommandScript : public CommandScript
{
public:
    GuildBan_CommandScript() : CommandScript("GuildBan_CommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable gbanCommandTable =
        {
            { "character",  HandleGbanCharacterCommand,  SEC_PLAYER,     Console::No },
            { "account",    HandleGbanAccountCommand,    SEC_PLAYER,     Console::No },
            { "remove",     HandleGbanRemoveCommand,     SEC_PLAYER,     Console::No },
            { "list",       HandleGbanListCommand,       SEC_PLAYER,     Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "gban", gbanCommandTable },
        };

        return commandTable;
    }

    static bool CanBan(ChatHandler* handler, Guild* guild, Player* admin)
    {
        if (!sGuildBanMgr->IsEnabled())
        {
            handler->SendErrorMessage("Guild ban system is disabled.");
            return false;
        }

        if (!admin->GetGuild() || admin->GetGuild()->GetId() != guild->GetId())
        {
            handler->SendErrorMessage("You must be in a guild to use this command.");
            return false;
        }

        bool isLeader = guild->GetLeaderGUID() == admin->GetGUID();
        bool isOfficer = false;

        if (!isLeader)
        {
            // Check if player has officer rank (rank 1 or lower numbers are higher)
            if (Guild::Member const* member = guild->GetMember(admin->GetGUID()))
            {
                isOfficer = member->GetRankId() <= 1; // Rank 0 = leader, Rank 1 = officer typically
            }
        }

        if (!isLeader && !(sGuildBanMgr->AllowOfficerBan() && isOfficer))
        {
            handler->SendErrorMessage("Only the guild leader can ban members.");
            return false;
        }

        return true;
    }

    static bool HandleGbanCharacterCommand(ChatHandler* handler, Optional<PlayerIdentifier> target, Optional<std::string> reason)
    {
        Player* admin = handler->GetSession()->GetPlayer();
        if (!admin)
            return false;

        Guild* guild = admin->GetGuild();
        if (!guild)
        {
            handler->SendErrorMessage("You are not in a guild.");
            return false;
        }

        if (!CanBan(handler, guild, admin))
            return false;

        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
        {
            handler->SendErrorMessage("No player specified.");
            return false;
        }

        ObjectGuid targetGuid = target->GetGUID();
        std::string targetName = target->GetName();
        uint32 targetAccountId = 0;

        // Get account ID
        if (target->IsConnected())
        {
            targetAccountId = target->GetConnectedPlayer()->GetSession()->GetAccountId();
        }
        else
        {
            targetAccountId = sCharacterCache->GetCharacterAccountIdByGuid(targetGuid);
        }

        // Check if trying to ban self
        if (targetGuid == admin->GetGUID())
        {
            handler->SendErrorMessage("You cannot ban yourself.");
            return false;
        }

        // Check if trying to ban the leader
        if (targetGuid == guild->GetLeaderGUID())
        {
            handler->SendErrorMessage("You cannot ban the guild leader.");
            return false;
        }

        std::string banReason = reason.value_or("No reason specified");

        // Add to ban list
        sGuildBanMgr->AddBan(guild->GetId(), targetGuid.GetCounter(), targetAccountId,
                            admin->GetName(), banReason, 0, GUILD_BAN_CHARACTER);

        // Kick from guild if member
        if (guild->GetMember(targetGuid))
        {
            guild->DeleteMember(targetGuid, false, true, false);
        }

        handler->PSendSysMessage("|cff00ff00[Guild Ban]|r Character %s has been banned from the guild. Reason: %s",
                                 targetName.c_str(), banReason.c_str());

        // Notify the banned player if online
        if (target->IsConnected())
        {
            ChatHandler(target->GetConnectedPlayer()->GetSession()).PSendSysMessage(
                "|cffff0000[Guild Ban]|r You have been banned from <%s>. Reason: %s",
                guild->GetName().c_str(), banReason.c_str());
        }

        return true;
    }

    static bool HandleGbanAccountCommand(ChatHandler* handler, Optional<PlayerIdentifier> target, Optional<std::string> reason)
    {
        Player* admin = handler->GetSession()->GetPlayer();
        if (!admin)
            return false;

        Guild* guild = admin->GetGuild();
        if (!guild)
        {
            handler->SendErrorMessage("You are not in a guild.");
            return false;
        }

        if (!CanBan(handler, guild, admin))
            return false;

        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
        {
            handler->SendErrorMessage("No player specified.");
            return false;
        }

        ObjectGuid targetGuid = target->GetGUID();
        std::string targetName = target->GetName();
        uint32 targetAccountId = 0;

        // Get account ID
        if (target->IsConnected())
        {
            targetAccountId = target->GetConnectedPlayer()->GetSession()->GetAccountId();
        }
        else
        {
            targetAccountId = sCharacterCache->GetCharacterAccountIdByGuid(targetGuid);
        }

        if (targetAccountId == 0)
        {
            handler->SendErrorMessage("Could not find account for player.");
            return false;
        }

        // Check if trying to ban self
        if (targetGuid == admin->GetGUID())
        {
            handler->SendErrorMessage("You cannot ban yourself.");
            return false;
        }

        // Check if trying to ban the leader
        if (targetGuid == guild->GetLeaderGUID())
        {
            handler->SendErrorMessage("You cannot ban the guild leader.");
            return false;
        }

        std::string banReason = reason.value_or("No reason specified");

        // Add to ban list (account-wide)
        sGuildBanMgr->AddBan(guild->GetId(), targetGuid.GetCounter(), targetAccountId,
                            admin->GetName(), banReason, 0, GUILD_BAN_ACCOUNT);

        // Kick from guild if member
        if (guild->GetMember(targetGuid))
        {
            guild->DeleteMember(targetGuid, false, true, false);
        }

        // Also kick all other characters from the same account
        QueryResult result = CharacterDatabase.Query(
            "SELECT guid FROM characters WHERE account = {} AND guid != {}",
            targetAccountId, targetGuid.GetCounter());

        if (result)
        {
            do
            {
                uint32 charGuid = result->Fetch()[0].Get<uint32>();
                ObjectGuid altGuid = ObjectGuid::Create<HighGuid::Player>(charGuid);

                if (guild->GetMember(altGuid))
                {
                    guild->DeleteMember(altGuid, false, true, false);
                }
            } while (result->NextRow());
        }

        handler->PSendSysMessage("|cff00ff00[Guild Ban]|r Account of %s has been banned from the guild (all characters). Reason: %s",
                                 targetName.c_str(), banReason.c_str());

        // Notify the banned player if online
        if (target->IsConnected())
        {
            ChatHandler(target->GetConnectedPlayer()->GetSession()).PSendSysMessage(
                "|cffff0000[Guild Ban]|r Your account has been banned from <%s>. Reason: %s",
                guild->GetName().c_str(), banReason.c_str());
        }

        return true;
    }

    static bool HandleGbanRemoveCommand(ChatHandler* handler, Optional<PlayerIdentifier> target)
    {
        Player* admin = handler->GetSession()->GetPlayer();
        if (!admin)
            return false;

        Guild* guild = admin->GetGuild();
        if (!guild)
        {
            handler->SendErrorMessage("You are not in a guild.");
            return false;
        }

        if (!CanBan(handler, guild, admin))
            return false;

        if (!target)
            target = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!target)
        {
            handler->SendErrorMessage("No player specified.");
            return false;
        }

        ObjectGuid targetGuid = target->GetGUID();
        std::string targetName = target->GetName();

        if (!sGuildBanMgr->IsCharacterBanned(guild->GetId(), targetGuid.GetCounter()))
        {
            handler->SendErrorMessage("Player %s is not banned from this guild.", targetName.c_str());
            return false;
        }

        sGuildBanMgr->RemoveBan(guild->GetId(), targetGuid.GetCounter());

        handler->PSendSysMessage("|cff00ff00[Guild Ban]|r Ban removed for player %s.", targetName.c_str());

        return true;
    }

    static bool HandleGbanListCommand(ChatHandler* handler)
    {
        Player* admin = handler->GetSession()->GetPlayer();
        if (!admin)
            return false;

        Guild* guild = admin->GetGuild();
        if (!guild)
        {
            handler->SendErrorMessage("You are not in a guild.");
            return false;
        }

        if (!sGuildBanMgr->IsEnabled())
        {
            handler->SendErrorMessage("Guild ban system is disabled.");
            return false;
        }

        auto bans = sGuildBanMgr->GetGuildBans(guild->GetId());

        if (bans.empty())
        {
            handler->PSendSysMessage("|cff00ff00[Guild Ban]|r No bans for this guild.");
            return true;
        }

        handler->PSendSysMessage("|cff00ff00[Guild Ban]|r Ban list for <%s>:", guild->GetName().c_str());
        handler->PSendSysMessage("-------------------------------------------");

        for (auto const& ban : bans)
        {
            std::string charName = "Unknown";
            if (CharacterCacheEntry const* entry = sCharacterCache->GetCharacterCacheByGuid(ObjectGuid::Create<HighGuid::Player>(ban.guid)))
            {
                charName = entry->Name;
            }

            std::string banTypeStr = ban.banType == GUILD_BAN_ACCOUNT ? "Account" : "Character";
            std::string expiryStr = ban.unbanDate == 0 ? "Permanent" : Acore::Time::TimeToTimestampStr(Seconds(ban.unbanDate));

            handler->PSendSysMessage("  %s [%s] - Banned by: %s - Expires: %s",
                                     charName.c_str(), banTypeStr.c_str(),
                                     ban.bannedBy.c_str(), expiryStr.c_str());
            handler->PSendSysMessage("    Reason: %s", ban.banReason.c_str());
        }

        return true;
    }
};

void AddGuildBanCommands()
{
    new GuildBan_CommandScript();
}
