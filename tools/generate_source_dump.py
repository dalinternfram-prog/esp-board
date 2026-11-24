#!/usr/bin/env python3
import datetime
import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "source_dump.txt"

SOURCE_EXTENSIONS = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx",
    ".py", ".sh", ".md", ".cmake",
}

NAME_INCLUDE = {
    "CMakeLists.txt", "partitions.csv", "sdkconfig.defaults",
    "dependencies.lock", ".gitignore", "idf_component.yml",
    "Kconfig.projbuild", "README", "Readme",
}

TREE_ONLY = {".clangd"}

EXCLUDED_DIRS = {
    ".git", ".vscode", ".idea", "__pycache__", "build", "dist",
    "venv", ".venv", "node_modules", "managed_components", ".cache",
    "esp_bus", "esp_wifi_manager", "opus",
}

EXCLUDED_PATHS = {
    "sdkconfig", "sdkconfig.old",
    "main/general/kcp/ikcp.c", "main/general/kcp/ikcp.h",
    "tools/generate_source_dump.py",
}

C_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
HASH_EXTENSIONS = {".sh", ".cmake"}
HASH_NAMES = {
    "CMakeLists.txt", "partitions.csv", "sdkconfig.defaults",
    ".gitignore", "idf_component.yml", "Kconfig.projbuild",
}
MARKDOWN_EXTENSIONS = {".md"}
MARKDOWN_NAMES = {"README", "Readme"}
PASSTHROUGH_NAMES = {"dependencies.lock"}

SEP = "=" * 62
SEP2 = "-" * 62


def collect_files():
    files = set()
    tree_only = set()

    def walk(directory, prefix):
        for current, dirs, filenames in os.walk(directory):
            dirs[:] = sorted(d for d in dirs if d not in EXCLUDED_DIRS)
            for filename in sorted(filenames):
                path = Path(current) / filename
                rel = (prefix / path.relative_to(directory)).as_posix()
                if rel in EXCLUDED_PATHS:
                    continue
                if rel in TREE_ONLY:
                    tree_only.add(rel)
                    continue
                if path.suffix.lower() in SOURCE_EXTENSIONS or filename in NAME_INCLUDE:
                    files.add(rel)

    walk(ROOT, Path("."))
    proto = ROOT / "components" / "proto"
    if proto.is_symlink():
        walk(proto, Path("components") / "proto")

    key = lambda p: p.lower()
    return sorted(files, key=key), sorted(tree_only, key=key)


def strip_c_style(text):
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            q = c
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == q:
                    j += 1
                    break
                j += 1
            out.append(text[i:j])
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "/":
            nl = text.find("\n", i + 2)
            if nl == -1:
                i = n
            else:
                out.append("\n")
                i = nl + 1
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            end = text.find("*/", i + 2)
            i = n if end == -1 else end + 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def shebang_split(text):
    pos = 0
    while pos < len(text):
        nl = text.find("\n", pos)
        line_end = len(text) if nl == -1 else nl
        line = text[pos:line_end]
        if line.strip():
            if line.lstrip().startswith("#!"):
                cut = line_end + (0 if nl == -1 else 1)
                return text[:cut], text[cut:]
            return "", text
        pos = line_end + 1
    return "", text


def strip_hash(text, keep_shebang=True):
    head, rest = text, ""
    if keep_shebang:
        head, rest = shebang_split(text)
    out = []
    i, n = 0, len(rest)
    line_start = True
    while i < n:
        c = rest[i]
        if c == '"' or c == "'":
            q = c
            j = i + 1
            while j < n:
                if rest[j] == "\\":
                    j += 2
                    continue
                if rest[j] == q or rest[j] == "\n":
                    j += 1
                    break
                j += 1
            out.append(rest[i:j])
            i = j
            line_start = False
            continue
        if c == "#" and (line_start or (out and out[-1] in " \t")):
            nl = rest.find("\n", i)
            if nl == -1:
                i = n
            else:
                out.append("\n")
                i = nl + 1
                line_start = True
            continue
        line_start = c == "\n"
        out.append(c)
        i += 1
    return head + "".join(out)


def strip_python(text):
    head, rest = shebang_split(text)
    out = []
    i, n = 0, len(rest)
    while i < n:
        if rest.startswith('"""', i) or rest.startswith("'''", i):
            q = rest[i : i + 3]
            end = rest.find(q, i + 3)
            j = n if end == -1 else end + 3
            out.append(rest[i:j])
            i = j
            continue
        c = rest[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n:
                if rest[j] == "\\":
                    j += 2
                    continue
                if rest[j] == c or rest[j] == "\n":
                    j += 1
                    break
                j += 1
            out.append(rest[i:j])
            i = j
            continue
        if c == "#":
            nl2 = rest.find("\n", i)
            if nl2 == -1:
                i = n
            else:
                out.append("\n")
                i = nl2 + 1
            continue
        out.append(c)
        i += 1
    return head + "".join(out)



def strip_markdown(text):
    return re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL)


