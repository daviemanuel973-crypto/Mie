#!/usr/bin/env python3
"""Validate the WIP advanced survival content manifest without requiring the game build."""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "resources" / "gameData" / "content" / "advanced_survival_v02.json"

ID_SECTIONS = (
    "crops", "cooking_recipes", "backpacks", "base_tiers", "workstations",
    "weapon_upgrades", "treasures", "discoveries", "world_events",
    "cave_themes", "poi_expansion",
)

def require(cond: bool, errors: list[str], message: str) -> None:
    if not cond:
        errors.append(message)

def main() -> int:
    errors: list[str] = []
    try:
        data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"[content-validation] cannot load {MANIFEST}: {exc}", file=sys.stderr)
        return 2

    require(data.get("schema_version") == 1, errors, "schema_version must be 1")
    require(data.get("status") == "development", errors, "WIP manifest must keep status=development")

    all_ids: set[str] = set()
    section_ids: dict[str, set[str]] = {}
    for section in ID_SECTIONS:
        values = data.get(section)
        require(isinstance(values, list) and values, errors, f"{section} must be a non-empty array")
        current: set[str] = set()
        for index, entry in enumerate(values or []):
            ident = entry.get("id") if isinstance(entry, dict) else None
            require(isinstance(ident, str) and "." in ident, errors, f"{section}[{index}] has invalid id")
            if isinstance(ident, str):
                require(ident not in current, errors, f"duplicate id in {section}: {ident}")
                require(ident not in all_ids, errors, f"duplicate id across manifest: {ident}")
                current.add(ident)
                all_ids.add(ident)
        section_ids[section] = current

    for crop in data.get("crops", []):
        require(2 <= crop.get("growth_stages", 0) <= 8, errors, f"{crop['id']}: growth_stages must be 2..8")
        require(crop.get("growth_seconds", 0) >= 60, errors, f"{crop['id']}: growth_seconds too small")
        require(bool(crop.get("biomes")), errors, f"{crop['id']}: at least one biome required")
        require(crop.get("water_bonus", 0) >= 1.0, errors, f"{crop['id']}: water_bonus must be >= 1")

    for recipe in data.get("cooking_recipes", []):
        require(bool(recipe.get("inputs")), errors, f"{recipe['id']}: recipe needs inputs")
        output = recipe.get("output")
        require(isinstance(output, list) and len(output) == 2 and output[1] > 0, errors, f"{recipe['id']}: invalid output")
        require(recipe.get("hunger", 0) > 0, errors, f"{recipe['id']}: hunger must be positive")

    previous_slots = 0
    previous_tier = 0
    for backpack in data.get("backpacks", []):
        require(backpack.get("extra_slots", 0) > previous_slots, errors, f"{backpack['id']}: slots must strictly increase")
        require(backpack.get("tier", 0) > previous_tier, errors, f"{backpack['id']}: tiers must strictly increase")
        previous_slots = backpack.get("extra_slots", 0)
        previous_tier = backpack.get("tier", 0)

    ranks = [entry.get("rank") for entry in data.get("base_tiers", [])]
    require(ranks == list(range(1, len(ranks) + 1)), errors, "base_tiers ranks must be contiguous starting at 1")
    known_stations = section_ids.get("workstations", set()) | {"station.workbench", "station.cooking_pot"}
    for tier in data.get("base_tiers", []):
        for station in tier.get("requirements", {}).get("workstations", []):
            require(station in known_stations, errors, f"{tier['id']}: unknown workstation {station}")

    for upgrade in data.get("weapon_upgrades", []):
        require(upgrade.get("station") in known_stations, errors, f"{upgrade['id']}: unknown upgrade station")
        require(1 <= upgrade.get("max_level", 0) <= 5, errors, f"{upgrade['id']}: invalid max_level")

    rarities = {"common", "uncommon", "rare", "epic", "legendary"}
    for treasure in data.get("treasures", []):
        require(treasure.get("rarity") in rarities, errors, f"{treasure['id']}: invalid rarity")
        require(bool(treasure.get("source")), errors, f"{treasure['id']}: source required")

    for event in data.get("world_events", []):
        require(event.get("weight", 0) > 0, errors, f"{event['id']}: event weight must be positive")
        duration = event.get("duration_seconds")
        require(isinstance(duration, list) and len(duration) == 2 and 0 < duration[0] <= duration[1], errors, f"{event['id']}: invalid duration range")

    cave_weights = sum(entry.get("weight", 0) for entry in data.get("cave_themes", []))
    require(cave_weights == 100, errors, f"cave theme weights must total 100, got {cave_weights}")
    for cave in data.get("cave_themes", []):
        depth = cave.get("depth")
        require(isinstance(depth, list) and len(depth) == 2 and 0 <= depth[0] < depth[1], errors, f"{cave['id']}: invalid depth range")

    for poi in data.get("poi_expansion", []):
        rarity = poi.get("rarity", -1)
        require(0 < rarity < 1, errors, f"{poi['id']}: rarity must be between 0 and 1")
        require(bool(poi.get("biomes")), errors, f"{poi['id']}: biomes required")

    night = data.get("night_danger", {})
    require(0 <= night.get("start_hour", -1) < 24, errors, "night_danger.start_hour invalid")
    require(0 <= night.get("end_hour", -1) < 24, errors, "night_danger.end_hour invalid")
    require(night.get("hostile_spawn_multiplier", 0) >= 1.0, errors, "night danger must not reduce hostile spawns")

    map_system = data.get("map_system", {})
    require(1 <= map_system.get("reveal_radius_chunks", 0) <= 4, errors, "map reveal radius must be 1..4 chunks")
    require(map_system.get("poi_markers_require_discovery") is True, errors, "POI markers must require discovery")

    if errors:
        print(f"[content-validation] FAILED: {len(errors)} issue(s)", file=sys.stderr)
        for issue in errors:
            print(f" - {issue}", file=sys.stderr)
        return 1

    print("[content-validation] OK")
    print(f"  stable content ids: {len(all_ids)}")
    print(f"  crops: {len(data['crops'])}")
    print(f"  cooking recipes: {len(data['cooking_recipes'])}")
    print(f"  backpacks: {len(data['backpacks'])}")
    print(f"  world events: {len(data['world_events'])}")
    print(f"  cave themes: {len(data['cave_themes'])}")
    print(f"  discoveries: {len(data['discoveries'])}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
