from pathlib import Path

# Trigger the temporary branch-only patch workflow.

def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


path = Path("src/gameLayer/multyPlayer/tick.cpp")
text = path.read_text(encoding="utf-8")

start_marker = "else if (i.t.taskType == Task::clientUsedItem)"
end_marker = "else if (i.t.taskType == Task::clientInteractedWithBlock)"
start = text.find(start_marker)
end = text.find(end_marker, start + len(start_marker))
if start < 0 or end < 0 or end <= start:
    raise SystemExit("could not isolate clientUsedItem handler")
section = text[start:end]

section = replace_once(
    section,
    "\t\t\t\t\t\t\t\t\tallowed = true;\n\n\t\t\t\t\t\t\t\t\t\n\n\t\t\t\t\t\t\t\t\tif (from->counter <= 0) { from = {}; }\n\n\t\t\t\t\t\t\t\t\tif (from->type == i.t.itemType)\n",
    "\t\t\t\t\t\t\t\t\tallowed = mie::serverValidation::isAuthoritativeItemSlotUsable(\n\t\t\t\t\t\t\t\t\t\tfrom->type, from->counter, i.t.itemType);\n\n\t\t\t\t\t\t\t\t\tif (allowed)\n",
    "authoritative slot validation",
)

section = replace_once(
    section,
    "\t\t\t\t\t\t\t\t\t\tspawnPig(chunkCache, p, worldSaver, rng);\n",
    "\t\t\t\t\t\t\t\t\t\tallowed = mie::serverValidation::itemUseRemainsAllowedAfterAction(\n\t\t\t\t\t\t\t\t\t\t\tallowed, spawnPig(chunkCache, p, worldSaver, rng));\n",
    "pig spawn result",
)
section = replace_once(
    section,
    "\t\t\t\t\t\t\t\t\t\tspawnZombie(chunkCache, z, getEntityIdAndIncrement(worldSaver,\n\t\t\t\t\t\t\t\t\t\t\tEntityType::zombies));\n",
    "\t\t\t\t\t\t\t\t\t\tallowed = mie::serverValidation::itemUseRemainsAllowedAfterAction(\n\t\t\t\t\t\t\t\t\t\t\tallowed, spawnZombie(chunkCache, z, getEntityIdAndIncrement(worldSaver,\n\t\t\t\t\t\t\t\t\t\t\t\tEntityType::zombies)));\n",
    "zombie spawn result",
)
section = replace_once(
    section,
    "\t\t\t\t\t\t\t\t\t\tspawnCat(chunkCache, c, worldSaver, rng);\n",
    "\t\t\t\t\t\t\t\t\t\tallowed = mie::serverValidation::itemUseRemainsAllowedAfterAction(\n\t\t\t\t\t\t\t\t\t\t\tallowed, spawnCat(chunkCache, c, worldSaver, rng));\n",
    "cat spawn result",
)
section = replace_once(
    section,
    "\t\t\t\t\t\t\t\t\t\tspawnGoblin(chunkCache, g, worldSaver, rng);\n",
    "\t\t\t\t\t\t\t\t\t\tallowed = mie::serverValidation::itemUseRemainsAllowedAfterAction(\n\t\t\t\t\t\t\t\t\t\t\tallowed, spawnGoblin(chunkCache, g, worldSaver, rng));\n",
    "goblin spawn result",
)
section = replace_once(
    section,
    "\t\t\t\t\t\t\t\t\t\tspawnScareCrow(chunkCache, g, worldSaver, rng);\n",
    "\t\t\t\t\t\t\t\t\t\tallowed = mie::serverValidation::itemUseRemainsAllowedAfterAction(\n\t\t\t\t\t\t\t\t\t\t\tallowed, spawnScareCrow(chunkCache, g, worldSaver, rng));\n",
    "scarecrow spawn result",
)

section = replace_once(
    section,
    "\t\t\t\t\t\tif (shouldUpdateRevisionStuff)\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tcomputeRevisionStuff(*client, allowed, i.t.eventId);\n\t\t\t\t\t\t}\n\n\t\t\t\t\t\t//the client might have eaten something so we update life anyway,\n",
    "\t\t\t\t\t\tif (shouldUpdateRevisionStuff)\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tcomputeRevisionStuff(*client, allowed, i.t.eventId);\n\t\t\t\t\t\t}\n\n\t\t\t\t\t\tif (!allowed)\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tupdatePlayerSurvivalStats(*client);\n\t\t\t\t\t\t}\n\n\t\t\t\t\t\t//the client might have eaten something so we update life anyway,\n",
    "rejected-use survival rollback",
)

text = text[:start] + section + text[end:]
path.write_text(text, encoding="utf-8")
