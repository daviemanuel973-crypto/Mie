#!/usr/bin/env python3
from pathlib import Path

PATH = Path("src/gameLayer/multyPlayer/tick.cpp")
text = PATH.read_text(encoding="utf-8")


def once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    text = text.replace(old, new, 1)


helper_marker = '''\t// Treat an interaction as a lease. Distance, death, chunk lifetime and block
\t// identity are authoritative server state and are revalidated every tick.
'''
helper = '''\tauto resyncInventoryAndCurrentInteraction = [&](Client &client)
\t{
\t\t// A stale inventory revision is an explicit rejection, not a packet to
\t\t// silently ignore. If the station lease itself expired, close it using the
\t\t// stronger path; otherwise resend both inventory and container state.
\t\tif (client.playerData.interactingWithBlock && !isCurrentBlockInteractionValid(client))
\t\t{
\t\t\tresyncAndCloseInteraction(client);
\t\t\treturn;
\t\t}

\t\tsendPlayerInventoryAndIncrementRevision(client);
\t\tif (!client.playerData.interactingWithBlock) { return; }

\t\tconst glm::ivec3 position = client.playerData.currentBlockInteractWithPosition;
\t\tif (client.playerData.interactingWithBlock == InteractionTypes::chestInteraction)
\t\t{
\t\t\tSavedChunk *containerChunk = nullptr;
\t\t\tif (ChestBlock *chest = chunkCache.getChestBlock(position, containerChunk))
\t\t\t{
\t\t\t\tsendChestDataToCurrentPlayer(&client, *chest, position);
\t\t\t}
\t\t}
\t\telse if (client.playerData.interactingWithBlock == InteractionTypes::furnace)
\t\t{
\t\t\tSavedChunk *containerChunk = nullptr;
\t\t\tif (FurnaceBlock *furnace = chunkCache.getFurnaceBlock(position, containerChunk))
\t\t\t{
\t\t\t\tsendFurnaceData(&client, *furnace, position, true);
\t\t\t}
\t\t}
\t};

'''
if helper_marker not in text:
    raise RuntimeError("interaction helper insertion marker not found")
text = text.replace(helper_marker, helper + helper_marker, 1)

once(
'''\t\t\t\telse if (i.t.taskType == Task::clientMovedItem)
\t\t\t\t{
\t\t\t\t\tauto client = getClientNotLocked(i.cid);
\t\t\t\t\tif (client && client->playerData.inventory.revisionNumber == i.t.revisionNumber)
\t\t\t\t\t{
''',
'''\t\t\t\telse if (i.t.taskType == Task::clientMovedItem)
\t\t\t\t{
\t\t\t\t\tauto client = getClientNotLocked(i.cid);
\t\t\t\t\tif (client && client->playerData.inventory.revisionNumber != i.t.revisionNumber)
\t\t\t\t\t{
\t\t\t\t\t\tresyncInventoryAndCurrentInteraction(*client);
\t\t\t\t\t\tcontinue;
\t\t\t\t\t}
\t\t\t\t\tif (client)
\t\t\t\t\t{
''',
"moved-item stale revision resync")

once(
'''\t\t\t\telse if (i.t.taskType == Task::clientSwapItems)
\t\t\t\t{
\t\t\t\t\tauto client = getClientNotLocked(i.cid);
\t\t\t\t\tif (client && client->playerData.inventory.revisionNumber == i.t.revisionNumber)
\t\t\t\t\t{
''',
'''\t\t\t\telse if (i.t.taskType == Task::clientSwapItems)
\t\t\t\t{
\t\t\t\t\tauto client = getClientNotLocked(i.cid);
\t\t\t\t\tif (client && client->playerData.inventory.revisionNumber != i.t.revisionNumber)
\t\t\t\t\t{
\t\t\t\t\t\tresyncInventoryAndCurrentInteraction(*client);
\t\t\t\t\t\tcontinue;
\t\t\t\t\t}
\t\t\t\t\tif (client)
\t\t\t\t\t{
''',
"swap-item stale revision resync")

once(
'''\t\t\t\t\tauto client = getClientNotLocked(i.cid);

\t\t\t\t\tif (client)
\t\t\t\t\t{



\t\t\t\t\t\t//if the revision number isn't good we don't do anything
\t\t\t\t\t\tif (client->playerData.inventory.revisionNumber
\t\t\t\t\t\t\t== i.t.revisionNumber
\t\t\t\t\t\t\t)
''',
'''\t\t\t\t\tauto client = getClientNotLocked(i.cid);

\t\t\t\t\tif (client)
\t\t\t\t\t{
\t\t\t\t\t\tif (client->playerData.inventory.revisionNumber != i.t.revisionNumber)
\t\t\t\t\t\t{
\t\t\t\t\t\t\tresyncInventoryAndCurrentInteraction(*client);
\t\t\t\t\t\t\tcontinue;
\t\t\t\t\t\t}

\t\t\t\t\t\tif (client->playerData.inventory.revisionNumber
\t\t\t\t\t\t\t== i.t.revisionNumber
\t\t\t\t\t\t\t)
''',
"craft stale revision resync")

PATH.write_text(text, encoding="utf-8")
print("v0.9.6 stale inventory revision resync patch applied")
