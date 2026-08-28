#!/usr/bin/env python3
import re
from pathlib import Path
import apply_v096_core as base

ROOT = Path(__file__).resolve().parents[1]


def sub_exact(text: str, pattern: str, repl, count: int, label: str) -> str:
    regex = re.compile(pattern, re.MULTILINE)
    found = list(regex.finditer(text))
    if len(found) != count:
        raise RuntimeError(f"{label}: expected {count} matches, found {len(found)}")
    return regex.sub(repl, text)


def patch_tick() -> None:
    path = ROOT / "src/gameLayer/multyPlayer/tick.cpp"
    text = path.read_text(encoding="utf-8-sig")

    marker = "\n\t// Furnace simulation is server-authoritative and runs only for chunks kept in\n"
    if text.count(marker) != 1:
        raise RuntimeError(f"tick interaction helper marker: found {text.count(marker)}")

    helpers = r'''

	constexpr double BLOCK_INTERACTION_REACH = 21.5;
	auto isBlockInteractionWithinReach = [&](const Client &client, const glm::ivec3 &position)
	{
		const glm::dvec3 delta = client.playerData.entity.position - glm::dvec3(position);
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

	// Treat an interaction as a lease. Distance, death, chunk lifetime and block
	// identity are authoritative server state and are revalidated every tick.
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

    open_pattern = (
        r'(?P<i>\t+)if \(b && b->getType\(\) == blockType\n'
        r'(?P=i)\t&& isInteractable\(blockType\)\n'
        r'(?P=i)\t\)\n'
        r'(?P=i)\{\n'
        r'(?P=i)\t//todo check distance\.\n'
        r'(?P=i)\tallows = true;\n'
        r'(?P=i)\}'
    )
    def open_repl(match: re.Match) -> str:
        i = match.group('i')
        return (
            f"{i}if (b && b->getType() == blockType\n"
            f"{i}\t&& isInteractable(blockType)\n"
            f"{i}\t&& isBlockInteractionWithinReach(*client, i.t.pos)\n"
            f"{i}\t)\n"
            f"{i}{{\n"
            f"{i}\tallows = true;\n"
            f"{i}}}"
        )
    text = sub_exact(text, open_pattern, open_repl, 1, "interaction open reach validation")

    uses_pattern = (
        r'(?P<i>\t+)const bool usesContainer = i\.t\.from >= PlayerInventory::CHEST_START_INDEX \|\|\n'
        r'(?P=i)\ti\.t\.to >= PlayerInventory::CHEST_START_INDEX;'
    )
    def uses_repl(match: re.Match) -> str:
        i = match.group('i')
        original = match.group(0)
        return (
            original + "\n" +
            f"{i}if (usesContainer && !isCurrentBlockInteractionValid(*client))\n"
            f"{i}{{\n"
            f"{i}\tresyncAndCloseInteraction(*client);\n"
            f"{i}\tcontinue;\n"
            f"{i}}}"
        )
    text = sub_exact(text, uses_pattern, uses_repl, 2,
                     "move/swap container lease validation")

    craft_pattern = (
        r'(?P<i>\t+)auto resultCrafting = getRecepieFromIndexUnsafe\(craftingIndex\);\n'
        r'(?P=i)bool correctStation = true;'
    )
    def craft_repl(match: re.Match) -> str:
        i = match.group('i')
        return (
            f"{i}auto resultCrafting = getRecepieFromIndexUnsafe(craftingIndex);\n"
            f"{i}const bool requiresActiveStation = resultCrafting.requiresWorkBench ||\n"
            f"{i}\tresultCrafting.requiresCookingPot || resultCrafting.requiresGoblin;\n"
            f"{i}if (requiresActiveStation && !isCurrentBlockInteractionValid(*client))\n"
            f"{i}{{\n"
            f"{i}\tresyncAndCloseInteraction(*client);\n"
            f"{i}\tcontinue;\n"
            f"{i}}}\n"
            f"{i}bool correctStation = true;"
        )
    text = sub_exact(text, craft_pattern, craft_repl, 1,
                     "crafting station lease validation")

    destroy_pattern = (
        r'(?P<i>\t+)//close interaction with block\.\n'
        r'(?P=i)//todo close chests here\.\n'
        r'(?P=i)c\.second\.playerData\.interactingWithBlock = 0;\n'
        r'(?P=i)c\.second\.playerData\.currentBlockInteractWithPosition = \{0,-1,0\};'
    )
    def destroy_repl(match: re.Match) -> str:
        i = match.group('i')
        return (
            f"{i}// The authoritative block changed while its UI was open.\n"
            f"{i}resyncAndCloseInteraction(*c.second);"
        )
    text = sub_exact(text, destroy_pattern, destroy_repl, 1,
                     "close interaction when block changes")

    path.write_text(text, encoding="utf-8")


def main() -> None:
    base.patch_chunk_header()
    base.patch_chunk_cpp()
    patch_tick()
    base.patch_version_docs()
    print("v0.9.6 guarded core patches applied successfully (v2)")


if __name__ == "__main__":
    main()
