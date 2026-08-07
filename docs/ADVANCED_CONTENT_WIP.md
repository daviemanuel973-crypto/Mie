# Advanced Survival & Exploration — WIP branch

Branch: `agent/dev-content-expansion-wip`

This branch is intentionally allowed to be temporarily non-runnable while the next content systems are defined and integrated. The shipping branches remain separate.

## Source of truth
`resources/gameData/content/advanced_survival_v02.json` contains stable string IDs and balance data. During WIP we deliberately avoid assigning new numeric `ItemTypes`, `BlockTypes`, packet IDs or inventory indices. Numeric IDs will only be assigned during stabilization after save/network compatibility is reviewed.

## Content currently specified

### Agriculture
- Wheat, carrot, potato and rare moonroot crops.
- Growth stages, growth time, preferred biomes and water-growth multipliers.

### Cooking
- Campfire and cooking-pot recipes.
- Baked potato, vegetable stew, hearty stew and berry porridge.
- Hunger restoration and optional temporary buffs.

### Backpacks
- Small: +6 slots.
- Field: +12 slots.
- Explorer: +18 slots.
- Integration must expand capacity without renumbering the current base inventory.

### Base progression
- Shelter -> Camp -> Base -> Fortress.
- Requirements are based on enclosed construction volume, storage and placed workstations.
- Tiers unlock recipes/upgrades instead of imposing a fixed house shape.

### Workstations and equipment progression
- Campfire, advanced workbench, anvil and alchemy table.
- Reinforced, balanced and tempered weapon-upgrade tracks.
- Repair/upgrades must use item metadata rather than new item IDs for every quality level.

### Exploration treasures
- Ancient lantern.
- Surveyor compass.
- Miner's charm.
- Ancient shard.
- Vault key.

### Discovery journal and map
- Discoveries for biomes, structures, enemies and resources.
- Visited chunks are intended to persist as fog-of-war map data.
- POI markers appear only after discovery.

### Dynamic events
- Heavy fog.
- Quiet night.
- Blood moon.
- Goblin raid.
- Meteor event.

### Cave expansion
- Normal, lush, crystal, ruined and abyssal cave themes.
- Depth ranges and weights total 100%.
- Future underground landmarks include deep vaults and ancient corridors.

### Night danger
- Night increases hostile spawn pressure.
- Runner and Brute probabilities gain small night bonuses.

## Integration order
1. Allocate/verify assets and stable engine IDs without renumbering existing IDs.
2. Persistent crop block data + save/load + multiplayer replication.
3. Cooking/workstation recipes and food buffs.
4. Backpack capacity + inventory serialization versioning.
5. Discovery journal + visited-chunk map persistence.
6. World-event state in the authoritative server clock.
7. Base-tier detection and unlock state.
8. Cave themes/underground POIs.
9. Equipment upgrade metadata and anvil UI.
10. Full Windows + Linux/Flatpak stabilization and migration tests.

## Validation policy
`tools/validate_content_manifest.py` validates IDs, weights, ranges and cross-references without needing the game to compile. The WIP CI intentionally checks that `advancedSurvivalContent.cpp` is **not** added to the shipping CMake target yet. Once the data contract is stable, a separate stabilization branch will wire these systems into the real build and run save/network/gameplay tests.
