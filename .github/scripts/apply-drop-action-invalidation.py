from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


path = Path("src/gameLayer/multyPlayer/tick.cpp")
text = path.read_text(encoding="utf-8")

start_marker = "else if (i.t.taskType == Task::droppedItemEntity)"
end_marker = "else if (i.t.taskType == Task::clientMovedItem)"
start = text.find(start_marker)
end = text.find(end_marker, start + len(start_marker))
if start < 0 or end < 0 or end <= start:
    raise SystemExit("could not isolate dropped-item task handler")

section = text[start:end]
section = replace_once(
    section,
    "mie::network::droppedItemRevisionRequiresInventoryResync(",
    "mie::network::droppedItemRevisionIsStale(",
    "stale dropped-item revision policy",
)

inventory_resync = "\t\t\t\t\t\t\tsendPlayerInventoryAndIncrementRevision(*client);"
last_resync = section.rfind(inventory_resync)
if last_resync < 0:
    raise SystemExit("could not find stale-revision inventory resync")
if section.find(inventory_resync, last_resync + 1) >= 0:
    raise SystemExit("unexpected inventory resync after stale-revision branch")

invalidation = "\t\t\t\t\t\t\tcomputeRevisionStuff(*client, false, i.t.eventId);\n"
if invalidation.strip() in section:
    raise SystemExit("prediction invalidation already present")
section = section[:last_resync] + invalidation + section[last_resync:]

text = text[:start] + section + text[end:]
path.write_text(text, encoding="utf-8")
