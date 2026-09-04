from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


tick_path = Path("src/gameLayer/multyPlayer/tick.cpp")
tick = tick_path.read_text(encoding="utf-8")

tick = replace_once(
    tick,
    "#include <multyPlayer/dataIntegrity.h>\n",
    "#include <multyPlayer/dataIntegrity.h>\n#include <multyPlayer/actionResync.h>\n",
    "actionResync include",
)

start_marker = "else if (i.t.taskType == Task::droppedItemEntity)"
end_marker = "else if (i.t.taskType == Task::clientMovedItem)"
start = tick.find(start_marker)
end = tick.find(end_marker, start + len(start_marker))
if start < 0 or end < 0 or end <= start:
    raise SystemExit("could not isolate dropped-item task handler")

section = tick[start:end]
section = replace_once(
    section,
    "if (client->playerData.inventory.revisionNumber\n\t\t\t\t\t\t\t== i.t.revisionNumber\n\t\t\t\t\t\t\t)\n",
    "if (!mie::network::droppedItemRevisionRequiresInventoryResync(\n\t\t\t\t\t\t\tclient->playerData.inventory.revisionNumber, i.t.revisionNumber))\n",
    "dropped-item revision decision",
)

section = replace_once(
    section,
    "\t\t\t\t\t\t\tkillItem = true;\n\t\t\t\t\t\t}\n\n\t\t\t\t\t\tif (killItem)\n",
    "\t\t\t\t\t\t\tsendPlayerInventoryAndIncrementRevision(*client);\n\t\t\t\t\t\t\tkillItem = true;\n\t\t\t\t\t\t}\n\n\t\t\t\t\t\tif (killItem)\n",
    "stale revision rollback",
)

tick = tick[:start] + section + tick[end:]
tick_path.write_text(tick, encoding="utf-8")

todo_path = Path("hardertodos.md")
todos = todo_path.read_text(encoding="utf-8")
todos = replace_once(
    todos,
    "- todo: start working at item dropping + survival mode, don't forget that rejecting a dropped item should recreate inventory\n",
    "",
    "resolved dropped-item todo",
)
todo_path.write_text(todos, encoding="utf-8")
