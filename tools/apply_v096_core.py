#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8-sig")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def replace_n(text: str, old: str, new: str, expected: int, label: str) -> str:
    count = text.count(old)
    if count != expected:
        raise RuntimeError(f"{label}: expected {expected} matches, found {count}")
    return text.replace(old, new)


def patch_chunk_header() -> None:
    path = "include/gameLayer/chunkSystem.h"
    text = read(path)
    old = "\tstd::vector<Chunk*> loadedChunks;\n\tint squareSize = 4;\n\tstd::vector<glm::ivec2> chunksToAddLight;"
    new = """\tstd::vector<Chunk*> loadedChunks;
\tint squareSize = 4;
\t// v0.9.6: render-distance changes are staged until update(), where the
\t// ClientEntityManager is available. This lets us preserve chunks that stay
\t// inside the new radius instead of destroying/reloading the whole cache.
\tint pendingSquareSize = 0;
\tbool pendingRenderDistanceNotifyServer = true;
\tstd::vector<glm::ivec2> chunksToAddLight;"""
    text = replace_once(text, old, new, "chunkSystem.h resize state")
    write(path, text)


def patch_chunk_cpp() -> None:
    path = "src/gameLayer/chunkSystem.cpp"
    text = read(path)

    old_change = """void ChunkSystem::changeRenderDistance(int squareDistance, bool notifyServer)
{

\tif (squareSize == squareDistance) { return; }
\tif (squareDistance > 102) { squareDistance = 102; }
\tif (squareDistance < 2) { squareDistance = 2; }

\tcleanup(notifyServer);

\tinit(squareDistance);

}"""
    new_change = """void ChunkSystem::changeRenderDistance(int squareDistance, bool notifyServer)
{
\tif (squareDistance > 102) { squareDistance = 102; }
\tif (squareDistance < 2) { squareDistance = 2; }
\tif (squareSize == squareDistance && pendingSquareSize == 0) { return; }

#if REMOVE_BIG_GPU_BUFFER == 1
\t// Do not tear down the complete chunk cache. update() will remap retained
\t// chunks into the new matrix and discard only the ring that left the view.
\tpendingSquareSize = squareDistance;
\tpendingRenderDistanceNotifyServer = notifyServer;
#else
\t// The legacy arena allocates capacity from squareSize. If it is re-enabled,
\t// keep the conservative full rebuild until the arena itself supports resize.
\tcleanup(notifyServer);
\tinit(squareDistance);
#endif
}"""
    text = replace_once(text, old_change, new_change, "incremental changeRenderDistance")

    old_update = """\t//index of the chunk
\tint x = divideChunk(playerBlockPosition.x);
\tint z = divideChunk(playerBlockPosition.z);

\t//bottom left most chunk and top right most chunk of my array"""
    new_update = """\t//index of the chunk
\tint x = divideChunk(playerBlockPosition.x);
\tint z = divideChunk(playerBlockPosition.z);

#if REMOVE_BIG_GPU_BUFFER == 1
\tif (pendingSquareSize != 0 && pendingSquareSize != squareSize)
\t{
\t\tconst int newSquareSize = pendingSquareSize;
\t\tconst glm::ivec2 newMinPos = glm::ivec2(x, z) -
\t\t\tglm::ivec2(newSquareSize / 2, newSquareSize / 2);
\t\tconst glm::ivec2 newMaxPos = glm::ivec2(x, z) +
\t\t\tglm::ivec2(newSquareSize / 2 + newSquareSize % 2,
\t\t\t\tnewSquareSize / 2 + newSquareSize % 2);
\t\tstd::vector<Chunk *> resizedChunks(
\t\t\tstatic_cast<std::size_t>(newSquareSize) * newSquareSize, nullptr);

\t\tfor (std::size_t i = 0; i < loadedChunks.size(); ++i)
\t\t{
\t\t\tChunk *chunk = loadedChunks[i];
\t\t\tif (!chunk) { continue; }

\t\t\tconst glm::ivec2 chunkPos(chunk->data.x, chunk->data.z);
\t\t\tconst bool keep = chunkPos.x >= newMinPos.x && chunkPos.y >= newMinPos.y &&
\t\t\t\tchunkPos.x < newMaxPos.x && chunkPos.y < newMaxPos.y &&
\t\t\t\tisChunkInRadius({playerBlockPosition.x, playerBlockPosition.z},
\t\t\t\t\tchunkPos, newSquareSize);

\t\t\tif (keep)
\t\t\t{
\t\t\t\tconst glm::ivec2 relative = chunkPos - newMinPos;
\t\t\t\tresizedChunks[static_cast<std::size_t>(relative.x) * newSquareSize +
\t\t\t\t\trelative.y] = chunk;
\t\t\t\tloadedChunks[i] = nullptr;
\t\t\t\tchunk->setDirty(true);
\t\t\t\tchunk->setDirtyTransparency(true);
\t\t\t\tcontinue;
\t\t\t}

\t\t\tif (pendingRenderDistanceNotifyServer)
\t\t\t{
\t\t\t\tPacket packet = {};
\t\t\t\tpacket.cid = getConnectionData().cid;
\t\t\t\tpacket.header = headerClientDroppedChunk;
\t\t\t\tPacket_ClientDroppedChunk packetData = {};
\t\t\t\tpacketData.chunkPos = chunkPos;
\t\t\t\tsendPacket(getConnectionData().server, packet,
\t\t\t\t\treinterpret_cast<char *>(&packetData), sizeof(packetData), true,
\t\t\t\t\tchannelPlayerPositions);
\t\t\t}

\t\t\tauto &chunkData = chunk->data;
\t\t\tfor (int blockX = 0; blockX < CHUNK_SIZE; ++blockX)
\t\t\t\tfor (int blockZ = 0; blockZ < CHUNK_SIZE; ++blockZ)
\t\t\t\t\tfor (int blockY = 0; blockY < CHUNK_HEIGHT; ++blockY)
\t\t\t\t\t{
\t\t\t\t\t\tclientEntityManager.removeBlockEntity(
\t\t\t\t\t\t\t{blockX + chunkData.x * CHUNK_SIZE, blockY,
\t\t\t\t\t\t\t blockZ + chunkData.z * CHUNK_SIZE},
\t\t\t\t\t\t\tchunkData.blocks[blockX][blockZ][blockY].getType());
\t\t\t\t\t}
\t\t\tdropChunkAtIndexUnsafe(static_cast<int>(i), &gpuBuffer);
\t\t}

\t\tloadedChunks = std::move(resizedChunks);
\t\tsquareSize = newSquareSize;
\t\tcornerPos = newMinPos;
\t\tpendingSquareSize = 0;
\t\tchunksToAddLight.clear(); // pending matrix coordinates belonged to the old grid
\t\tshouldUpdateLights = true;
\t\tcreated = 1;
\t\tlastX = x;
\t\tlastZ = z;
\t}
\telse if (pendingSquareSize == squareSize)
\t{
\t\tpendingSquareSize = 0;
\t}
#endif

\t//bottom left most chunk and top right most chunk of my array"""
    text = replace_once(text, old_update, new_update, "chunk resize in update")
    write(path, text)


