#pragma once

#include <cstddef>
#include <cstdint>

#include <gameplay/worldDifficulty.h>

struct NaturalSpawnPressure
{
	bool night = false;
	bool hostileSpawningEnabled = false;
	std::size_t passiveCap = 0;
	std::size_t hostileCap = 0;
	float passiveIntervalMin = 8.f;
	float passiveIntervalMax = 14.f;
	float hostileIntervalMin = 9.f;
	float hostileIntervalMax = 14.f;
};

bool isNightSpawnPhase(float dayPhase);
NaturalSpawnPressure getNaturalSpawnPressure(float dayPhase,
	std::uint64_t visibleDay, std::size_t activePlayers,
	std::size_t survivalPlayers,
	WorldDifficultySettings difficulty, bool siegeActive);
