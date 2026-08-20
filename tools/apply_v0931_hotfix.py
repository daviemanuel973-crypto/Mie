from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    return (ROOT / path).read_text(encoding="utf-8")


def save(path, text):
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def replace_region(text, start_marker, end_marker, replacement, label):
    start = text.find(start_marker)
    if start < 0:
        raise RuntimeError(f"{label}: start marker not found")
    end = text.find(end_marker, start)
    if end < 0:
        raise RuntimeError(f"{label}: end marker not found")
    return text[:start] + replacement.rstrip() + "\n\n" + text[end:]


# ---- Small shared integrity helpers -------------------------------------------------
path = "include/gameLayer/multyPlayer/dataIntegrity.h"
text = load(path)
old = "\tinline bool isDroppedItemSpawnPositionValid(const glm::dvec3 &playerPosition,\n\t\tconst glm::dvec3 &dropPosition)\n\t{\n\t\tif (!isFiniteWorldPosition(playerPosition) || !isFiniteWorldPosition(dropPosition)) { return false; }\n\n\t\tconst glm::dvec3 delta = dropPosition - playerPosition;\n\t\treturn glm::dot(delta, delta) <= MaximumDroppedItemDistance * MaximumDroppedItemDistance;\n\t}\n"
new = old + "\n\tstruct BlockPlacementCollisionQuery\n\t{\n\t\tglm::dvec3 entityPosition = {};\n\t\tglm::vec3 colliderSize = {};\n\t\tglm::ivec3 blockPosition = {};\n\t};\n\n\tinline BlockPlacementCollisionQuery makeBlockPlacementCollisionQuery(\n\t\tconst glm::dvec3 &entityPosition, const glm::vec3 &colliderSize,\n\t\tconst glm::ivec3 &blockPosition)\n\t{\n\t\treturn {entityPosition, colliderSize, blockPosition};\n\t}\n"
text = replace_once(text, old, new, "dataIntegrity helper")
save(path, text)


# ---- WorldSaver sidecar API now reports write success -------------------------------
path = "include/gameLayer/multyPlayer/chunkSaver.h"
text = load(path)
text = replace_once(text, "\tvoid saveChunkBlockData(SavedChunk &c);", "\tbool saveChunkBlockData(SavedChunk &c);", "chunkSaver block-data return")
text = replace_once(text, "\tvoid saveEntitiesForChunk(SavedChunk &c);", "\tbool saveEntitiesForChunk(SavedChunk &c);", "chunkSaver entity return")
save(path, text)


