#include <gameplay/spawnPressure.h>

#include <algorithm>
#include <cmath>

namespace
{
	float normalizedDayPhase(float dayPhase)
	{
		if (!std::isfinite(dayPhase)) { return 0.25f; }
		return dayPhase - std::floor(dayPhase);
	}

	std::size_t saturatingPlayerBudget(std::size_t players,
		std::size_t perPlayer, std::size_t ageBonus)
	{
		constexpr std::size_t maximumHostiles = 48;
		const std::size_t remainingBudget = maximumHostiles -
			std::min(ageBonus, maximumHostiles);
		if (perPlayer == 0 || players > remainingBudget / perPlayer)
		{
			return maximumHostiles;
		}
		return std::min(players * perPlayer + ageBonus, maximumHostiles);
	}
}

bool isNightSpawnPhase(float dayPhase)
{
	return normalizedDayPhase(dayPhase) >= 0.5f;
}

NaturalSpawnPressure getNaturalSpawnPressure(float dayPhase,
	std::uint64_t visibleDay, std::size_t activePlayers,
	std::size_t survivalPlayers,
	WorldDifficultySettings difficulty, bool siegeActive)
{
	difficulty.sanitize();
	NaturalSpawnPressure result;
	result.night = isNightSpawnPhase(dayPhase);

	if (activePlayers != 0 && result.night)
	{
		result.passiveCap = std::max<std::size_t>(4, activePlayers * 6);
		result.passiveIntervalMin = 14.f;
		result.passiveIntervalMax = 22.f;
	}
	else if (activePlayers != 0)
	{
		result.passiveCap = std::max<std::size_t>(8, activePlayers * 12);
	}

	if (survivalPlayers == 0 || !result.night || siegeActive ||
		difficulty.difficulty == WorldDifficulty::Peaceful)
	{
		return result;
	}

	const std::uint64_t completedPressureSteps = visibleDay > 1 ?
		(visibleDay - 1) / 3 : 0;
	const std::size_t ageBonus = static_cast<std::size_t>(std::min<std::uint64_t>(
		completedPressureSteps, 4));
	std::size_t perPlayer = 5;
	float baseMinInterval = 9.f;
	float baseMaxInterval = 14.f;

	switch (difficulty.difficulty)
	{
	case WorldDifficulty::Easy:
		perPlayer = 3;
		baseMinInterval = 13.f;
		baseMaxInterval = 18.f;
		break;
	case WorldDifficulty::Hard:
		perPlayer = 7;
		baseMinInterval = 6.f;
		baseMaxInterval = 10.f;
		break;
	case WorldDifficulty::Normal:
		break;
	case WorldDifficulty::Peaceful:
		return result;
	}

	result.hostileSpawningEnabled = true;
	result.hostileCap = saturatingPlayerBudget(survivalPlayers, perPlayer, ageBonus);
	const float ageIntervalReduction = static_cast<float>(ageBonus) * 0.5f;
	result.hostileIntervalMin = std::max(4.f, baseMinInterval - ageIntervalReduction);
	result.hostileIntervalMax = std::max(result.hostileIntervalMin + 2.f,
		baseMaxInterval - ageIntervalReduction);
	return result;
}
