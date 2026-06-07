// DD game project
// Copyright (C) 2026 Alexander Boldyrev <boldir@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see https://www.gnu.org/licenses/.

#ifndef __GAME_MAPGEN_CMDS_H__
#define __GAME_MAPGEN_CMDS_H__

class idStr;

bool MapGen_Join(const char* planName, const char* outputMapName, idStr& status);

typedef void (*mapgenCommandBuffer_t)(const char* commandText, void* userData);

bool MapGen_DevMap(
    const char* planName, mapgenCommandBuffer_t bufferCommand, void* userData, idStr& status, idStr& outputMapName);

#endif /* !__GAME_MAPGEN_CMDS_H__ */