# ---- Transactional block/entity sidecars + recovery ---------------------------------
path = "src/gameLayer/multyPlayer/chunkSaver.cpp"
text = load(path)
insert_marker = "//todo if the loading chunk fails we should not load the entities there and rather delete those files if exist!!"
helpers = r'''namespace
{
	std::string sidecarBackupPath(const std::string &fileName)
	{
		return fileName + ".bak";
	}

	std::string sidecarTempPath(const std::string &fileName)
	{
		return fileName + ".tmp";
	}

	bool removeSidecarAndBackup(const std::string &fileName)
	{
		bool success = true;
		for (const std::string candidate : {fileName, sidecarBackupPath(fileName), sidecarTempPath(fileName)})
		{
			std::error_code error;
			std::filesystem::remove(candidate, error);
			if (error) { success = false; }
		}
		return success;
	}

	bool promoteSidecarTempFile(const std::string &tempFile, const std::string &fileName)
	{
		const std::string backupFile = sidecarBackupPath(fileName);
		std::error_code error;
		std::filesystem::remove(backupFile, error);
		error.clear();

		const bool hadPrimary = std::filesystem::exists(fileName, error) && !error;
		error.clear();
		if (hadPrimary)
		{
			std::filesystem::rename(fileName, backupFile, error);
			if (error)
			{
				std::filesystem::remove(tempFile, error);
				return false;
			}
		}

		std::filesystem::rename(tempFile, fileName, error);
		if (!error) { return true; }

		std::error_code cleanupError;
		std::filesystem::remove(tempFile, cleanupError);
		if (hadPrimary && !std::filesystem::exists(fileName, cleanupError))
		{
			cleanupError.clear();
			std::filesystem::rename(backupFile, fileName, cleanupError);
		}
		return false;
	}

	bool writeSidecarAtomically(const std::string &fileName,
		const unsigned char *data, std::size_t size)
	{
		const std::string tempFile = sidecarTempPath(fileName);
		{
			std::ofstream file(tempFile, std::ios::binary | std::ios::trunc);
			if (!file.is_open()) { return false; }
			if (size) { file.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size)); }
			file.flush();
			if (!file.good())
			{
				file.close();
				std::error_code error;
				std::filesystem::remove(tempFile, error);
				return false;
			}
		}
		return promoteSidecarTempFile(tempFile, fileName);
	}

	enum class EntitySidecarLoadResult
	{
		missing,
		loaded,
		corrupt,
	};

	EntitySidecarLoadResult loadEntitySidecarCandidate(const std::string &fileName,
		EntityData &entityData)
	{
		std::ifstream f(fileName, std::ios::binary);
		if (!f.is_open()) { return EntitySidecarLoadResult::missing; }

		for (;;)
		{
			if (f.peek() == std::ifstream::traits_type::eof())
			{
				return EntitySidecarLoadResult::loaded;
			}

			Marker marker = 0;
			if (!readMarker(f, marker) || marker == 0)
			{
				return EntitySidecarLoadResult::corrupt;
			}

			std::uint64_t eid = 0;
			if (!readEntityId(f, eid) || eid == 0)
			{
				return EntitySidecarLoadResult::corrupt;
			}

			bool success = false;
			switch (marker)
			{
			case Markers::droppedItem:
			{
				DroppedItemServer item;
				success = getEntityTypeFromEID(eid) == EntityType::droppedItems &&
					item.loadFromDisk(f) && entityData.droppedItems.insert({eid, item}).second;
			}
			break;
			case Markers::zombie:
			{
				ZombieServer zombie;
				if (getEntityTypeFromEID(eid) == EntityType::zombies && zombie.loadFromDisk(f))
				{
					success = entityData.zombies.insert({eid, std::move(zombie)}).second;
				}
			}
			break;
			case Markers::pig:
			{
				PigServer pig;
				success = getEntityTypeFromEID(eid) == EntityType::pigs && pig.loadFromDisk(f) &&
					entityData.pigs.insert({eid, pig}).second;
			}
			break;
			case Markers::cat:
			{
				CatServer cat;
				success = getEntityTypeFromEID(eid) == EntityType::cats && cat.loadFromDisk(f) &&
					entityData.cats.insert({eid, cat}).second;
			}
			break;
			case Markers::goblin:
			{
				GoblinServer goblin;
				success = getEntityTypeFromEID(eid) == EntityType::goblins && goblin.loadFromDisk(f) &&
					entityData.goblins.insert({eid, goblin}).second;
			}
			break;
			case Markers::scareCrow:
			{
				ScareCrowServer scareCrow;
				success = getEntityTypeFromEID(eid) == EntityType::scareCrow && scareCrow.loadFromDisk(f) &&
					entityData.scareCrows.insert({eid, scareCrow}).second;
			}
			break;
			default:
				return EntitySidecarLoadResult::corrupt;
			}

			if (!success) { return EntitySidecarLoadResult::corrupt; }
			reserveEntityId(eid);
		}
	}
}

'''
if insert_marker not in text:
    raise RuntimeError("chunkSaver helpers: marker missing")
text = text.replace(insert_marker, helpers + insert_marker, 1)

text = replace_region(text,
    "void WorldSaver::saveChunkBlockData(SavedChunk &c)",
    "bool fileIsEmpty(std::ifstream &f)",
    r'''bool WorldSaver::saveChunkBlockData(SavedChunk &c)
{
	const glm::ivec2 pos = {c.chunk.x, c.chunk.z};
	std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".block";

	std::vector<unsigned char> data;
	c.blockData.formatBlockData(data, pos.x, pos.y);
	if (data.empty())
	{
		return removeSidecarAndBackup(fileName);
	}

	if (!writeSidecarAtomically(fileName, data.data(), data.size()))
	{
		std::cout << "Server error saving block sidecar transactionally: " << fileName << "\n";
		return false;
	}
	return true;
}''', "saveChunkBlockData")

