# Survival Mode v0.3

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

## Persistent players

- Every installation creates a stable 128-bit player identity on first launch.
- Each world stores that identity's inventory, equipped armour, position, life, hunger and game mode.
- The server restores the snapshot on reconnect instead of creating a fresh Survival player.
- Player snapshots are saved every 30 seconds, on disconnect and during clean server shutdown.
- Every snapshot is versioned, size-limited and written with a checksum plus a recovery copy.
- Chunk terrain now uses checksummed primary/recovery files; failed writes stay dirty and are retried instead of being discarded.
- A server rejects simultaneous sessions using the same identity to prevent save rollback.
- Existing worlds remain compatible; their first reconnect simply creates the first player snapshot.

## Survival milestone status

- difficulty selection and hardcore rules — implemented for v0.8.0;
- day/night spawn pressure — implemented for v0.8.0;
- tool and weapon durability — implemented for v0.8.0;
- dedicated furnace fuel/progress UI;
- recipe discovery/book;
- farming expansion and additional cooked foods;
- bed/spawn-point system.
