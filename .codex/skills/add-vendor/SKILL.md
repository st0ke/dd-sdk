---
name: add-vendor
description: Add or update vendored third-party dependencies in this dd-sdk repository. Use when Codex needs to create root-level vendor subdirectories, add self-contained update scripts, preserve upstream licenses, avoid unnecessary build integration, or summarize dependency vendoring work for this project.
---

# Add Vendor

## Workflow

1. Inspect the repository for existing dependency conventions before creating files:

```bash
find . -maxdepth 3 -type d -name vendor -o -name third_party -o -name external
rg -n "vendor|third|external|FetchContent|add_subdirectory" CMakeLists.txt game d3xp -g 'CMakeLists.txt'
```

2. If there is no existing convention, use `vendor/<dependency_name>/` under the project root.

3. Keep the vendored dependency self-contained:

- Put upstream files under predictable paths, such as `include/<namespace>/...`.
- Include the upstream license file when it is available.
- Add a small `VERSION` file if the update script writes one.
- Do not modify root `CMakeLists.txt` unless the user asks for build integration or the dependency must compile.

4. Add an update script in the dependency subdirectory:

- Use Python for portable download/update scripts.
- Make the script accept an optional version argument and default to the upstream latest release only when that is reasonable.
- Download from stable upstream release/tag URLs.
- Regenerate the same files it initially vendors.

5. For new project-authored source files and substantive scripts, add the project copyright header from `AGENTS.md`.
Do not add project headers to upstream vendored files, tiny metadata files, generated version markers, or skill/frontmatter YAML files unless explicitly requested.

## Validation

After vendoring:

1. Run the update script once for the intended version.
2. Run `git diff --check`.
3. If build files were changed, build with `cmake --build ../../build/dd-sdk-vscode/RelWithDebInfo`.
4. Report the vendored version, important paths, and any validation that could not be run.