text = replace_region(text,
    "void WorldSaver::loadEntityData(EntityData &entityData,",
    "void WorldSaver::loadBlockData(SavedChunk &c)",
    r'''void WorldSaver::loadEntityData(EntityData &entityData,
	glm::ivec2 chunkPosition)
{
	const glm::ivec2 pos = chunkPosition;
	const std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".entity";

	EntityData loaded;
	const auto primaryResult = loadEntitySidecarCandidate(fileName, loaded);
	if (primaryResult == EntitySidecarLoadResult::loaded)
	{
		entityData = std::move(loaded);
		return;
	}

	loaded = {};
	const auto backupResult = loadEntitySidecarCandidate(sidecarBackupPath(fileName), loaded);
	if (backupResult == EntitySidecarLoadResult::loaded)
	{
		entityData = std::move(loaded);
		std::cout << "Server warning: recovered entity sidecar from backup for chunk "
			<< pos.x << ',' << pos.y << ".\n";
		return;
	}

	if (primaryResult == EntitySidecarLoadResult::corrupt ||
		backupResult == EntitySidecarLoadResult::corrupt)
	{
		std::cout << "Server warning: entity sidecar corrupted for chunk "
			<< pos.x << ',' << pos.y << "; starting that chunk with no persisted entities.\n";
	}
	entityData = {};
}''', "loadEntityData")

text = replace_region(text,
    "void WorldSaver::loadBlockData(SavedChunk &c)",
    "void WorldSaver::saveEntitiesForChunk(SavedChunk &c)",
    r'''void WorldSaver::loadBlockData(SavedChunk &c)
{
	const glm::ivec2 pos = {c.chunk.x, c.chunk.z};
	const std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".block";

	auto tryLoad = [&](const std::string &candidate) -> bool
	{
		std::vector<unsigned char> data;
		if (sfs::readEntireFile(data, candidate.c_str()) != sfs::noError || data.empty())
		{
			return false;
		}
		BlocksWithDataHolder loaded;
		if (!loaded.loadBlockData(data, pos.x, pos.y)) { return false; }
		c.blockData = std::move(loaded);
		return true;
	};

	if (tryLoad(fileName)) { return; }
	if (tryLoad(sidecarBackupPath(fileName)))
	{
		std::cout << "Server warning: recovered block-data sidecar from backup for chunk "
			<< pos.x << ',' << pos.y << ".\n";
		return;
	}
	c.blockData = {};
}''', "loadBlockData")

text = replace_region(text,
    "void WorldSaver::saveEntitiesForChunk(SavedChunk &c)",
    "//todo\nvoid WorldSaver::appendEntitiesForChunk(glm::ivec2 chunkPos)",
    r'''bool WorldSaver::saveEntitiesForChunk(SavedChunk &c)
{
	const glm::ivec2 pos = {c.chunk.x, c.chunk.z};
	const std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".entity";
	const std::string tempFile = sidecarTempPath(fileName);

	std::ofstream f(tempFile, std::ios::binary | std::ios::trunc);
	if (!f.is_open()) { return false; }
	saveAllEntitiesIntoOpenFile(f, c.entityData);
	const std::streampos written = f.tellp();
	f.flush();
	const bool writeSucceeded = f.good();
	f.close();

	if (!writeSucceeded || written < std::streampos(0))
	{
		std::error_code error;
		std::filesystem::remove(tempFile, error);
		return false;
	}
	if (written == std::streampos(0))
	{
		std::error_code error;
		std::filesystem::remove(tempFile, error);
		return removeSidecarAndBackup(fileName);
	}
	if (!promoteSidecarTempFile(tempFile, fileName))
	{
		std::cout << "Server error saving entity sidecar transactionally: " << fileName << "\n";
		return false;
	}
	return true;
}''', "saveEntitiesForChunk")
save(path, text)


# ---- Chunk lifecycle: load the correct entity sidecar, preserve dirty state ----------
path = "src/gameLayer/multyPlayer/serverChunkStorer.cpp"
text = load(path)
text = replace_once(text, "#include <gameplay/lootTables.h>\n", "#include <gameplay/lootTables.h>\n#include <multyPlayer/dataIntegrity.h>\n", "serverChunkStorer include")
text = replace_once(text,
    "\t\tworldSaver.loadBlockData(*rez);\n\t\tif (rez->normalize())",
    "\t\tworldSaver.loadBlockData(*rez);\n\t\tworldSaver.loadEntityData(rez->entityData, pos);\n\t\tindexEntityChunkPositionsForChunk(*rez);\n\t\tif (rez->normalize())",
    "load entities with persisted chunk")
