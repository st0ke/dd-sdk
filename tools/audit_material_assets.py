#!/usr/bin/env python3
"""
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
"""

from __future__ import annotations

import argparse
import collections
import dataclasses
import re
from pathlib import Path


IMAGE_DIRECTIVES = {
    "bumpmap",
    "clampmap",
    "diffusemap",
    "map",
    "qer_editorimage",
    "specularmap",
}
RUNTIME_KINDS = {"runtime image", "cubemap", "video", "GUI"}
PATH_PATTERN = re.compile(
    r"""(?ix)
    (?:[a-z0-9_@.-]+/)+[a-z0-9_@./-]+
    |
    [a-z0-9_@.-]+\.(?:tga|jpg|jpeg|png|dds|roq|lwo|ase|ma|gui)
    """
)
DIRECTIVE_PATTERN = re.compile(r"\s*([A-Za-z_]+)\b(.*)")
IMAGE_EXTENSION_PATTERN = re.compile(r"\.(?:tga|jpg|jpeg|png|dds)$", re.IGNORECASE)
REFERENCE_TOKEN_PATTERN = re.compile(rb"[A-Za-z0-9_@.+/\\-]+")
NON_MATERIAL_DECLARATIONS = ("particle ", "skin ", "table ")


@dataclasses.dataclass(frozen=True)
class Dependency:
    kind: str
    path: str
    exists: bool
    line: int


@dataclasses.dataclass
class Material:
    name: str
    source: Path
    line: int
    dependencies: list[Dependency]


def strip_comments(text: str) -> str:
    text = re.sub(
        r"/\*.*?\*/",
        lambda match: "\n" * match.group(0).count("\n"),
        text,
        flags=re.DOTALL,
    )
    return re.sub(r"//.*", "", text).replace("\\", "/")


def material_blocks(path: Path):
    current_name: str | None = None
    pending_name: str | None = None
    start_line = 0
    depth = 0
    body: list[str] = []

    text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    for line_number, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if depth == 0:
            if not stripped:
                continue
            if "{" not in stripped:
                if stripped != "}":
                    pending_name = stripped
                continue

            prefix = stripped.split("{", 1)[0].strip()
            declaration = prefix or pending_name
            pending_name = None
            depth = line.count("{") - line.count("}")
            if declaration and not declaration.lower().startswith(
                NON_MATERIAL_DECLARATIONS
            ):
                current_name = re.sub(
                    r"^material\s+", "", declaration, flags=re.IGNORECASE
                )
                start_line = line_number
                body = [line]
            if depth == 0 and current_name is not None:
                yield current_name, start_line, body
                current_name = None
            continue

        body.append(line)
        depth += line.count("{") - line.count("}")
        if depth == 0:
            if current_name is not None:
                yield current_name, start_line, body
            current_name = None
            body = []


def logical_lines(start_line: int, lines: list[str]):
    accumulated = ""
    accumulated_line = start_line
    parentheses = 0
    for offset, line in enumerate(lines):
        if not accumulated:
            accumulated = line
            accumulated_line = start_line + offset
        else:
            accumulated += " " + line.strip()
        parentheses += line.count("(") - line.count(")")
        if parentheses <= 0:
            yield accumulated_line, accumulated
            accumulated = ""
            parentheses = 0
    if accumulated:
        yield accumulated_line, accumulated


class ResourceIndex:
    def __init__(self, base: Path):
        self.base = base
        self.files = {
            path.relative_to(base).as_posix().lower()
            for path in base.rglob("*")
            if path.is_file()
        }

    def has_file(self, path: str) -> bool:
        return path.strip('"').lower() in self.files

    def has_image(self, path: str) -> bool:
        normalized = path.strip('"').lower()
        stem = IMAGE_EXTENSION_PATTERN.sub("", normalized)
        candidates = [
            normalized,
            stem + ".tga",
            stem + ".jpg",
            stem + ".jpeg",
            stem + ".png",
            stem + ".dds",
            "dds/" + stem + ".dds",
        ]
        return any(candidate in self.files for candidate in candidates)

    def missing_cube_faces(self, path: str, camera: bool) -> list[str]:
        stem = IMAGE_EXTENSION_PATTERN.sub("", path.strip('"').lower())
        suffixes = (
            ["_forward", "_back", "_left", "_right", "_up", "_down"]
            if camera
            else ["_px", "_nx", "_py", "_ny", "_pz", "_nz"]
        )
        return [stem + suffix for suffix in suffixes if not self.has_image(stem + suffix)]

    def has_video(self, path: str) -> bool:
        normalized = path.strip('"').lower()
        if normalized in self.files:
            return True
        return "/" not in normalized and "video/" + normalized in self.files


