#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

constexpr std::uint32_t WORLD_DIFFICULTY_FORMAT_VERSION = 1;

enum class WorldDifficulty : std::uint8_t
{
	Peaceful = 0,
	Easy,
	Normal,
	Hard,
};

struct WorldDifficultySettings
{
	WorldDifficulty difficulty = WorldDifficulty::Normal;
	bool hardcore = false;

	void sanitize();
	bool operator==(const WorldDifficultySettings &other) const
	{
		return difficulty == other.difficulty && hardcore == other.hardcore;
	}
};

const char *getWorldDifficultyName(WorldDifficulty difficulty);
float getIncomingDamageMultiplier(const WorldDifficultySettings &settings);
float getSiegeEnemyMultiplier(const WorldDifficultySettings &settings);
int getStarvationHealthFloor(const WorldDifficultySettings &settings);
bool areNaturalSiegesEnabled(const WorldDifficultySettings &settings);
short scaleIncomingDamageForDifficulty(short difference,
	const WorldDifficultySettings &settings);

std::vector<unsigned char> formatWorldDifficultySettings(
	const WorldDifficultySettings &settings);
bool parseWorldDifficultySettings(const char *data, std::size_t size,
	WorldDifficultySettings &settings);
bool saveWorldDifficultySettings(const std::filesystem::path &worldRoot,
	const WorldDifficultySettings &settings);
bool loadWorldDifficultySettings(const std::filesystem::path &worldRoot,
	WorldDifficultySettings &settings);

void setServerWorldDifficultySettings(WorldDifficultySettings settings);
const WorldDifficultySettings &getServerWorldDifficultySettings();
void resetServerWorldDifficultySettings();

void setClientWorldDifficultySettings(WorldDifficultySettings settings);
const WorldDifficultySettings &getClientWorldDifficultySettings();
void resetClientWorldDifficultySettings();