old_region = '''#pragma region load entities

	for (auto &c : newCreatedChunks)
	{
		worldSaver.loadEntityData(c.second->entityData, c.first);
		indexEntityChunkPositionsForChunk(*c.second);
	}

#pragma endregion'''
new_region = '''#pragma region load entities

	// Persisted entity sidecars are restored only when their chunk geometry was
	// successfully restored above. Freshly generated chunks must never revive a
	// stale .entity file left behind by a missing/corrupt chunk.

#pragma endregion'''
text = replace_once(text, old_region, new_region, "remove generated-chunk entity load")

old_collision = '''		for (auto &e : container)
		{
			glm::dvec3 position = e.second.getPosition();

			if constexpr (hasGetColliderOffset<decltype(e.second.entity)>) { position += e.second.entity.getColliderOffset(); }

			auto rez = boxColideBlock(position, e.second.entity.getColliderSize(), position);
			
			if (rez)'''
new_collision = '''		for (auto &e : container)
		{
			glm::dvec3 entityPosition = e.second.getPosition();

			if constexpr (hasGetColliderOffset<decltype(e.second.entity)>) { entityPosition += e.second.entity.getColliderOffset(); }

			const auto query = mie::dataIntegrity::makeBlockPlacementCollisionQuery(
				entityPosition, e.second.entity.getColliderSize(), position);
			auto rez = boxColideBlock(query.entityPosition, query.colliderSize, query.blockPosition);
			
			if (rez)'''
text = replace_once(text, old_collision, new_collision, "placed-block collision shadow")

text = replace_region(text,
    "bool ServerChunkStorer::saveNextChunk(WorldSaver &worldSaver, int count, int entitySaver)",
    "void ServerChunkStorer::saveChunk(WorldSaver &worldSaver, SavedChunk *savedChunks)",
    r'''bool ServerChunkStorer::saveNextChunk(WorldSaver &worldSaver, int count, int entitySaver)
{
	int blockDataCounter = count + 1;
	bool succeeded = false;

	for (auto &entry : savedChunks)
	{
		SavedChunk *chunk = entry.second;
		if (!chunk) { continue; }

		if (chunk->otherData.dirty && count > 0)
		{
			saveChunk(worldSaver, chunk);
			if (!chunk->otherData.dirty)
			{
				succeeded = true;
				--count;
			}
		}

		if (chunk->otherData.dirtyEntity && entitySaver > 0)
		{
			if (worldSaver.saveEntitiesForChunk(*chunk))
			{
				chunk->otherData.dirtyEntity = false;
				succeeded = true;
				--entitySaver;
			}
		}

		if (chunk->otherData.dirtyBlockData && blockDataCounter > 0)
		{
			if (worldSaver.saveChunkBlockData(*chunk))
			{
				chunk->otherData.dirtyBlockData = false;
				succeeded = true;
				--blockDataCounter;
			}
		}

		if (count <= 0 && entitySaver <= 0 && blockDataCounter <= 0) { break; }
	}

	return succeeded;
}''', "saveNextChunk")

text = replace_region(text,
    "void ServerChunkStorer::saveChunk(WorldSaver &worldSaver, SavedChunk *savedChunks)",
    "void ServerChunkStorer::saveChunkBlockData(WorldSaver &worldSaver, SavedChunk *savedChunks)",
    r'''void ServerChunkStorer::saveChunk(WorldSaver &worldSaver, SavedChunk *savedChunk)
{
	const bool savedChunkData = worldSaver.saveChunk(savedChunk->chunk);
	const bool savedEntities = worldSaver.saveEntitiesForChunk(*savedChunk);
	if (savedChunkData) { savedChunk->otherData.dirty = false; }
	if (savedEntities) { savedChunk->otherData.dirtyEntity = false; }
	else { savedChunk->otherData.dirtyEntity = true; }
}''', "saveChunk")

text = replace_region(text,
    "void ServerChunkStorer::saveChunkBlockData(WorldSaver &worldSaver, SavedChunk *savedChunks)",
    "void ServerChunkStorer::saveAllChunks(WorldSaver &worldSaver)",
    r'''void ServerChunkStorer::saveChunkBlockData(WorldSaver &worldSaver, SavedChunk *savedChunk)
{
	if (worldSaver.saveChunkBlockData(*savedChunk))
	{
		savedChunk->otherData.dirtyBlockData = false;
	}
}''', "saveChunkBlockData wrapper")

