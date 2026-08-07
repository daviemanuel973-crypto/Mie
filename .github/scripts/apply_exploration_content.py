from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    s = p.read_text()
    count = s.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one anchor, found {count}")
    p.write_text(s.replace(old, new, 1))


# World exploration landmarks using assets already shipped with the game.
path = "src/gameLayer/worldGenerator.cpp"
old = """\t\tauto barn = [&]()
\t\t{
\t\t\tStructureToGenerate str;
\t\t\tstr.type = Structure_Barn;
\t\t\tsetPosAndRandomNumbers(str);
\t\t\tstr.setDefaultSmallBuildingSettings();
\t\t\tgenerateStructures.push_back(str);
\t\t};
"""
new = old + """
\t\tauto treeHouse = [&]()
\t\t{
\t\t\tStructureToGenerate str;
\t\t\tstr.type = Structure_TreeHouse;
\t\t\tsetPosAndRandomNumbers(str);
\t\t\tstr.setDefaultSmallBuildingSettings();
\t\t\tgenerateStructures.push_back(str);
\t\t};

\t\tauto pyramid = [&]()
\t\t{
\t\t\tStructureToGenerate str;
\t\t\tstr.type = Structure_Pyramid;
\t\t\tsetPosAndRandomNumbers(str);
\t\t\tstr.setDefaultSmallBuildingSettings();
\t\t\tgenerateStructures.push_back(str);
\t\t};

\t\tauto igloo = [&]()
\t\t{
\t\t\tStructureToGenerate str;
\t\t\tstr.type = Structure_Igloo;
\t\t\tsetPosAndRandomNumbers(str);
\t\t\tstr.setDefaultSmallBuildingSettings();
\t\t\tgenerateStructures.push_back(str);
\t\t};

\t\tauto minesDungeon = [&]()
\t\t{
\t\t\tStructureToGenerate str;
\t\t\tstr.type = Structure_MinesDungeon;
\t\t\tsetPosAndRandomNumbers(str);
\t\t\tstr.setDefaultDungeonSettings();
\t\t\tgenerateStructures.push_back(str);
\t\t};
"""
replace_once(path, old, new)

old = """\t\t\t\t\tif (currentBiomeIndex == BiomesManager::hayLand)
\t\t\t\t\t{
\t\t\t\t\t\tstructuresChoice.push_back(barn);
\t\t\t\t\t\tstructuresChoice.push_back(barn);
\t\t\t\t\t\tstructuresChoice.push_back(barn);
\t\t\t\t\t}
"""
new = """\t\t\t\t\t// Exploration Update: each biome gets recognisable landmarks.
\t\t\t\t\tif (currentBiomeIndex == BiomesManager::hayLand)
\t\t\t\t\t{
\t\t\t\t\t\tstructuresChoice.push_back(barn);
\t\t\t\t\t\tstructuresChoice.push_back(barn);
\t\t\t\t\t\tstructuresChoice.push_back(barn);
\t\t\t\t\t\tstructuresChoice.push_back(tavern);
\t\t\t\t\t}
\t\t\t\t\telse if (currentBiomeIndex == BiomesManager::desert)
\t\t\t\t\t{
\t\t\t\t\t\tstructuresChoice.push_back(pyramid);
\t\t\t\t\t\tstructuresChoice.push_back(pyramid);
\t\t\t\t\t\tstructuresChoice.push_back(smallStoneRuins);
\t\t\t\t\t}
\t\t\t\t\telse if (currentBiomeIndex == BiomesManager::snow)
\t\t\t\t\t{
\t\t\t\t\t\tstructuresChoice.push_back(igloo);
\t\t\t\t\t\tstructuresChoice.push_back(igloo);
\t\t\t\t\t}
\t\t\t\t\telse if (currentBiomeIndex == BiomesManager::plains)
\t\t\t\t\t{
\t\t\t\t\t\tstructuresChoice.push_back(treeHouse);
\t\t\t\t\t}
\t\t\t\t\telse if (currentBiomeIndex == BiomesManager::wasteLand)
\t\t\t\t\t{
\t\t\t\t\t\tstructuresChoice.push_back(smallStoneRuins);
\t\t\t\t\t\tstructuresChoice.push_back(smallStoneRuins);
\t\t\t\t\t\tstructuresChoice.push_back(goblinTower);
\t\t\t\t\t}
"""
replace_once(path, old, new)

