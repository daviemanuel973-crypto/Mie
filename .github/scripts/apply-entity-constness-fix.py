from pathlib import Path

path = Path("src/gameLayer/rendering/renderer.cpp")
text = path.read_text(encoding="utf-8")
old = "auto entityChunkReadyForRendering = [&](const auto &entry)"
new = "auto entityChunkReadyForRendering = [&](auto &entry)"

count = text.count(old)
if count != 1:
    raise SystemExit(f"expected exactly one renderer helper signature, found {count}")

path.write_text(text.replace(old, new, 1), encoding="utf-8")
