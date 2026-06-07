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

#include "sys/platform.h"

#include "Cmds.h"
#include "Join.h"

namespace {

const char* const MAPGEN_DEVMAP_OUTPUT = "maps/mapgen/devmap";
const char* const MAPGEN_DEVMAP_MAP = "mapgen/devmap";

void BufferCommand(mapgenCommandBuffer_t bufferCommand, void* userData, const char* commandName, const char* mapName) {
    idStr commandText;

    commandText = commandName;
    commandText += " ";
    commandText += mapName;
    commandText += "\n";
    bufferCommand(commandText.c_str(), userData);
}

}

bool MapGen_Join(const char* planName, const char* outputMapName, idStr& status) {
    mapgenJoinJob job;
    return job.Run(planName, outputMapName, status);
}

bool MapGen_DevMap(
    const char* planName, mapgenCommandBuffer_t bufferCommand, void* userData, idStr& status, idStr& outputMapName) {
    if (!MapGen_Join(planName, MAPGEN_DEVMAP_OUTPUT, status)) {
        return false;
    }

    outputMapName = MAPGEN_DEVMAP_OUTPUT;
    outputMapName.BackSlashesToSlashes();
    outputMapName.StripFileExtension();
    outputMapName.SetFileExtension("map");

    if (bufferCommand != NULL) {
        BufferCommand(bufferCommand, userData, "dmap", MAPGEN_DEVMAP_MAP);
        BufferCommand(bufferCommand, userData, "devmap", MAPGEN_DEVMAP_MAP);
    }

    return true;
}