old = """\t\t\t\t\tuint32_t randomValue = hash(c.x, c.z, seedHash++);
\t\t\t\t\tint index = randomValue % structuresChoice.size();
\t\t\t\t\tstructuresChoice[index](); // Call the selected function
"""
new = old + """

\t\t\t\t\t// Rare underground destination, deterministic from world seed/chunk.
\t\t\t\t\tuint32_t dungeonRoll = hash(c.x, c.z, seedHash++);
\t\t\t\t\tif (chunkDistanceFromCenter > 8 && (dungeonRoll % 24u) == 0u)
\t\t\t\t\t{
\t\t\t\t\t\tminesDungeon();
\t\t\t\t\t}
"""
replace_once(path, old, new)

# Lightweight server-side zombie variants: no new model/texture/network format.
path = "include/gameLayer/gameplay/zombie.h"
old = """struct ZombieServer: public ServerEntity<Zombie>
{

\tglm::vec2 direction = {};
"""
new = """struct ZombieServer: public ServerEntity<Zombie>
{
\tenum Variant : unsigned char
\t{
\t\tWalker = 0,
\t\tRunner,
\t\tBrute,
\t};

\tVariant variant = Walker;
\tfloat moveSpeedMultiplier = 1.f;

\tglm::vec2 direction = {};
"""
replace_once(path, old, new)

replace_once(
    "src/gameLayer/gameplay/zombie.cpp",
    "\tauto move = 2.f * deltaTime * direction;\n",
    "\t// Variant speed is server authoritative; clients receive the resulting position.\n"
    "\tauto move = (2.f * moveSpeedMultiplier) * deltaTime * direction;\n",
)

path = "src/gameLayer/multyPlayer/tick.cpp"
old = """\t\tZombieServer serverZombie = {};
\t\tserverZombie.entity = zombie;

\t\tc->entityData.zombies.insert({newId, serverZombie});
"""
new = """\t\tZombieServer serverZombie = {};
\t\tserverZombie.entity = zombie;

\t\t// Deterministic variants avoid adding bytes to the current spawn protocol.
\t\t// 70% walkers, 20% runners, 10% brutes.
\t\tconst unsigned int variantRoll = static_cast<unsigned int>(newId % 10u);
\t\tif (variantRoll == 0u)
\t\t{
\t\t\tserverZombie.variant = ZombieServer::Brute;
\t\t\tserverZombie.moveSpeedMultiplier = 0.72f;
\t\t\tserverZombie.entity.life = Life{320};
\t\t}
\t\telse if (variantRoll <= 2u)
\t\t{
\t\t\tserverZombie.variant = ZombieServer::Runner;
\t\t\tserverZombie.moveSpeedMultiplier = 1.45f;
\t\t\tserverZombie.entity.life = Life{125};
\t\t}

\t\tc->entityData.zombies.insert({newId, serverZombie});
"""
replace_once(path, old, new)

# Survival/farming integration without changing item IDs or save layout.
path = "src/gameLayer/gameplay/crafting.cpp"
old = """\t//food
\trecepie<2>(Item(ItemTypes::applePie, 1), {Item(ItemTypes::apple, 2),  Item(ItemTypes::wheat, 3)}).setRequiresCookingPot(),
"""
new = """\t//food / exploration provisions
\trecepie<2>(Item(ItemTypes::applePie, 1), {Item(ItemTypes::apple, 2),  Item(ItemTypes::wheat, 3)}).setRequiresCookingPot(),
\t// Farming storage loop: compact wheat into decorative hay, then unpack it without loss.
\trecepie<1>(Item(BlockTypes::hayBalde, 1), {Item(ItemTypes::wheat, 9)}).setRequiresWorkBench(),
\trecepie<1>(Item(ItemTypes::wheat, 9), {Item(BlockTypes::hayBalde, 1)}),
"""
replace_once(path, old, new)

Path("docs/EXPLORATION_CONTENT.md").write_text("""# Exploration Content v0.1

This update builds on Survival + Linux low-end work without removing existing gameplay.

## World landmarks
- Desert: pyramids and stone ruins.
- Snow: igloos.
- Plains: rare tree houses.
- Hayland: barns and rare taverns.
- Wasteland: denser ruins and goblin towers.
- Existing abandoned houses, training camps and goblin towers remain.
- Rare deterministic underground mine/dungeon destinations now appear away from spawn.

Generation remains seed/chunk deterministic for multiplayer compatibility and reuses assets already shipped with the project.

## Enemy variety
- Walker (70%): original baseline.
- Runner (20%): 125 HP, 1.45x speed.
- Brute (10%): 320 HP, 0.72x speed.

Variants intentionally reuse the current zombie model and texture so the feature adds almost no GPU/VRAM cost.

## Survival integration
- 9 wheat -> 1 hay bale at a workbench.
- 1 hay bale -> 9 wheat.
- Existing cooking, hunger, loot, furnace and equipment systems remain intact.

## Compatibility
No save format, inventory index, entity packet, block id or item id is renumbered by this update.
""")
