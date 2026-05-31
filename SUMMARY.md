# Project Summary

## What This Repository Is

This repository is a C/C++ mod SDK for dhewm3, the modernized Doom 3 engine port. It contains GPLv3-licensed Doom 3 game-code sources and engine interface headers so mods can build game libraries that are loaded by dhewm3.

The repository does not contain the full dhewm3 engine implementation. It mainly provides:

- The base Doom 3 game module in `game/`
- The Resurrection of Evil / D3XP game module in `d3xp/`
- Shared low-level support code in `idlib/`
- Public engine interfaces for renderer, sound, UI, file system, declarations, commands, CVars, collision, and platform services
- A CMake build that produces loadable game libraries

## High-Level Inventory

- Languages: C++ and C-style C/C++ headers
- Build system: CMake
- Files scanned: 431 project files
- Source/header mix: 194 `.cpp` files, 233 `.h` files, plus docs/config/license files
- Approximate line count: 393k lines across source, headers, docs, and license text
- License: GPLv3-or-later terms are included in source headers and `COPYING.txt`

## Main Build Targets

`CMakeLists.txt` defines these targets:

- `idlib`: static library built from shared utility code in `idlib/`
- `base`: shared game library built from `game/`
- `d3xp`: shared game library built from `d3xp/`

Important CMake options and cache variables:

- `BASE`: build the base game code, default `ON`
- `BASE_NAME`: output name for the `game/` library, default `base`
- `BASE_DEFS`: compile definitions for `game/`, default `GAME_DLL`
- `D3XP`: build the expansion game code, default `ON`
- `D3XP_NAME`: output name for the `d3xp/` library, default `d3xp`
- `D3XP_DEFS`: compile definitions for `d3xp/`, default `GAME_DLL;_D3XP;CTF`
- `ONATIVE`: optimize for the host CPU, default `OFF`
- `FORCE_COLORED_OUTPUT`: colored GCC/Clang diagnostics, default `OFF`
- `ASAN`: GCC/Clang AddressSanitizer option, default `OFF`

The default build type is `RelWithDebInfo` when no build type is provided.

## Build and Platform Notes

The README documents manual builds for Windows and Unix-like systems.

Typical Linux/Unix flow:

```sh
mkdir build
cd build
cmake ..
make -j4
```

Windows builds are intended to work through CMake-generated Visual Studio projects or MinGW-w64. The build file also contains platform handling for macOS, Linux/Unix, Windows/MSVC, MinGW, and AROS. Install rules are only active for non-Apple, non-Windows platforms.

`.gitignore` only ignores `build*`, so build directories such as `build/` or `build-debug/` are expected.

## Runtime Entry Point

Both `game/` and `d3xp/` expose the Doom 3 game DLL API:

- `GetGameAPI(gameImport_t *import)` in `game/Game_local.cpp`
- `GetGameAPI(gameImport_t *import)` in `d3xp/Game_local.cpp`

The public API is declared in `framework/Game.h`.

Key API types:

- `GAME_API_VERSION` is `9`
- `gameImport_t` is the set of engine services passed into the game library
- `gameExport_t` returns the game interfaces back to the engine
- `idGame` is the main runtime game interface
- `idGameEdit` exposes in-game editing and tooling helpers

`idGame` covers initialization, shutdown, map loading, save/load, player spawning, per-frame simulation, drawing, GUI command handling, networking, prediction, snapshots, team switching, downloads, and map loading UI.

## Directory Breakdown

### `game/`

Base Doom 3 game code. This is the main gameplay module for the vanilla game.

Important areas:

- Entity system: `Entity.*`, `Actor.*`, `Player.*`, `Item.*`, `Projectile.*`, `Weapon.*`
- Game runtime: `Game_local.*`, `Game_network.cpp`, `GameEdit.*`, `GameBase.h`
- AI and AAS use: `ai/`
- Animation: `anim/`
- Physics and forces: `physics/`
- Scripting runtime/compiler: `script/`
- Save game, events, class metadata, console commands, CVars: `gamesys/`
- Multiplayer rules and UI state: `MultiplayerGame.*`
- World systems: PVS, movers, triggers, lights, sound entities, cameras, smoke particles, FX, articulated figures