text = replace_region(text,
    "void ServerChunkStorer::saveAllChunks(WorldSaver &worldSaver)",
    "int ServerChunkStorer::unloadChunksThatNeedUnloading(WorldSaver &worldSaver, int count)",
    r'''void ServerChunkStorer::saveAllChunks(WorldSaver &worldSaver)
{
	for (auto &entry : savedChunks)
	{
		SavedChunk *chunk = entry.second;
		if (!chunk) { continue; }
		if (chunk->otherData.dirty) { saveChunk(worldSaver, chunk); }
		else if (worldSaver.saveEntitiesForChunk(*chunk)) { chunk->otherData.dirtyEntity = false; }
		else { chunk->otherData.dirtyEntity = true; }

		if (chunk->otherData.dirtyBlockData) { saveChunkBlockData(worldSaver, chunk); }
	}
}''', "saveAllChunks")

text = replace_region(text,
    "int ServerChunkStorer::unloadChunksThatNeedUnloading(WorldSaver &worldSaver, int count)",
    "bool ServerChunkStorer::entityAlreadyExists(std::uint64_t eid)",
    r'''int ServerChunkStorer::unloadChunksThatNeedUnloading(WorldSaver &worldSaver, int count)
{
	int unloaded = 0;
	for (auto it = savedChunks.begin(); it != savedChunks.end(); )
	{
		auto &entry = *it;
		SavedChunk *chunk = entry.second;
		if (!chunk || !chunk->otherData.shouldUnload)
		{
			++it;
			continue;
		}

		if (chunk->otherData.dirty)
		{
			saveChunk(worldSaver, chunk);
		}
		else
		{
			// Entity motion/state can change even when block data did not.
			if (worldSaver.saveEntitiesForChunk(*chunk)) { chunk->otherData.dirtyEntity = false; }
			else { chunk->otherData.dirtyEntity = true; }
		}

		if (chunk->otherData.dirtyEntity)
		{
			if (worldSaver.saveEntitiesForChunk(*chunk)) { chunk->otherData.dirtyEntity = false; }
		}
		if (chunk->otherData.dirtyBlockData)
		{
			saveChunkBlockData(worldSaver, chunk);
		}

		if (chunk->otherData.dirty || chunk->otherData.dirtyEntity || chunk->otherData.dirtyBlockData)
		{
			++it;
			continue;
		}

		// Never remove index entries before every pending persistence write succeeds.
		removeEntityChunkPositionsForChunk(*chunk);
		delete chunk;
		it = savedChunks.erase(it);
		++unloaded;
		if (unloaded >= count) { break; }
	}
	return unloaded;
}''', "unload persistence")
save(path, text)


# ---- Entity IDs are durable before being exposed to the server -----------------------
path = "src/gameLayer/multyPlayer/enetServerFunction.cpp"
text = load(path)
text = replace_once(text, "#include <algorithm>\n", "#include <algorithm>\n#include <multyPlayer/entityIdAllocator.h>\n", "entity allocator include")
text = replace_once(text, "EntityIdHolder entityIds;\n", "EntityIdHolder entityIds;\nstatic PersistentEntityIdAllocator persistentEntityIds;\n", "entity allocator instance")
text = replace_region(text,
    "std::uint64_t getEntityIdAndIncrement(WorldSaver &worldSaver, int entityType)",
    "std::uint64_t getCurrentEntityId(int entityType)",
    r'''std::uint64_t getEntityIdAndIncrement(WorldSaver &worldSaver, int entityType)
{
	permaAssert(entityType < EntitiesTypesCount);
	permaAssert(entityType >= 0);

	const std::uint64_t rawId = persistentEntityIds.allocate(worldSaver.savePath,
		static_cast<unsigned int>(entityType));
	permaAssertComment(rawId != 0 && rawId < 0x00FFFFFFFFFFFFFFULL,
		"Server could not reserve a persistent entity ID");
	entityIds.entityIds[entityType] = std::max(entityIds.entityIds[entityType], rawId + 1);
	return rawId | (static_cast<std::uint64_t>(static_cast<unsigned char>(entityType)) << 56);
}''', "getEntityIdAndIncrement")
text = replace_region(text,
    "std::uint64_t getCurrentEntityId(int entityType)",
    "void reserveEntityId(std::uint64_t entityId)",
    r'''std::uint64_t getCurrentEntityId(int entityType)
{
	permaAssert(entityType < EntitiesTypesCount);
	permaAssert(entityType >= 0);
	return std::max(entityIds.entityIds[entityType],
		persistentEntityIds.peek(static_cast<unsigned int>(entityType)));
}''', "getCurrentEntityId")
text = replace_region(text,
    "void reserveEntityId(std::uint64_t entityId)",
    "void broadCastNotLocked(Packet p, void *data, size_t size, ENetPeer *peerToIgnore,",
    r'''void reserveEntityId(std::uint64_t entityId)
{
	const unsigned int entityType = getEntityTypeFromEID(entityId);
	if (entityType >= EntitiesTypesCount) { return; }
	const std::uint64_t rawId = getOnlyIdFromEID(entityId);
	if (rawId >= 0x00FFFFFFFFFFFFFFULL) { return; }

	persistentEntityIds.observe(entityId);
	entityIds.entityIds[entityType] = std::max(entityIds.entityIds[entityType], rawId + 1);
}''', "reserveEntityId")
save(path, text)


