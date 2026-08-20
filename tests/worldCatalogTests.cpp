#include <worldCatalog.h>
#include <gameplay/worldGameMode.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed: " #condition " at line " << __LINE__ << "\n"; \
		return 1; \
	} } while (false)
}

int main()
{
	std::string normalized;
	std::string error;
	REQUIRE(normalizeWorldName("  My World  ", normalized, &error));
	REQUIRE(normalized == "My World");
	REQUIRE(!normalizeWorldName("../escape", normalized, &error));
	REQUIRE(!normalizeWorldName("bad/world", normalized, &error));
	REQUIRE(!normalizeWorldName("CON", normalized, &error));
	REQUIRE(!normalizeWorldName("con.txt", normalized, &error));
	REQUIRE(!normalizeWorldName("world.", normalized, &error));
	REQUIRE(!normalizeWorldName("   ", normalized, &error));

	const auto root = std::filesystem::temp_directory_path() / "mie-world-catalog-test";
	std::error_code filesystemError;
	std::filesystem::remove_all(root, filesystemError);
	std::filesystem::create_directories(root / "Older World" / "world", filesystemError);
	REQUIRE(!filesystemError);
	{
		std::ofstream settings(root / "Older World" / "worldGenSettings.wgenerator");
		settings << "seed: 12345;\n";
	}
	std::filesystem::create_directories(root / "New World", filesystemError);
	{
		std::ofstream seed(root / "New World" / "seed.txt");
		seed << "9876";
	}
	WorldDifficultySettings hardcore;
	hardcore.hardcore = true;
	REQUIRE(saveWorldDifficultySettings(root / "New World", hardcore));

	// Worlds created before the v0.9.3 game-mode setting remain Survival.
	WorldGameModeSettings legacyMode;
	REQUIRE(!loadWorldGameModeSettings(root / "Older World", legacyMode));
	REQUIRE(legacyMode.mode == WorldGameMode::Survival);
	REQUIRE(std::string(getWorldGameModeName(legacyMode.mode)) == "Survival");

	// New worlds persist either explicit initial mode independently from difficulty.
	WorldGameModeSettings creativeMode;
	creativeMode.mode = WorldGameMode::Creative;
	REQUIRE(saveWorldGameModeSettings(root / "New World", creativeMode));
	WorldGameModeSettings loadedMode;
	REQUIRE(loadWorldGameModeSettings(root / "New World", loadedMode));
	REQUIRE(loadedMode.mode == WorldGameMode::Creative);
	REQUIRE(std::string(getWorldGameModeName(loadedMode.mode)) == "Creative");

	WorldGameModeSettings survivalMode;
	survivalMode.mode = WorldGameMode::Survival;
	REQUIRE(saveWorldGameModeSettings(root / "New World", survivalMode));
	REQUIRE(loadWorldGameModeSettings(root / "New World", loadedMode));
	REQUIRE(loadedMode.mode == WorldGameMode::Survival);
	REQUIRE(saveWorldGameModeSettings(root / "New World", creativeMode));

	// Invalid/corrupt values never opt an old world into Creative.
	{
		std::ofstream invalid(root / "Older World" / "worldGameMode", std::ios::trunc);
		invalid << "MIE_WORLD_GAME_MODE 1\n99\n";
	}
	WorldGameModeSettings invalidMode;
	REQUIRE(!loadWorldGameModeSettings(root / "Older World", invalidMode));
	REQUIRE(invalidMode.mode == WorldGameMode::Survival);

	const auto now = std::filesystem::file_time_type::clock::now();
	std::filesystem::last_write_time(root / "Older World", now - std::chrono::hours(2), filesystemError);
	std::filesystem::last_write_time(root / "New World", now - std::chrono::hours(1), filesystemError);
	REQUIRE(!filesystemError);

	auto worlds = loadWorldCatalog(root, &error);
	REQUIRE(error.empty());
	REQUIRE(worlds.size() == 2);
	REQUIRE(worlds.front().folderName == "New World");

	auto findWorld = [&](const std::string &name) -> const WorldCatalogEntry*
	{
		for (const auto &world : worlds)
		{
			if (world.folderName == name) { return &world; }
		}
		return nullptr;
	};

	const auto *older = findWorld("Older World");
	const auto *newer = findWorld("New World");
	REQUIRE(older && newer);
	REQUIRE(older->hasSeed && older->seed == 12345);
	REQUIRE(older->hasGeneratedWorld);
	REQUIRE(!older->hasExplicitDifficulty);
	REQUIRE(older->difficultySettings.difficulty == WorldDifficulty::Normal);
	REQUIRE(newer->hasSeed && newer->seed == 9876);
	REQUIRE(newer->hasExplicitDifficulty);
	REQUIRE(newer->difficultySettings.hardcore);
	REQUIRE(newer->difficultySettings.difficulty == WorldDifficulty::Hard);
	REQUIRE(markWorldPlayed(root, "Older World", &error));
	REQUIRE(std::filesystem::exists(root / "Older World" / ".last_played"));
	worlds = loadWorldCatalog(root, &error);
	REQUIRE(error.empty());
	REQUIRE(worlds.front().folderName == "Older World");
	REQUIRE(!markWorldPlayed(root, "../escape", &error));
	REQUIRE(worldSeedFromText("12345", 7) == 12345);
	REQUIRE(worldSeedFromText("", 42) == 42);
	REQUIRE(worldSeedFromText("Mie", 42) == worldSeedFromText("Mie", 99));
	REQUIRE(worldSeedFromText("Mie", 42) > 0);

	std::filesystem::remove_all(root, filesystemError);
	return 0;
}
