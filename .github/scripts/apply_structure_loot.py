from pathlib import Path

path = Path('src/gameLayer/multyPlayer/serverChunkStorer.cpp')
s = path.read_text()
old = '''\t\t\t\t\t\t\tc->removeBlockWithData({inChunkX, y, inChunkZ}, oldBlock.getType());
\t\t\t\t\t\t\tb = newBlock; //we set the new block!

\t\t\t\t\t\t\tif (sendDataToPlayers)
'''
new = '''\t\t\t\t\t\t\tc->removeBlockWithData({inChunkX, y, inChunkZ}, oldBlock.getType());
\t\t\t\t\t\t\tb = newBlock; //we set the new block!

\t\t\t\t\t\t\t// Exploration loot: structure chests get deterministic supplies the first
\t\t\t\t\t\t\t// time they are generated. Existing/player chest contents are never replaced.
\t\t\t\t\t\t\tif (isChest(b.getType()))
\t\t\t\t\t\t\t{
\t\t\t\t\t\t\t\tauto *chest = c->blockData.getOrCreateChestBlock(inChunkX, y, inChunkZ);
\t\t\t\t\t\t\t\tbool emptyChest = true;
\t\t\t\t\t\t\t\tfor (const auto &item : chest->items)
\t\t\t\t\t\t\t\t{
\t\t\t\t\t\t\t\t\tif (item.type != 0) { emptyChest = false; break; }
\t\t\t\t\t\t\t\t}

\t\t\t\t\t\t\t\tif (emptyChest)
\t\t\t\t\t\t\t\t{
\t\t\t\t\t\t\t\t\tconst std::uint32_t lootSeed =
\t\t\t\t\t\t\t\t\t\tstatic_cast<std::uint32_t>(x) * 73856093u ^
\t\t\t\t\t\t\t\t\t\tstatic_cast<std::uint32_t>(y) * 19349663u ^
\t\t\t\t\t\t\t\t\t\tstatic_cast<std::uint32_t>(z) * 83492791u ^
\t\t\t\t\t\t\t\t\t\tstatic_cast<std::uint32_t>(s.type) * 2654435761u;
\n\t\t\t\t\t\t\t\t\tauto put = [&](int slot, unsigned short type, unsigned short amount)
\t\t\t\t\t\t\t\t\t{
\t\t\t\t\t\t\t\t\t\tif (slot >= 0 && slot < CHEST_CAPACITY && amount > 0)
\t\t\t\t\t\t\t\t\t\t{
\t\t\t\t\t\t\t\t\t\t\tchest->items[slot] = itemCreator(type, amount);
\t\t\t\t\t\t\t\t\t\t}
\t\t\t\t\t\t\t\t\t};

\t\t\t\t\t\t\t\t\t// Universal expedition supplies.
\t\t\t\t\t\t\t\t\tput(0, ItemTypes::bandage, 1 + (lootSeed % 2u));
\t\t\t\t\t\t\t\t\tput(1, ItemTypes::copperCoin, 4 + (lootSeed % 13u));

\t\t\t\t\t\t\t\t\tswitch (s.type)
\t\t\t\t\t\t\t\t\t{
\t\t\t\t\t\t\t\t\tcase Structure_Barn:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::wheat, 5 + (lootSeed % 8u));
\t\t\t\t\t\t\t\t\t\tput(3, ItemTypes::apple, 1 + (lootSeed % 3u));
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\tcase Structure_GoblinTower:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::cloth, 2 + (lootSeed % 4u));
\t\t\t\t\t\t\t\t\t\tput(3, ItemTypes::fang, 1 + (lootSeed % 3u));
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\tcase Structure_Pyramid:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::silverCoin, 1 + (lootSeed % 4u));
\t\t\t\t\t\t\t\t\t\tif ((lootSeed % 5u) == 0u) put(3, ItemTypes::goldIngot, 1);
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\tcase Structure_Igloo:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::blueBerrie, 2 + (lootSeed % 4u));
\t\t\t\t\t\t\t\t\t\tput(3, ItemTypes::apple, 1 + (lootSeed % 2u));
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\tcase Structure_MinesDungeon:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::ironIngot, 1 + (lootSeed % 3u));
\t\t\t\t\t\t\t\t\t\tif ((lootSeed % 4u) == 0u) put(3, ItemTypes::silverIngot, 1);
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\tcase Structure_AbandonedTrainingCamp:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::arrow, 4 + (lootSeed % 8u));
\t\t\t\t\t\t\t\t\t\tput(3, ItemTypes::wheat, 1 + (lootSeed % 4u));
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\tdefault:
\t\t\t\t\t\t\t\t\t\tput(2, ItemTypes::wheat, 1 + (lootSeed % 3u));
\t\t\t\t\t\t\t\t\t\tbreak;
\t\t\t\t\t\t\t\t\t}
\t\t\t\t\t\t\t\t}
\t\t\t\t\t\t\t}

\t\t\t\t\t\t\tif (sendDataToPlayers)
'''
count = s.count(old)
if count != 1:
    raise SystemExit(f'expected one block-placement anchor, found {count}')
path.write_text(s.replace(old, new, 1))

p = Path('docs/EXPLORATION_CONTENT.md')
doc = p.read_text()
doc = doc.replace('## Survival integration\n', '## Structure loot\n- Generated structure chests receive deterministic loot based on structure type and coordinates.\n- Barns favour food/wheat; goblin towers favour cloth/fangs; pyramids favour coins/rare metal; igloos favour food; mines favour ingots; training camps favour arrows.\n- Existing chest data is never overwritten.\n\n## Survival integration\n')
p.write_text(doc)
