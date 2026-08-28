#!/usr/bin/env python3
from pathlib import Path

path = Path("src/gameLayer/multyPlayer/tick.cpp")
text = path.read_text(encoding="utf-8")
old = "resyncAndCloseInteraction(*c.second);"
new = "resyncAndCloseInteraction(c.second);"
count = text.count(old)
if count != 1:
    raise RuntimeError(f"client reference fix: expected exactly one match, found {count}")
path.write_text(text.replace(old, new, 1), encoding="utf-8")
print("v0.9.6 Client reference fix applied")