`idGameLocal` in `game/Game_local.h` is the central implementation of `idGame`. It owns map state, entities, players, networking snapshots, collision, PVS, scripts, multiplayer state, and per-frame timing.

### `d3xp/`

Expansion game code. It mirrors most of `game/` but adds Resurrection of Evil / D3XP behavior and CTF-related code through `_D3XP` and `CTF` compile definitions.

Notable additions and differences:

- `Grabber.*`
- `physics/Force_Grab.*`
- D3XP-specific player view/effects and gameplay changes
- CTF game type, flag status, team sounds, point limits, and extra multiplayer messages
- Slow-motion/time-group support visible in `GameBase.h` and `Game_local.h`

### `idlib/`

Shared low-level support library used by both game modules.

Major components:

- Math: vectors, matrices, planes, rotations, quaternions, curves, polynomials, ODE helpers, LCP solver
- SIMD backends: generic, MMX, 3DNow, SSE, SSE2, SSE3, AltiVec
- Geometry: windings, surfaces, patches, swept splines, trace models, draw vertices, joint transforms
- Bounds/volumes: bounds, boxes, spheres, frustums
- Containers: lists, hash tables/indexes, queues, stacks, trees, link lists, string pools
- Parsing/data: lexer, parser, tokens, dictionaries, map files, language dictionaries
- Utility: strings, timers, heap, bit messages, base64, CRC32, MD4, MD5

`idlib/Lib.h` documents `idLib` as mostly stateless support code with engine pointers supplied by the host.

### `framework/`

Public engine-facing framework interfaces and declarations. These are mostly headers, not full engine implementations.

Key interfaces:

- `Game.h`: game DLL API, `idGame`, `idGameEdit`
- `Common.h`: engine common loop/services interface
- `CmdSystem.h`: console command registration and command buffering
- `CVarSystem.h`: console variables
- `File.h` and `FileSystem.h`: filesystem abstraction and background downloads
- `DeclManager.h` plus declaration headers: materials, skins, FX, particles, AF, PDA, entity defs, tables
- `UsercmdGen.h`: user command/input structures
- `async/NetworkSystem.h`: networking interface

### `renderer/`

Renderer-facing public interfaces and render data structures.

Includes:

- `RenderSystem.h`: render system interface, fonts, screen dimensions, render world allocation
- `RenderWorld.h`: render entities, lights, views, portal/proc constants, shader parameters
- `Model.h`, `ModelManager.h`: render model data and model manager interface
- `Material.h`: material/declaration structures
- `Cinematic.h`: cinematic interface
- `qgl.h`: OpenGL-related declarations

These are headers used by game code and tools; they are not the full renderer implementation.

### `sound/`

`sound/sound.h` defines sound shader data, sound emitters, sound worlds, sound system concepts, spatialization parameters, channels, and sound flags.

### `ui/`

User interface interfaces:

- `UserInterface.h`: interactive GUI abstraction and GUI manager
- `ListGUI.h`: list GUI helper interface

Doom 3 GUI files are not present here; this is the C++ interface layer.

### `sys/`

Platform and OS abstraction headers:

- `platform.h`: platform macros, exports, alignment, path separators, CPU architecture definitions, standard includes
- `sys_public.h`: system services such as events, clipboard, dynamic libraries, input polling, timing, CPU detection, paths, files, and window/input handling
- `Stub_SDL_endian.h`: endian support shim

### `cm/`

`CollisionModel.h` declares the collision model manager API for trace-model vs polygonal collision detection, contacts, translations, rotations, contents checks, and collision model loading.

### `tools/compilers/aas/`

AAS navigation file interfaces:

- `AASFile.h`: AAS file format structures, travel flags, reachability, areas, portals, clusters, traces, settings
- `AASFileManager.h`: AAS file manager interface

### `MayaImport/`

`maya_main.h` declares function pointer types for a Maya export/import DLL interface.

### Root Files

- `README.md`: project description, build instructions, and mod-porting workflow
- `CMakeLists.txt`: full build configuration
- `config.h.in`: generated build configuration template
- `COPYING.txt`: GPLv3 license text
- `.gitignore`: ignores `build*`