def parse_dependencies(index: ResourceIndex, start_line: int, body: list[str]):
    dependencies: list[Dependency] = []
    for line_number, line in logical_lines(start_line, body):
        match = DIRECTIVE_PATTERN.match(line)
        if not match:
            continue
        directive = match.group(1).lower()
        arguments = match.group(2)
        paths = PATH_PATTERN.findall(arguments)

        if directive in IMAGE_DIRECTIVES:
            kind = "editor image" if directive == "qer_editorimage" else "runtime image"
            dependencies.extend(
                Dependency(kind, path, index.has_image(path), line_number)
                for path in paths
            )
        elif directive in {"cubemap", "cameracubemap"}:
            for path in paths:
                missing = index.missing_cube_faces(
                    path, camera=directive == "cameracubemap"
                )
                if missing:
                    dependencies.extend(
                        Dependency("cubemap", face, False, line_number)
                        for face in missing
                    )
                else:
                    dependencies.append(Dependency("cubemap", path, True, line_number))
        elif directive == "videomap":
            dependencies.extend(
                Dependency("video", path, index.has_video(path), line_number)
                for path in paths
            )
        elif directive == "guisurf":
            dependencies.extend(
                Dependency("GUI", path, index.has_file(path), line_number)
                for path in paths
                if path.lower().endswith(".gui")
            )
        elif directive == "renderbump":
            # The final geometry argument is an offline high-poly authoring input.
            models = [
                path
                for path in paths
                if path.lower().endswith((".lwo", ".ase", ".ma"))
            ]
            dependencies.extend(
                Dependency("authoring model", path, index.has_file(path), line_number)
                for path in models[-1:]
            )
    return dependencies


def scan(base: Path) -> list[Material]:
    index = ResourceIndex(base)
    materials = []
    for source in sorted((base / "materials").glob("*.mtr")):
        for name, line, body in material_blocks(source):
            materials.append(
                Material(name, source, line, parse_dependencies(index, line, body))
            )
    return materials


def scan_references(base: Path, names: set[str]) -> dict[str, list[str]]:
    normalized_names = {name.lower(): name for name in names}
    references: dict[str, list[str]] = collections.defaultdict(list)

    for path in base.rglob("*"):
        if not path.is_file() or (
            path.parent == base / "materials" and path.suffix.lower() == ".mtr"
        ):
            continue
        relative = path.relative_to(base).as_posix()
        data = path.read_bytes()
        found_in_file = set()
        for match in REFERENCE_TOKEN_PATTERN.finditer(data):
            token = match.group(0).replace(b"\\", b"/").lower()
            try:
                normalized = token.decode("ascii")
            except UnicodeDecodeError:
                continue
            original = normalized_names.get(normalized)
            if original is not None:
                found_in_file.add(original)
        for name in found_in_file:
            references[name].append(relative)
    return references


def material_line_spans(path: Path):
    raw = path.read_bytes().decode("latin-1")
    raw_lines = raw.splitlines(keepends=True)
    clean_lines = strip_comments(raw).splitlines()
    current_name: str | None = None
    pending_name: str | None = None
    pending_line: int | None = None
    declaration_line = 0
    brace_line = 0
    depth = 0

    for line_index, line in enumerate(clean_lines):
        stripped = line.strip()
        if depth == 0:
            if not stripped:
                continue
            if "{" not in stripped:
                if stripped != "}":
                    pending_name = stripped
                    pending_line = line_index
                continue

            prefix = stripped.split("{", 1)[0].strip()
            declaration = prefix or pending_name
            declaration_line = line_index if prefix else pending_line or line_index
            brace_line = line_index
            pending_name = None
            pending_line = None
            depth = line.count("{") - line.count("}")
            if declaration and not declaration.lower().startswith(
                NON_MATERIAL_DECLARATIONS
            ):
                current_name = re.sub(
                    r"^material\s+", "", declaration, flags=re.IGNORECASE
                )
            if depth == 0 and current_name is not None:
                yield (
                    current_name,
                    brace_line + 1,
                    declaration_line,
                    line_index + 1,
                    raw_lines,
                )
                current_name = None
            continue

        depth += line.count("{") - line.count("}")
        if depth == 0:
            if current_name is not None:
                yield (
                    current_name,
                    brace_line + 1,
                    declaration_line,
                    line_index + 1,
                    raw_lines,
                )
            current_name = None


