#!/usr/bin/env python3
from pathlib import Path
import apply_v096_core as base
from apply_v096_core_v2 import patch_tick

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def patch_version_docs() -> None:
    cmake = ROOT / "CMakeLists.txt"
    text = cmake.read_text(encoding="utf-8-sig")
    text = replace_once(
        text,
        'set(OURCRAFT_VERSION "0.9.5" CACHE STRING "Mie Survival release version")',
        'set(OURCRAFT_VERSION "0.9.6" CACHE STRING "Mie Survival release version")',
        "CMake version",
    )
    cmake.write_text(text, encoding="utf-8")

    version = ROOT / "VERSION"
    current = version.read_text(encoding="utf-8-sig").strip()
    if current != "0.9.5":
        raise RuntimeError(f"VERSION expected 0.9.5, got {current!r}")
    version.write_text("0.9.6\n", encoding="utf-8")

    readme = ROOT / "README.md"
    text = readme.read_text(encoding="utf-8-sig")
    text = replace_once(
        text,
        "**Current development version: v0.9.4**",
        "**Current development version: v0.9.6 (release candidate)**",
        "README development version",
    )
    readme.write_text(text, encoding="utf-8")


def main() -> None:
    base.patch_chunk_header()
    base.patch_chunk_cpp()
    patch_tick()
    patch_version_docs()
    print("v0.9.6 guarded core patches applied successfully (v3)")


if __name__ == "__main__":
    main()
