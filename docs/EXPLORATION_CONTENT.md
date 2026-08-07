# Exploration Content v0.1

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

## Structure loot
- Generated structure chests receive deterministic loot based on structure type and coordinates.
- Barns favour food/wheat; goblin towers favour cloth/fangs; pyramids favour coins/rare metal; igloos favour food; mines favour ingots; training camps favour arrows.
- Existing chest data is never overwritten.

## Survival integration
- 9 wheat -> 1 hay bale at a workbench.
- 1 hay bale -> 9 wheat.
- Existing cooking, hunger, loot, furnace and equipment systems remain intact.

## Compatibility
No save format, inventory index, entity packet, block id or item id is renumbered by this update.