def remove_materials(materials: list[Material]) -> tuple[int, int]:
    targets_by_source: dict[Path, set[tuple[str, int]]] = collections.defaultdict(set)
    for material in materials:
        targets_by_source[material.source].add((material.name, material.line))

    removed = 0
    changed_files = 0
    for source, targets in targets_by_source.items():
        spans = list(material_line_spans(source))
        if not spans:
            continue
        raw_lines = spans[0][4]
        delete_lines = set()
        matched = set()
        for name, brace_line, start, end, _ in spans:
            key = (name, brace_line)
            if key not in targets:
                continue
            delete_lines.update(range(start, end))
            matched.add(key)
        unmatched = targets - matched
        if unmatched:
            details = ", ".join(f"{name}:{line}" for name, line in sorted(unmatched))
            raise RuntimeError(f"Could not locate material blocks in {source}: {details}")
        if not delete_lines:
            continue
        output = b"".join(
            line.encode("latin-1")
            for index, line in enumerate(raw_lines)
            if index not in delete_lines
        )
        source.write_bytes(output)
        removed += len(matched)
        changed_files += 1
    return removed, changed_files


def classify(material: Material) -> str | None:
    missing = [dependency for dependency in material.dependencies if not dependency.exists]
    if not missing:
        return None

    runtime = [
        dependency
        for dependency in material.dependencies
        if dependency.kind in RUNTIME_KINDS
    ]
    if runtime and not any(dependency.exists for dependency in runtime):
        return "полностью неработоспособен"
    if any(not dependency.exists for dependency in runtime):
        return "частично неработоспособен"
    if any(dependency.kind == "editor image" for dependency in missing):
        return "нет preview для редактора"
    return "нет authoring-исходника"


def escape_cell(value: str) -> str:
    return value.replace("\\", "\\\\").replace("|", "\\|").replace("\n", " ")


def compact_missing(material: Material) -> str:
    grouped: dict[str, list[str]] = collections.defaultdict(list)
    for dependency in material.dependencies:
        if not dependency.exists and dependency.path not in grouped[dependency.kind]:
            grouped[dependency.kind].append(dependency.path)
    return "<br>".join(
        f"**{escape_cell(kind)}:** "
        + ", ".join(f"`{escape_cell(path)}`" for path in paths)
        for kind, paths in sorted(grouped.items())
    )


