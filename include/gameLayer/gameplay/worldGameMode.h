#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

// The world game mode is only the default assigned to players that do not yet
// have a player save in this world. Existing player saves keep their own mode.
enum class WorldGameMode : std::uint8_t
{
	Survival = 0,
	Creative = 1,
};

struct WorldGameModeSettings
{
	WorldGameMode mode = WorldGameMode::Survival;

	void sanitize()
	{
		if (mode != WorldGameMode::Survival && mode != WorldGameMode::Creative)
		{
			mode = WorldGameMode::Survival;
		}
	}
};

inline const char *getWorldGameModeName(WorldGameMode mode)
{
	return mode == WorldGameMode::Creative ? "Creative" : "Survival";
}

inline bool saveWorldGameModeSettings(const std::filesystem::path &worldRoot,
	WorldGameModeSettings settings)
{
	settings.sanitize();
	std::error_code error;
	std::filesystem::create_directories(worldRoot, error);
	if (error) { return false; }

	std::ofstream file(worldRoot / "worldGameMode", std::ios::trunc);
	if (!file) { return false; }
	file << "MIE_WORLD_GAME_MODE 1\n";
	file << static_cast<int>(settings.mode) << '\n';
	return static_cast<bool>(file);
}

inline bool loadWorldGameModeSettings(const std::filesystem::path &worldRoot,
	WorldGameModeSettings &settings)
{
	settings = {};
	std::ifstream file(worldRoot / "worldGameMode");
	if (!file) { return false; }

	std::string magic;
	int version = 0;
	int mode = 0;
	if (!(file >> magic >> version) || magic != "MIE_WORLD_GAME_MODE" || version != 1 ||
		!(file >> mode) || (mode != static_cast<int>(WorldGameMode::Survival) &&
			mode != static_cast<int>(WorldGameMode::Creative)))
	{
		settings = {};
		return false;
	}

	settings.mode = static_cast<WorldGameMode>(mode);
	settings.sanitize();
	return true;
}