# ---- Tick: transactional player drops and no disappearing boundary entities ----------
path = "src/gameLayer/multyPlayer/tick.cpp"
text = load(path)
text = replace_once(text, "#include <gameplay/itemDurability.h>\n", "#include <gameplay/itemDurability.h>\n#include <multyPlayer/dataIntegrity.h>\n", "tick integrity include")
old_orphan = '''							else
							{
								//std::cout << "Not Found!\\n";

								//the entity left the region, we move it out,
								// so we save it to disk or to other chunks

								auto found = chunkCache.entityChunkPositions.find(e.first);
								if (found != chunkCache.entityChunkPositions.end())
								{
									chunkCache.entityChunkPositions.erase(found);
								}

								orphanContainer.insert(
									{e.first, e.second});
							}


							it = container.erase(it);'''
new_orphan = '''							else
							{
								// Do not let a simulated entity fall out of the loaded world and then
								// disappear when the per-region orphan container is destroyed. Keep it
								// inside the last authoritative loaded chunk until streaming catches up.
								const glm::dvec3 clampedPosition = mie::dataIntegrity::clampEntityPositionToChunk(
									e.second.getPosition(), initialChunk);
								e.second.entity.position = clampedPosition;
								e.second.entity.lastPosition = clampedPosition;
								chunkCache.entityChunkPositions[e.first] = initialChunk;
								++it;
								continue;
							}


							it = container.erase(it);'''
text = replace_once(text, old_orphan, new_orphan, "streaming boundary retention")

validation_anchor = '''							if (client->playerData.killed)
							{
								serverAllows = 0;
							}


							if (
								getEntityTypeFromEID(i.t.entityId) != EntityType::droppedItems ||'''
validation_new = '''							if (client->playerData.killed)
							{
								serverAllows = 0;
							}

							if (!mie::dataIntegrity::isDroppedItemSpawnPositionValid(
								client->playerData.entity.position, i.t.doublePos))
							{
								serverAllows = false;
							}


							if (
								getEntityTypeFromEID(i.t.entityId) != EntityType::droppedItems ||'''
text = replace_once(text, validation_anchor, validation_new, "drop position validation")

old_drop = '''							if (computeRevisionStuff(*client, true && serverAllows, i.t.eventId,
								&i.t.entityId, &newId))
							{

								//todo get or create chunk here, so we create a function that cant fail.
								spawnDroppedItemEntity(chunkCache,
									worldSaver, i.t.blockCount, i.t.blockType, &from->metaData,
									i.t.doublePos, i.t.motionState, newId,
									computeRestantTimer(i.t.timer, getTimer()));

								//std::cout << "restant: " << newEntity.restantTime << "\\n";

								//substract item from inventory
								from->counter -= i.t.blockCount;
								if (!from->counter) { *from = {}; }

							}
'''
new_drop = '''							if (computeRevisionStuff(*client, serverAllows, i.t.eventId,
								&i.t.entityId, &newId))
							{
								const bool spawned = spawnDroppedItemEntity(chunkCache,
									worldSaver, i.t.blockCount, i.t.blockType, &from->metaData,
									i.t.doublePos, i.t.motionState, newId,
									computeRestantTimer(i.t.timer, getTimer()));

								if (spawned)
								{
									// Consume inventory only after the authoritative entity exists.
									from->counter -= i.t.blockCount;
									if (!from->counter) { *from = {}; }
								}
								else
								{
									serverAllows = false;
								}
							}
							else
							{
								serverAllows = false;
							}
'''
text = replace_once(text, old_drop, new_drop, "transactional dropped item")
save(path, text)


