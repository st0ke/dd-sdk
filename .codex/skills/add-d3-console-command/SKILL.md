---
name: add-d3-console-command
description: Add, modify, or review Doom 3/dhewm3 SDK console commands in a game DLL. Use when Codex needs to implement a command registered through cmdSystem AddCommand, edit game/gamesys/SysCmds.cpp or d3xp/gamesys/SysCmds.cpp, work with idCmdArgs, choose CMD_FL_GAME/CMD_FL_CHEAT flags, add argument completion, or explain how console commands are wired in this SDK.
---

# Add D3 Console Command

## Overview

Use this workflow to add commands to the dhewm3/Doom 3 game DLL through the existing `idCmdSystem` interface. Keep changes aligned with the local `SysCmds.cpp` style and register commands during game initialization, not per-map runtime.

## Workflow

1. Inspect the command system and local style before editing:

```bash
rg -n "InitConsoleCommands|ShutdownConsoleCommands|AddCommand|CMD_FL_GAME|CheatsOk" game/gamesys/SysCmds.cpp d3xp/gamesys/SysCmds.cpp framework/CmdSystem.h
```

2. Choose the target tree:

- Base game commands go in `game/gamesys/SysCmds.cpp`.
- Resurrection of Evil / d3xp commands go in `d3xp/gamesys/SysCmds.cpp`.
- If the user asks for behavior in both, apply the same scoped change to both trees.
- If the target is ambiguous, infer from nearby requested files or existing branch changes. Ask only when choosing the wrong DLL would create real rework.

3. Add a command function near related `Cmd_*_f` functions. Prefer a `static void Cmd_Name_f( const idCmdArgs &args )` helper unless existing nearby commands are non-static.

4. Register it in `idGameLocal::InitConsoleCommands()` with `cmdSystem->AddCommand(...)`.

5. Verify that every new game command includes `CMD_FL_GAME`; `ShutdownConsoleCommands()` removes commands by this flag.

## Implementation Pattern

Use this shape for simple commands:

```cpp
static void Cmd_MyCommand_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		gameLocal.Printf( "usage: myCommand <value>\n" );
		return;
	}

	gameLocal.Printf( "value: %s\n", args.Argv( 1 ) );
}
```

Register it like this:

```cpp
cmdSystem->AddCommand( "myCommand", Cmd_MyCommand_f, CMD_FL_GAME, "prints a value" );
```

For cheat/debug commands that mutate gameplay state:

```cpp
static void Cmd_MyCheat_f( const idCmdArgs &args ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	// mutate game state here
}
```

Register gameplay-mutating debug commands with:

```cpp
cmdSystem->AddCommand( "myCheat", Cmd_MyCheat_f, CMD_FL_GAME|CMD_FL_CHEAT, "does the thing" );
```

## Argument Handling

- Use `args.Argc()` for arity checks and print a short usage line on mismatch.
- Use `args.Argv( n )` for individual tokens and `args.Args()` only when the command intentionally consumes the rest of the line as text.
- Use existing helpers when present, such as `Cmd_GetFloatArg`.
- Avoid ad hoc string parsing when `idCmdArgs`, `idStr`, or `idDict` already fits the job.

## Argument Completion

Use an existing completion callback when it matches the first argument:

- Entity names: `idGameLocal::ArgCompletion_EntityName`
- Entity defs: `idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>`
- Models, skins, FX, and other decls: use the matching `DECL_*` with `idCmdSystem::ArgCompletion_Decl<...>`
- Files or maps: use the relevant `idCmdSystem::ArgCompletion_*` helper from `framework/CmdSystem.h`

Example:

```cpp
cmdSystem->AddCommand( "myEntityCommand", Cmd_MyEntityCommand_f, CMD_FL_GAME|CMD_FL_CHEAT, "acts on an entity", idGameLocal::ArgCompletion_EntityName );
```

## Guardrails

- Do not edit the engine command system for a normal game/mod command.
- Do not use `CMD_FL_SYSTEM` for commands owned by the game DLL.
- Do not register commands during map load or entity spawn unless the command is intentionally temporary.
- Do not omit `CMD_FL_GAME`; otherwise shutdown cleanup will miss the command.
- Be careful with multiplayer: client commands should not directly mutate authoritative server state unless the surrounding code already does that safely.
- If adding a new source file instead of editing `SysCmds.cpp`, update `CMakeLists.txt` in the matching source list. Prefer editing `SysCmds.cpp` for simple command additions.

## Validation

After editing:

1. Run `git diff --check`.
2. Search for build directories with `rg --files -g CMakeCache.txt`.
3. If a build directory exists, run the appropriate `cmake --build <dir>` command.
4. If no build directory exists, say compilation was not run and report the static checks performed.