The hidden `.agents/` and `.codex/` directories are present but did not contain project files at scan depth.

## Main Gameplay Systems Present

The source includes implementations for:

- Single-player and multiplayer game loops
- Entity lifecycle, spawn args, thinking, events, networking, save/restore
- Player movement, weapons, HUD/view effects, damage, inventory-related logic
- AI navigation and behavior, including AAS pathing/routing
- Script compiler, interpreter, thread/program runtime
- Physics objects, articulated figures, collision clips, player/monster physics, rigid bodies, movers, pushers, forces
- Animation blending, MD5 animation support, test model helpers
- Multiplayer match state, voting, chat, scoreboard, snapshots, and teams
- In-game editing hooks and map/entity helper APIs
- PVS, render entity/light setup, sounds, FX, smoke particles, cameras, triggers, targets, security cameras

## What Is Not Included

- Full dhewm3 engine implementation
- Game assets such as maps, textures, sounds, models, scripts, GUIs, or pak files
- Standalone executable target
- Automated tests or CTest integration
- CI configuration
- Package/dependency manager metadata

## How To Extend A Mod

The intended mod workflow is to modify `game/` or `d3xp/` and build a game library loaded by dhewm3.

For new `.cpp` files:

- Add base-game files to the `src_game_mod` list in `CMakeLists.txt`
- Add expansion files to the `src_d3xp_mod` list in `CMakeLists.txt`

For output names:

- Change `BASE_NAME` to produce a custom library name from `game/`
- Change `D3XP_NAME` to produce a custom library name from `d3xp/`

The generated library should be placed next to dhewm3's game libraries and loaded with `+set fs_game <modname>`, with matching game data in the corresponding mod directory.

## Notable Technical Details

- The codebase is legacy Doom 3 C++ and does not set a modern C++ standard in CMake.
- The build uses many compatibility flags and warning suppressions for GCC/Clang/MSVC.
- GCC/Clang builds use `-fPIC` for `idlib` when needed by shared game libraries.
- `config.h` is generated from `config.h.in` and includes build OS, CPU, endian, library suffix, install libdir, and install datadir.
- The code has many historical `FIXME` and `TODO` comments inherited from Doom 3/dhewm3.
- `d3xp/` duplicates much of `game/`, so changes to shared gameplay behavior may need to be applied in both trees.

## Quick Orientation For New Work

- Start with `README.md` for build and mod-porting workflow.
- Start with `CMakeLists.txt` when adding/removing source files or changing output library names.
- Start with `framework/Game.h` to understand the game DLL contract.
- Start with `game/Game_local.*` or `d3xp/Game_local.*` for map/game-frame lifecycle.
- Start with `game/Entity.*`, `Actor.*`, `Player.*`, and `Weapon.*` for gameplay behavior.
- Start with `game/gamesys/SysCmds.cpp` and `game/gamesys/SysCvar.*` for console commands and CVars.
- Mirror relevant changes into `d3xp/` when the expansion module should keep parity.

## `game/` Deep Dive Starting From `idGameLocal`

`game/` is the vanilla Doom 3 gameplay module. It builds into the `base` game library, and `idGameLocal` is the best starting point because it owns the game state and implements the engine-facing `idGame` API.

### `idGameLocal`

`idGameLocal` is declared in `game/Game_local.h`. It inherits from `idGame`, which is the interface dhewm3 calls into. A global singleton is created in `game/Game_local.cpp`: `gameLocal`, and the exported `idGame *game` points at it.

The DLL entry point is `GetGameAPI()` in `game/Game_local.cpp`. It receives engine services like `renderSystem`, `soundSystem`, `fileSystem`, `declManager`, `collisionModelManager`, and others, stores them globally, wires them into `idLib`, then returns `gameExport` with `game` and `gameEdit`.

Think of `idGameLocal` as the coordinator for:

