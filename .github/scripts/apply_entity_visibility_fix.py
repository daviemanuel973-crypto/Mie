from pathlib import Path

renderer_path = Path("src/gameLayer/rendering/renderer.cpp")
text = renderer_path.read_text(encoding="utf-8")

old = "\tstd::minstd_rand rng{std::random_device()()};\n\n\tauto renderAllEntitiesOfOneType = [&](Model &model, auto &container, bool isPlayers = 0,\n\t\tGLuint64 playerTextureOverride = 0)\n\t{\n"
new = "\tstd::minstd_rand rng{std::random_device()()};\n\n\tauto entityChunkReadyForRendering = [&](const auto &entry)\n\t{\n\t\tglm::ivec3 blockPos = {};\n\t\tif constexpr (hasPositionBasedID<decltype(entry.second.entityBuffered)>)\n\t\t{\n\t\t\tblockPos = fromEntityIDToBlockPos(entry.first);\n\t\t}\n\t\telse\n\t\t{\n\t\t\tblockPos = from3DPointToBlock(entry.second.getRubberBandPosition());\n\t\t}\n\n\t\tChunk *chunk = chunkSystem.getChunkSafeFromChunkPos(\n\t\t\tdivideChunk(blockPos.x), divideChunk(blockPos.z));\n\t\treturn chunk && !chunk->isDontDrawYet();\n\t};\n\n\tauto renderAllEntitiesOfOneType = [&](Model &model, auto &container, bool isPlayers = 0,\n\t\tGLuint64 playerTextureOverride = 0, bool requireReadyChunk = true)\n\t{\n"
if old not in text:
    raise SystemExit("renderer anchor 1 not found")
text = text.replace(old, new, 1)

old = "\t\tfor (auto &e : container)\n\t\t{\n\t\t\tPerEntityData data = {};\n"
new = "\t\tfor (auto &e : container)\n\t\t{\n\t\t\tif (requireReadyChunk && !entityChunkReadyForRendering(e)) { continue; }\n\n\t\t\tPerEntityData data = {};\n"
if old not in text:
    raise SystemExit("renderer anchor 2 not found")
text = text.replace(old, new, 1)

old = "\trenderAllEntitiesOfOneType(modelsManager.human, entityManager.localPlayersForRendering,\n\t\ttrue, currentSkinBindlessTexture);\n"
new = "\trenderAllEntitiesOfOneType(modelsManager.human, entityManager.localPlayersForRendering,\n\t\ttrue, currentSkinBindlessTexture, false);\n"
if old not in text:
    raise SystemExit("renderer anchor 3 not found")
text = text.replace(old, new, 1)

old = "\t\t\tfor (auto &e : entityManager.droppedItems)\n\t\t\t{\n\t\t\t\tif (!isBlock(e.second.entityBuffered.type)) { continue; }\n"
new = "\t\t\tfor (auto &e : entityManager.droppedItems)\n\t\t\t{\n\t\t\t\tif (!entityChunkReadyForRendering(e)) { continue; }\n\t\t\t\tif (!isBlock(e.second.entityBuffered.type)) { continue; }\n"
if old not in text:
    raise SystemExit("renderer anchor 4 not found")
text = text.replace(old, new, 1)

old = "\t\tfor (auto &e : entityManager.droppedItems)\n\t\t{\n\t\t\t//continue;\n\t\t\tif (isBlock(e.second.entityBuffered.type)) { continue; }\n"
new = "\t\tfor (auto &e : entityManager.droppedItems)\n\t\t{\n\t\t\t//continue;\n\t\t\tif (!entityChunkReadyForRendering(e)) { continue; }\n\t\t\tif (isBlock(e.second.entityBuffered.type)) { continue; }\n"
if old not in text:
    raise SystemExit("renderer anchor 5 not found")
text = text.replace(old, new, 1)

renderer_path.write_text(text, encoding="utf-8")

todo_path = Path("todo.txt")
todo = todo_path.read_text(encoding="utf-8")
todo = todo.replace("entities of not visible yet chunks should not be visible to the player\n\n", "", 1)
todo = todo.replace("changing the view distance should not reset all the chunks\n\n\n", "", 1)
todo_path.write_text(todo, encoding="utf-8")

harder_path = Path("hardertodos.md")
harder = harder_path.read_text(encoding="utf-8")
harder = harder.replace("- todo: fix camera move when exiting inventory\n", "", 1)
harder_path.write_text(harder, encoding="utf-8")

Path(".github/workflows/apply-entity-visibility-fix.yml").unlink()
Path(".github/scripts/apply_entity_visibility_fix.py").unlink()