def write_report(base: Path, output: Path, materials: list[Material]) -> None:
    affected = [(material, classify(material)) for material in materials]
    affected = [(material, status) for material, status in affected if status is not None]
    status_counts = collections.Counter(status for _, status in affected)
    missing_counts = collections.Counter(
        dependency.kind
        for material, _ in affected
        for dependency in material.dependencies
        if not dependency.exists
    )

    lines = [
        "# Материалы с отсутствующими ресурсами",
        "",
        f"Проверено material-объявлений: **{len(materials)}**. "
        f"Найдены отсутствующие зависимости у **{len(affected)}** материалов.",
        "",
        "Путь считается разрешённым, если существует исходное изображение либо его "
        "скомпилированный эквивалент `dds/<path>.dds`. Встроенные изображения движка "
        "без файлового пути в отчёт не включаются. Сравнение путей регистронезависимое, "
        "как в виртуальной файловой системе Doom 3.",
        "",
        "## Сводка по состоянию",
        "",
        "| Состояние | Материалов |",
        "|---|---:|",
    ]
    for status in (
        "полностью неработоспособен",
        "частично неработоспособен",
        "нет preview для редактора",
        "нет authoring-исходника",
    ):
        lines.append(f"| {status} | {status_counts[status]} |")

    lines.extend(
        [
            "",
            "## Сводка по отсутствующим зависимостям",
            "",
            "| Тип зависимости | Отсутствующих ссылок | Причина |",
            "|---|---:|---|",
        ]
    )
    explanations = {
        "runtime image": "Нет TGA/JPG/PNG/DDS и нет соответствующего `dds/…/*.dds`.",
        "editor image": "Нет изображения из `qer_editorimage`; runtime может работать.",
        "cubemap": "Нет одной или нескольких из шести граней cubemap.",
        "video": "Нет файла RoQ.",
        "GUI": "Нет файла GUI, указанного в `guiSurf`.",
        "authoring model": "Нет high-poly модели из `renderbump`; это offline-исходник.",
    }
    for kind in (
        "runtime image",
        "editor image",
        "cubemap",
        "video",
        "GUI",
        "authoring model",
    ):
        lines.append(
            f"| {kind} | {missing_counts[kind]} | {explanations[kind]} |"
        )

    lines.extend(
        [
            "",
            "## Полный список",
            "",
            "| Материал | Объявление | Состояние | Чего не хватает |",
            "|---|---|---|---|",
        ]
    )
    status_order = {
        "полностью неработоспособен": 0,
        "частично неработоспособен": 1,
        "нет preview для редактора": 2,
        "нет authoring-исходника": 3,
    }
    affected.sort(
        key=lambda item: (
            status_order[item[1]],
            item[0].source.name.lower(),
            item[0].line,
        )
    )
    for material, status in affected:
        source = material.source.relative_to(base).as_posix()
        lines.append(
            f"| `{escape_cell(material.name)}` | `{source}:{material.line}` | "
            f"{status} | {compact_missing(material)} |"
        )
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_unused_report(
    base: Path,
    output: Path,
    materials: list[Material],
    references: dict[str, list[str]],
) -> None:
    affected = [(material, classify(material)) for material in materials]
    affected = [(material, status) for material, status in affected if status is not None]
    unreferenced = [
        (material, status)
        for material, status in affected
        if material.name not in references
    ]
    referenced = [
        (material, status)
        for material, status in affected
        if material.name in references
    ]
    status_order = {
        "полностью неработоспособен": 0,
        "частично неработоспособен": 1,
        "нет preview для редактора": 2,
        "нет authoring-исходника": 3,
    }
    removable_statuses = {
        "полностью неработоспособен",
        "частично неработоспособен",
    }
    removable = [
        (material, status)
        for material, status in unreferenced
        if status in removable_statuses
    ]
    runtime_valid = [
        (material, status)
        for material, status in unreferenced
        if status not in removable_statuses
    ]
    removable.sort(
        key=lambda item: (
            status_order[item[1]],
            item[0].source.name.lower(),
            item[0].line,
        )
    )

    unused_counts = collections.Counter(status for _, status in unreferenced)
    used_counts = collections.Counter(status for _, status in referenced)
    lines = [
        "# Неиспользуемые повреждённые материалы",
        "",
        "Это консервативный список кандидатов на удаление. Для указанных material-имён "
        "не найдено точного совпадения ни в одном ресурсе за пределами `.mtr`: "
        "проверены в том числе карты, PROC/CM/AAS, модели, skins, particles, FX, "
        "GUI, def/script и бинарные ресурсы.",
        "",
        "Проверка не может учитывать внешние карты, ещё не добавленные в этот набор, "
        "динамическое составление material-имени в коде/скрипте и ручной выбор "
        "материала пользователем в редакторе.",
        "",
        "## Сводка",
        "",
        "| Состояние | Без ссылок | Есть ссылки — оставить | Рекомендация |",
        "|---|---:|---:|---|",
    ]
    for status in status_order:
        recommendation = (
            "можно удалить"
            if status in removable_statuses
            else "runtime исправен — автоматически не удалять"
        )
        lines.append(
            f"| {status} | {unused_counts[status]} | {used_counts[status]} | "
            f"{recommendation} |"
        )
    lines.extend(
        [
            f"| **Всего** | **{len(unreferenced)}** | **{len(referenced)}** | |",
            "",
            f"Итого безопасных в рамках текущего набора кандидатов: "
            f"**{len(removable)}**. Ещё **{len(runtime_valid)}** материалов без ссылок "
            "имеют исправные runtime-ресурсы и исключены из списка удаления.",
            "",
            "## Можно удалить",
            "",
            "| Материал | Объявление | Состояние | Чего не хватает |",
            "|---|---|---|---|",
        ]
    )
    for material, status in removable:
        source = material.source.relative_to(base).as_posix()
        lines.append(
            f"| `{escape_cell(material.name)}` | `{source}:{material.line}` | "
            f"{status} | {compact_missing(material)} |"
        )
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Audit Doom 3 material dependencies and write a Markdown report."
    )
    parser.add_argument("base", type=Path, help="Path to the game base directory")
    parser.add_argument("output", type=Path, help="Markdown report path")
    parser.add_argument(
        "--unused-report",
        type=Path,
        help="Also scan all non-MTR resources and write unreferenced candidates",
    )
    parser.add_argument(
        "--remove-fully-broken-unreferenced",
        action="store_true",
        help="Remove fully broken materials with no exact references outside MTR files",
    )
    args = parser.parse_args()

    base = args.base.resolve()
    materials = scan(base)
    write_report(base, args.output, materials)
    print(f"Wrote {args.output}: {len(materials)} material declarations checked.")
    if args.unused_report or args.remove_fully_broken_unreferenced:
        affected_names = {
            material.name for material in materials if classify(material) is not None
        }
        references = scan_references(base, affected_names)
        if args.unused_report:
            write_unused_report(base, args.unused_report, materials, references)
            print(
                f"Wrote {args.unused_report}: references to "
                f"{len(references)} affected material names found."
            )
        if args.remove_fully_broken_unreferenced:
            removable = [
                material
                for material in materials
                if classify(material) == "полностью неработоспособен"
                and material.name not in references
            ]
            removed, changed_files = remove_materials(removable)
            print(
                f"Removed {removed} fully broken unreferenced materials "
                f"from {changed_files} files."
            )


if __name__ == "__main__":
    main()
