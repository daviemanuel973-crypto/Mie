# Survival Mode v0.1

This branch turns the existing survival scaffolding into the default playable progression while preserving Creative mode for administration/testing.

## Core loop

1. New players spawn in Survival with an empty inventory.
2. Break trees by hand and craft planks/sticks.
3. Craft a work bench, chest and furnace.
4. Smelt copper/lead/iron/silver/gold.
5. Craft tools, weapons and armour.
6. Food restores hunger; high hunger enables natural regeneration.
7. Hunger drains slowly with time and movement. At zero hunger, starvation damages the player down to 10 HP.
8. Equipped armour is now serialized and contributes to server-side damage reduction.

## Compatibility

- `/gamemode creative` remains available.
- Existing legacy workbench/furnace recipes remain available in addition to the new survival-friendly recipes.
- Old inventory payloads remain readable because armour fields are appended and treated as optional on decode.
- Existing effects, food healing, mobs, mining, block drops, chests, coins and stations are preserved.

## Next survival milestones

- persistent player identity/inventory between reconnects;
- difficulty selection and hardcore rules;
- day/night spawn pressure;
- tool durability;
- dedicated furnace fuel/progress UI;
- recipe discovery/book;
- farming expansion and additional cooked foods;
- bed/spawn-point system.
