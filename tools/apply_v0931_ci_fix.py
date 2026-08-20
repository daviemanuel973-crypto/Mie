from pathlib import Path

root = Path(__file__).resolve().parents[1]


def patch(path, replacements):
    file_path = root / path
    text = file_path.read_text(encoding="utf-8")
    for old, new, label in replacements:
        count = text.count(old)
        if count != 1:
            raise RuntimeError(f"{path}: {label}: expected one match, found {count}")
        text = text.replace(old, new, 1)
    file_path.write_text(text, encoding="utf-8")


patch('.github/workflows/linux-flatpak-ci.yml', [
    ('if [[ ! "$MIE_VERSION" =~ ^[0-9]+\\.[0-9]+\\.[0-9]+$ ]]; then',
     'if [[ ! "$MIE_VERSION" =~ ^[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?$ ]]; then',
     'accept fourth hotfix component'),
    ("grep -F 'position += e.second.entity.getColliderOffset();' src/gameLayer/multyPlayer/serverChunkStorer.cpp",
     "grep -F 'entityPosition += e.second.entity.getColliderOffset();' src/gameLayer/multyPlayer/serverChunkStorer.cpp",
     'updated collision guard'),
    ('          ./survivalRulesTests\n\n      - name: Build Flatpak repository',
     '''          ./survivalRulesTests\n\n          c++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra \\\n            -Iinclude/gameLayer \\\n            -Ithirdparty/safeSave/include \\\n            tests/entityIdAllocatorTests.cpp \\\n            src/gameLayer/multyPlayer/entityIdAllocator.cpp \\\n            thirdparty/safeSave/src/safeSave.cpp \\\n            -o entityIdAllocatorTests\n          ./entityIdAllocatorTests\n\n      - name: Build Flatpak repository''',
     'allocator regression test'),
])

patch('.github/workflows/windows-msvc-ci.yml', [
    ("if ($version -notmatch '^\\d+\\.\\d+\\.\\d+$') {",
     "if ($version -notmatch '^\\d+\\.\\d+\\.\\d+(\\.\\d+)?$') {",
     'accept fourth hotfix component'),
    ("|farming-persistence)$'",
     "|farming-persistence|entity-id-allocator)$'",
     'allocator ctest gate'),
])

patch('.github/workflows/publish-windows-release.yml', [
    ('throw "Publish Windows Release must run from an existing vX.Y.Z tag"',
     'throw "Publish Windows Release must run from an existing vX.Y.Z or vX.Y.Z.W tag"',
     'tag guidance'),
    ("if ($version -notmatch '^\\d+\\.\\d+\\.\\d+([+-][0-9A-Za-z.-]+)?$') {",
     "if ($version -notmatch '^\\d+\\.\\d+\\.\\d+(\\.\\d+)?([+-][0-9A-Za-z.-]+)?$') {",
     'release version regex'),
    ("|recipe-book-discovery)$'",
     "|recipe-book-discovery|entity-id-allocator)$'",
     'release allocator ctest gate'),
])

print('v0.9.3.1 CI/release validation patched')
