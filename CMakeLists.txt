# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# Define the mod-guild-ban module
AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/GuildBan_loader.cpp")

# Add all source files
CU_ADD_HOOK(GUILD_BAN "${CMAKE_CURRENT_LIST_DIR}/src/GuildBan_SC.cpp")
CU_ADD_HOOK(GUILD_BAN "${CMAKE_CURRENT_LIST_DIR}/src/GuildBan_Commands.cpp")