def patch_tick() -> None:
    path = "src/gameLayer/multyPlayer/tick.cpp"
    text = read(path)

    marker = "\n\t// Furnace simulation is server-authoritative and runs only for chunks kept in\n"
    if text.count(marker) != 1:
        raise RuntimeError(f"tick interaction helper marker: found {text.count(marker)}")
    helpers = r'''

	constexpr double BLOCK_INTERACTION_REACH = 21.5;
	auto isBlockInteractionWithinReach = [&](const Client &client, const glm::ivec3 &position)
	{
		const glm::dvec3 delta = client.playerData.getPosition() - glm::dvec3(position);
		return glm::dot(delta, delta) <= BLOCK_INTERACTION_REACH * BLOCK_INTERACTION_REACH;
	};

	auto isCurrentBlockInteractionValid = [&](const Client &client)
	{
		const int interactionType = client.playerData.interactingWithBlock;
		if (!interactionType || client.playerData.killed) { return false; }
		const glm::ivec3 position = client.playerData.currentBlockInteractWithPosition;
		if (!isBlockInteractionWithinReach(client, position)) { return false; }
		Block *block = chunkCache.getBlockSafe(position);
		return block && isInteractable(block->getType()) == interactionType;
	};

	auto resyncAndCloseInteraction = [&](Client &client)
	{
		const int interactionType = client.playerData.interactingWithBlock;
		const glm::ivec3 position = client.playerData.currentBlockInteractWithPosition;
		const unsigned char interactionRevision = client.playerData.revisionNumberInteraction;

		if (interactionType == InteractionTypes::chestInteraction)
		{
			SavedChunk *containerChunk = nullptr;
			if (ChestBlock *chest = chunkCache.getChestBlock(position, containerChunk))
			{
				sendChestDataToCurrentPlayer(&client, *chest, position);
			}
		}
		else if (interactionType == InteractionTypes::furnace)
		{
			SavedChunk *containerChunk = nullptr;
			if (FurnaceBlock *furnace = chunkCache.getFurnaceBlock(position, containerChunk))
			{
				sendFurnaceData(&client, *furnace, position, true);
			}
		}

		sendPlayerInventoryAndIncrementRevision(client);
		sendPlayerExitInteraction(client, interactionRevision);
		client.playerData.interactingWithBlock = 0;
		client.playerData.currentBlockInteractWithPosition = {0, -1, 0};
	};

	// Interaction sessions are leases, not permanent capabilities. Revalidate
	// every server tick so walking away, death, chunk unload or block replacement
	// closes the UI even when the client sends no further container operation.
	for (auto &entry : allClients)
	{
		Client *client = entry.second;
		if (client && client->playerData.interactingWithBlock &&
			!isCurrentBlockInteractionValid(*client))
		{
			resyncAndCloseInteraction(*client);
		}
	}
'''
    text = text.replace(marker, helpers + marker, 1)

    old_open = """\t\t\t\t\tif (b && b->getType() == blockType
\t\t\t\t\t\t&& isInteractable(blockType)
\t\t\t\t\t\t)
\t\t\t\t\t{
\t\t\t\t\t\t//todo check distance.
\t\t\t\t\t\tallows = true;
\t\t\t\t\t}"""
    new_open = """\t\t\t\t\tif (b && b->getType() == blockType
\t\t\t\t\t\t&& isInteractable(blockType)
\t\t\t\t\t\t&& isBlockInteractionWithinReach(*client, i.t.pos)
\t\t\t\t\t\t)
\t\t\t\t\t{
\t\t\t\t\t\tallows = true;
\t\t\t\t\t}"""
    text = replace_once(text, old_open, new_open, "interaction open reach validation")

    # Both item-move and item-swap resolve container pointers from the stored
    # station position. Move usesContainer ahead of that resolution and reject a
    # stale lease before it can expose pointers to an unloaded/distant station.
    old_container_prefix = """\t\t\t\t\t\tconst glm::ivec3 containerPos = client->playerData.currentBlockInteractWithPosition;
\t\t\t\t\t\tif (client->playerData.interactingWithBlock == InteractionTypes::chestInteraction)"""
    new_container_prefix = """\t\t\t\t\t\tconst glm::ivec3 containerPos = client->playerData.currentBlockInteractWithPosition;
\t\t\t\t\t\tconst bool usesContainer = i.t.from >= PlayerInventory::CHEST_START_INDEX ||
\t\t\t\t\t\t\ti.t.to >= PlayerInventory::CHEST_START_INDEX;
\t\t\t\t\t\tif (usesContainer && !isCurrentBlockInteractionValid(*client))
\t\t\t\t\t\t{
\t\t\t\t\t\t\tresyncAndCloseInteraction(*client);
\t\t\t\t\t\t\tcontinue;
\t\t\t\t\t\t}
\t\t\t\t\t\tif (client->playerData.interactingWithBlock == InteractionTypes::chestInteraction)"""
    text = replace_n(text, old_container_prefix, new_container_prefix, 2, "move/swap container lease validation")

    old_uses = """\n\t\t\t\t\t\tconst bool usesContainer = i.t.from >= PlayerInventory::CHEST_START_INDEX ||
\t\t\t\t\t\t\ti.t.to >= PlayerInventory::CHEST_START_INDEX;"""
    # The original declaration now remains once in each block after the pointer
    # resolution. Remove those two duplicates; the new declaration is earlier.
    text = replace_n(text, old_uses, "", 2, "remove duplicate usesContainer declarations")

    old_craft = """\t\t\t\t\t\tauto resultCrafting = getRecepieFromIndexUnsafe(craftingIndex);
\t\t\t\t\t\tbool correctStation = true;"""
    new_craft = """\t\t\t\t\t\tauto resultCrafting = getRecepieFromIndexUnsafe(craftingIndex);
\t\t\t\t\t\tconst bool requiresActiveStation = resultCrafting.requiresWorkBench ||
\t\t\t\t\t\t\tresultCrafting.requiresCookingPot || resultCrafting.requiresGoblin;
\t\t\t\t\t\tif (requiresActiveStation && !isCurrentBlockInteractionValid(*client))
\t\t\t\t\t\t{
\t\t\t\t\t\t\tresyncAndCloseInteraction(*client);
\t\t\t\t\t\t\tcontinue;
\t\t\t\t\t\t}
\t\t\t\t\t\tbool correctStation = true;"""
    text = replace_once(text, old_craft, new_craft, "crafting station lease validation")

    old_destroy = """\t\t\t\t\t\t\t//close interaction with block.
\t\t\t\t\t\t\t//todo close chests here.
\t\t\t\t\t\t\tc.second.playerData.interactingWithBlock = 0;
\t\t\t\t\t\t\tc.second.playerData.currentBlockInteractWithPosition = {0,-1,0};"""
    new_destroy = """\t\t\t\t\t\t\t// The authoritative block changed while its UI was open.
\t\t\t\t\t\t\t// Close the lease and resync inventory immediately.
\t\t\t\t\t\t\tresyncAndCloseInteraction(c.second);"""
    text = replace_once(text, old_destroy, new_destroy, "close interaction when block changes")

    write(path, text)


def patch_version_docs() -> None:
    path = "CMakeLists.txt"
    text = read(path)
    text = replace_once(text, 'set(OURCRAFT_VERSION "0.9.5")',
                        'set(OURCRAFT_VERSION "0.9.6")', "CMake version")
    write(path, text)

    path = "VERSION"
    text = read(path)
    if text.strip() != "0.9.5":
        raise RuntimeError(f"VERSION expected 0.9.5, got {text.strip()!r}")
    write(path, "0.9.6\n")

    path = "README.md"
    text = read(path)
    text = replace_once(text,
                        "**Current development version: v0.9.4**",
                        "**Current development version: v0.9.6 (release candidate)**",
                        "README development version")
    write(path, text)


def main() -> None:
    patch_chunk_header()
    patch_chunk_cpp()
    patch_tick()
    patch_version_docs()
    print("v0.9.6 guarded core patches applied successfully")


if __name__ == "__main__":
    main()
