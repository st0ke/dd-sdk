# Project Notes

- This is a CMake project.
- Use `../../build/dd-sdk-vscode/RelWithDebInfo` as the build directory for compilation checks and unit test execution.
- Resource files (scripts, textures, models, etc.) are placed in the `../../data/sandbox001` directory and controlled by another Git repository.
- Format C++ files via the clang-format command only when the edited file is in a directory containing `.clang-format` or recursively below it. Currently, only `game/mapgen` has a `.clang-format`, so do not run clang-format on C++ files outside `game/mapgen`, such as `game/gamesys/SysCmds.cpp`.
- Place report files (e.g., when an answer is too large) in the `ai-reports/` directory. Add the creation date to the beginning of each filename.

## C++ Code Style

- Prefer anonymous namespaces over file-scope `static` for internal-linkage functions and constants.

## Licensing

For new project-authored source files and substantive scripts, put the following copyright notice as a file header (use '//' for C++). Do not add it to upstream vendored files, tiny metadata files, generated version markers, or skill/frontmatter YAML files unless explicitly requested:

```
DD game project
Copyright (C) 2026 Alexander Boldyrev <boldir@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see https://www.gnu.org/licenses/.
```