# ---- Regression contracts ------------------------------------------------------------
path = "tests/survivalRulesTests.cpp"
text = load(path)
text = replace_once(text, "#include <multyPlayer/serverActionValidation.h>\n", "#include <multyPlayer/serverActionValidation.h>\n#include <multyPlayer/dataIntegrity.h>\n", "survivalRules integrity include")
anchor = '\tstd::cout << "Survival rule tests passed.\\n";'
tests = r'''	// v0.9.3.1: entities that step beyond an unloaded streaming boundary are
	// retained inside the last authoritative chunk instead of being discarded.
	const glm::ivec2 retainedChunk{2, -3};
	const glm::dvec3 escapedPosition{
		static_cast<double>((retainedChunk.x + 1) * CHUNK_SIZE) + 4.0,
		64.0,
		static_cast<double>(retainedChunk.y * CHUNK_SIZE) - 4.0};
	const glm::dvec3 clampedPosition = mie::dataIntegrity::clampEntityPositionToChunk(
		escapedPosition, retainedChunk);
	REQUIRE(determineChunkThatIsEntityIn(clampedPosition) == retainedChunk);

	REQUIRE(mie::dataIntegrity::isDroppedItemSpawnPositionValid(
		{0.0, 64.0, 0.0}, {3.0, 64.0, 2.0}));
	REQUIRE(!mie::dataIntegrity::isDroppedItemSpawnPositionValid(
		{0.0, 64.0, 0.0}, {20.0, 64.0, 0.0}));
	REQUIRE(!mie::dataIntegrity::isDroppedItemSpawnPositionValid(
		{0.0, 64.0, 0.0}, {NAN, 64.0, 0.0}));

	const auto collisionQuery = mie::dataIntegrity::makeBlockPlacementCollisionQuery(
		{128.0, 70.0, -64.0}, {0.8f, 1.8f, 0.8f}, {4, 70, 9});
	REQUIRE(collisionQuery.entityPosition == glm::dvec3(128.0, 70.0, -64.0));
	REQUIRE(collisionQuery.blockPosition == glm::ivec3(4, 70, 9));

'''
text = replace_once(text, anchor, tests + anchor, "survivalRules v0.9.3.1 tests")
save(path, text)


# ---- Build the allocator regression test in normal CI --------------------------------
path = "CMakeLists.txt"
text = load(path)
insert_after = '''\tadd_test(NAME farming-persistence COMMAND farmingTests)\n'''
allocator_target = r'''

	add_executable(entityIdAllocatorTests
		tests/entityIdAllocatorTests.cpp
		src/gameLayer/multyPlayer/entityIdAllocator.cpp)
	target_include_directories(entityIdAllocatorTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/"
		"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/safeSave/include/")
	target_link_libraries(entityIdAllocatorTests PRIVATE safeSave)
	add_test(NAME entity-id-allocator COMMAND entityIdAllocatorTests)
'''
text = replace_once(text, insert_after, insert_after + allocator_target, "CMake allocator target")
text = replace_once(text, "\t\tfarming-persistence\n\t\tPROPERTIES TIMEOUT 60)", "\t\tfarming-persistence\n\t\tentity-id-allocator\n\t\tPROPERTIES TIMEOUT 60)", "CMake allocator timeout")
text = replace_once(text, 'set(OURCRAFT_VERSION "0.9.3" CACHE STRING "Mie Survival semantic version")', 'set(OURCRAFT_VERSION "0.9.3.1" CACHE STRING "Mie Survival semantic version")', "CMake version")
save(path, text)


# ---- Release version + docs ----------------------------------------------------------
version_path = ROOT / "VERSION"
if version_path.read_text(encoding="utf-8").strip() != "0.9.3":
    raise RuntimeError("VERSION did not contain the expected 0.9.3 baseline")
version_path.write_text("0.9.3.1\n", encoding="utf-8")

path = "windows/Mie.iss"
text = load(path)
text = replace_once(text, '#define MyAppVersion "0.9.3"', '#define MyAppVersion "0.9.3.1"', "Windows version")
save(path, text)

