
#!/usr/bin/env python3

import os
from pathlib import Path

SOURCE_EXTENSIONS = {
    ".cpp", ".hpp", ".h", ".cc", ".c",
    ".cxx", ".hxx", ".hh"
}

SPECIAL_FILES = {
    "CMakeLists.txt",
    "README.md",
    "Makefile"
}

EXCLUDED_DIRS = {
    ".git",
    ".vscode",
    ".idea",
    "__pycache__",
    "build",
    "dist",
    "venv",
    ".venv",
    "node_modules",
}


def should_include(path):
    return (
        path.suffix.lower() in SOURCE_EXTENSIONS
        or path.name in SPECIAL_FILES
    )


def get_files(root):
    files = []

    for current, dirs, filenames in os.walk(root):
        # Prevent entering excluded directories
        dirs[:] = sorted(
            d for d in dirs if d not in EXCLUDED_DIRS
        )

        for filename in sorted(filenames):
            path = Path(current) / filename

            if should_include(path):
                files.append(path)

    return sorted(files)


def get_tree(root, files):
    lines = [root.name + "/"]

    included_dirs = set()
    included_files = set(files)

    for file in files:
        parent = file.parent
        while parent != root:
            included_dirs.add(parent)
            parent = parent.parent

    all_paths = sorted(
        included_dirs | included_files,
        key=lambda p: (str(p.relative_to(root)).lower(),
                       p.is_file())
    )

    for path in all_paths:
        relative = path.relative_to(root)
        depth = len(relative.parts)
        name = relative.name

        suffix = "/" if path.is_dir() else ""
        lines.append("    " * depth + name + suffix)

    return "\n".join(lines)


def get_includes(files, root):
    result = []

    for file in files:
        try:
            content = file.read_text(
                encoding="utf-8",
                errors="replace"
            )
        except OSError:
            continue

        for line_number, line in enumerate(
            content.splitlines(), 1
        ):
            stripped = line.strip()

            if stripped.startswith("#include"):
                relative = file.relative_to(root)
                result.append(
                    f"{relative}:{line_number}: {stripped}"
                )

    return result


def export_project(root):
    files = get_files(root)

    output = root / "project_context.txt"

    with output.open("w", encoding="utf-8") as out:

        out.write("=" * 80 + "\n")
        out.write("C++ PROJECT CONTEXT\n")
        out.write("=" * 80 + "\n\n")

        # 1. Project structure
        out.write("PROJECT STRUCTURE\n")
        out.write("-" * 80 + "\n")
        out.write(get_tree(root, files))
        out.write("\n\n")

        # 2. Include relationships
        out.write("INCLUDE RELATIONSHIPS\n")
        out.write("-" * 80 + "\n")

        includes = get_includes(files, root)

        if includes:
            out.write("\n".join(includes))
        else:
            out.write("No include directives found.")

        out.write("\n\n")

        # 3. File contents
        out.write("SOURCE FILE CONTENTS\n")
        out.write("=" * 80 + "\n")

        for file in files:
            relative = file.relative_to(root)

            out.write("\n\n")
            out.write("=" * 80 + "\n")
            out.write(f"FILE: {relative}\n")
            out.write("=" * 80 + "\n\n")

            try:
                content = file.read_text(
                    encoding="utf-8",
                    errors="replace"
                )
                out.write(content)
            except OSError as error:
                out.write(f"[ERROR READING FILE: {error}]")

            out.write("\n")

    print("\nExport completed!")
    print(f"Files collected: {len(files)}")
    print(f"Output: {output}")


def main():
    print("================================")
    print("       C++ PROJECT EXPORTER")
    print("================================")

    folder = input("\nEnter project folder path: ").strip()
    folder = folder.strip('"').strip("'")

    root = Path(folder).expanduser().resolve()

    if not root.exists():
        print("Error: Folder does not exist.")
        return

    if not root.is_dir():
        print("Error: Path is not a directory.")
        return

    export_project(root)


if __name__ == "__main__":
    main()