- Server/game settings: `serverInfo`, `userInfo`, `persistentPlayerInfo`
- Entity storage: `entities`, `spawnIds`, `spawnedEntities`, `activeEntities`
- Per-map systems: `clip`, `push`, `pvs`, `aasList`, `mapFile`
- Script runtime: `program`, `frameCommandThread`
- Multiplayer: `mpGame`, snapshots, client PVS, network event queues
- Rendering/audio handles: global `gameRenderWorld` and `gameSoundWorld`
- Timing: `framenum`, `time`, `previousTime`, `msec`

Those fields are grouped in `game/Game_local.h`.

### Startup

`idGameLocal::Init()` is the one-time game-module startup path in `game/Game_local.cpp`. It initializes `idLib`, static CVars, SIMD, game decl types/folders, events/classes, console commands, default scripts, smoke particles, and AAS types.

The important startup chain is:

1. `GetGameAPI()` imports engine services.
2. `Init()` initializes game-side systems.
3. `program.Startup(SCRIPT_DEFAULT)` loads default script code.
4. AAS types are read from the `aas_types` entityDef.

### Map Load

A new map enters through `InitFromNewMap()` in `game/Game_local.cpp`. It sets server/client flags, assigns the render and sound worlds, calls `LoadMap()`, initializes map scripts, spawns map entities, resets multiplayer state, precaches multiplayer assets, and marks the game active.

`LoadMap()` in `game/Game_local.cpp` does the heavy reset work: parse `.map`, load collision models, clear entity arrays, reset timing, initialize `clip` and `pvs`, initialize AAS files, reset smoke particles, and cache extra media.

`MapPopulate()` in `game/Game_local.cpp` calls `SpawnMapEntities()`, spreads location names, prepares initial spawn points, and services events so map script `main()` runs before the first real physics frame.

### Frame Loop

`RunFrame()` in `game/Game_local.cpp` is the main simulation loop. Per frame it:

- Advances `framenum`, `time`, and `msec`
- Copies user commands into `usercmds`
- Updates the active player render view
- Clears debug render data
- Frees old smoke particles
- Processes server entity network events
- Updates gravity
- Builds player PVS
- Sorts active entities
- Calls `ent->Think()` on every active entity
- Services queued game events with `idEvent::ServiceEvents()`
- Runs multiplayer logic through `mpGame.Run()`
- Returns health/stamina/combat/session-command data to the engine

Drawing is separate. `Draw()` in `game/Game_local.cpp` delegates to `mpGame.Draw()` for multiplayer, otherwise it renders the local player view through `player->playerView.RenderPlayerView()`.

### Player Spawn

`SpawnPlayer()` in `game/Game_local.cpp` builds spawn args, picks `player_doommarine` or `player_doommarine_mp`, calls `SpawnEntityDef()`, verifies the result is an `idPlayer`, updates `numClients`, then notifies `mpGame`.

### Networking

The async networking path is split into `game/Game_network.cpp`. Server snapshots start at `ServerWriteSnapshot()`. Client snapshot ingestion starts at `ClientReadSnapshot()`. Client-side prediction runs in `ClientPrediction()`.

The key rule in that file: client game code should not spawn entities except while reading snapshots.

### Entity Model

Most gameplay objects inherit from `idEntity`, declared in `game/Entity.h`. It contains entity numbers, spawn args, script object, think flags, render/sound hooks, target lists, health, networking flags, and active/snapshot list nodes.

Core inheritance path:

- `idClass`: RTTI/events/spawn/save/restore base, in `game/gamesys/Class.h`
- `idEntity`: base world object
- `idAnimatedEntity`: animated renderable entity
- `idAFEntity`: articulated figure entities
- `idActor`: players and monsters, in `game/Actor.h`
- `idPlayer`: player state/input/HUD/inventory, in `game/Player.h`
- `idWeapon`: weapon state machine and script object, in `game/Weapon.h`

### Good Reading Order

Start here:

1. `game/Game_local.h`: `idGameLocal` state and public API.
2. `game/Game_local.cpp`: startup, map load, frame loop, draw.
3. `game/Entity.h`: base entity contract.
4. `game/Player.cpp`: player behavior.
5. `game/Game_network.cpp`: multiplayer snapshots/prediction.
6. `game/script/` and `game/gamesys/`: scripting, events, save games, commands, CVars.