path = "README.md"
text = load(path)
text = replace_once(text, "**Current development version: v0.9.2**", "**Current development version: v0.9.3.1**", "README current version")
section_marker = "## v0.9.2 gameplay & performance"
section = r'''## v0.9.3.1 data-integrity hotfix

v0.9.3.1 is a corrective release for persistence and authoritative-world integrity. It adds no new content.

- Persisted entity sidecars are restored only for chunks whose terrain was actually restored from disk.
- Entity and block-data sidecars use transactional temp/backup writes; failed writes stay dirty and block chunk unload until persistence succeeds.
- Non-player entities are retained at the last loaded chunk boundary instead of disappearing when simulation outruns streaming.
- Player item drops are validated against the authoritative player position and consume inventory only after the server-side dropped entity is created.
- Entity IDs reserve persistent high-water ranges before reuse, with backup recovery and a high migration floor for legacy worlds.
- Placed-block collision checks now keep the target block position separate from the entity position.
- Save IDs, item/block IDs and multiplayer packet layouts remain unchanged from v0.9.3.

See [`docs/V0.9.3.1_DATA_INTEGRITY.md`](docs/V0.9.3.1_DATA_INTEGRITY.md).

'''
if section_marker not in text:
    raise RuntimeError("README section marker missing")
text = text.replace(section_marker, section + section_marker, 1)
save(path, text)

(ROOT / "docs/V0.9.3.1_DATA_INTEGRITY.md").write_text(r'''# Mie Survival v0.9.3.1 — data-integrity hotfix

This hotfix keeps the v0.9.3 gameplay/content contract and concentrates on save, streaming and authoritative-state correctness.

## Corrected

- Chunk entity data is loaded only when the corresponding chunk terrain was restored successfully. Newly generated chunks no longer revive stale entity sidecars.
- `.entity` and `.block` sidecars are written through a temporary file and promoted transactionally while retaining the previous version as `.bak` recovery data.
- Block-data loading validates the serialized payload and falls back to the backup if the primary is unavailable or invalid.
- Entity loading parses into temporary state and falls back to the backup on corruption, preventing partially loaded entity maps from becoming authoritative.
- Dirty entity/block-data flags are cleared only after the corresponding write succeeds.
- A chunk marked for unload stays resident while any terrain, entity or block-data persistence write remains pending.
- Non-player entities that cross into a chunk not yet loaded are clamped to the last authoritative chunk boundary instead of being destroyed with a temporary orphan container.
- Manual item drops reject non-finite/remote positions and inventory is decremented only after the authoritative dropped entity was inserted successfully.
- Entity IDs use persisted, checksummed high-water reservations per world. Restarts skip the previously reserved range and loaded legacy IDs raise the reservation floor.
- Placed-block collision checks no longer shadow the block position with the entity position.

## Compatibility

- No item, block or entity type IDs changed.
- No multiplayer packet layout changed.
- Existing v0.9.3 worlds remain readable.
- Existing legacy `.entity` and `.block` sidecars remain the primary filenames; `.bak` files are recovery mirrors created by v0.9.3.1.
- The entity-ID high-water sidecar is additive and does not alter existing entity serialization.

## Validation contracts

- Entity-ID allocation is tested across restart, observed legacy IDs and primary-state corruption with backup recovery.
- Survival rules test streaming-boundary clamping, drop-distance validation and the separation of entity/block positions in placement collision queries.
- Windows Release and Linux Flatpak CI remain the release gates.
''', encoding="utf-8")


# ---- Static assertions on the resulting patch ---------------------------------------
checks = {
    "src/gameLayer/multyPlayer/serverChunkStorer.cpp": [
        "worldSaver.loadEntityData(rez->entityData, pos);",
        "query.blockPosition",
        "dirtyEntity || chunk->otherData.dirtyBlockData",
    ],
    "src/gameLayer/multyPlayer/tick.cpp": [
        "Consume inventory only after the authoritative entity exists.",
        "clampEntityPositionToChunk",
        "isDroppedItemSpawnPositionValid",
    ],
    "src/gameLayer/multyPlayer/enetServerFunction.cpp": [
        "persistentEntityIds.allocate",
        "persistentEntityIds.observe",
    ],
    "src/gameLayer/multyPlayer/chunkSaver.cpp": [
        "promoteSidecarTempFile",
        "loadEntitySidecarCandidate",
        "sidecarBackupPath",
    ],
}
for rel, needles in checks.items():
    data = load(rel)
    for needle in needles:
        if needle not in data:
            raise RuntimeError(f"post-patch assertion failed: {rel} missing {needle}")

print("v0.9.3.1 guarded hotfix patch applied successfully")