def strip_for_file(rel, text):
    name = rel.rsplit("/", 1)[-1]
    ext = Path(rel).suffix.lower()
    if ext in C_EXTENSIONS:
        return strip_c_style(text)
    if name in PASSTHROUGH_NAMES:
        return text
    if ext in MARKDOWN_EXTENSIONS or name in MARKDOWN_NAMES:
        return strip_markdown(text)
    if ext == ".py":
        return strip_python(text)
    if ext in HASH_EXTENSIONS or name in HASH_NAMES:
        return strip_hash(text)
    return text


def finalize(text, keep_blank=False):
    lines = [ln.rstrip() for ln in text.splitlines()]
    if not keep_blank:
        lines = [ln for ln in lines if ln.strip()]
    return "\n".join(lines)


def git_info():
    try:
        head = subprocess.check_output(
            ["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL, text=True).strip()
        subject = subprocess.check_output(
            ["git", "-C", str(ROOT), "log", "-1", "--format=%s"],
            stderr=subprocess.DEVNULL, text=True).strip()
        status = subprocess.check_output(
            ["git", "-C", str(ROOT), "status", "--porcelain"],
            stderr=subprocess.DEVNULL, text=True)
        dirty = any(
            not line.split(None, 1)[-1].startswith(".cache/")
            for line in status.splitlines()
            if line.strip()
        )
        label = f"{head} ({subject})"
        if dirty:
            label += " (dirty)"
        return label
    except Exception:
        return "unknown"


def build_dump():
    files, tree_only = collect_files()
    parts = []

    parts += [
        SEP,
        "PROJECT SOURCE DUMP (first-party code only, 3rd-party excluded)",
        f"Project: {ROOT.parent.parent.name} / {ROOT.parent.name} / {ROOT.name}",
        f"Generated: {datetime.datetime.now().astimezone().strftime('%Y-%m-%d %H:%M:%S %Z')}",
        f"Git HEAD: {git_info()}",
        SEP,
        "",
        "NOTE: Excluded third-party / generated content:",
        "  - managed_components/ (all registry components)",
        "  - build/ (generated build artifacts)",
        "  - components/esp_bus, esp_wifi_manager, opus (3rd-party libs)",
        "  - sdkconfig, sdkconfig.old (generated)",
        "  - main/general/kcp/ikcp.c & ikcp.h (vendored KCP lib, skywind3000)",
        "  - tools/generate_source_dump.py (this dump generator)",
        "  - .clangd (content omitted; tree listing only)",
        "",
        "NOTE: All comments are stripped from file contents (token-saving dump).",
        "",
    ]

    proto = ROOT / "components" / "proto"
    target = os.readlink(proto) if proto.is_symlink() else ""
    parts.append(
        f"INCLUDED: components/proto (symlink -> {target}, "
        "first-party Protocol library)"
    )
    parts += ["", SEP2, "FILE TREE", SEP2]

    tree_entries = tree_only + [f for f in files if f != "dependencies.lock"]
    for entry in sorted(tree_entries, key=lambda p: p.lower()):
        parts.append(entry)

    parts += [
        "",
        "... (dependencies.lock excluded from tree for brevity, see file below)",
        "",
        SEP,
        "SOURCE FILES",
        SEP,
        "",
    ]

    for rel in files:
        try:
            content = (ROOT / rel).read_text(
                encoding="utf-8", errors="replace")
        except OSError as error:
            parts += [SEP2, f"FILE: {rel}", SEP2, "",
                      f"[ERROR READING FILE: {error}]"]
            continue
        name = rel.rsplit("/", 1)[-1]
        ext = Path(rel).suffix.lower()
        keep_blank = ext in MARKDOWN_EXTENSIONS or name in MARKDOWN_NAMES
        body = finalize(strip_for_file(rel, content), keep_blank=keep_blank)
        parts += [SEP2, f"FILE: {rel}", SEP2, ""]
        if body:
            parts.append(body)

    text = "\n".join(parts) + "\n"
    old_size = OUTPUT.stat().st_size if OUTPUT.exists() else 0
    OUTPUT.write_text(text, encoding="utf-8")
    print(f"Files dumped: {len(files)} (+{len(tree_only)} tree-only)")
    print(f"Output: {OUTPUT}")
    print(f"Size: {old_size} -> {OUTPUT.stat().st_size} bytes")
    print(f"Estimated tokens (~4 chars/token): "
          f"{OUTPUT.stat().st_size // 4}")


if __name__ == "__main__":
    build_dump()

