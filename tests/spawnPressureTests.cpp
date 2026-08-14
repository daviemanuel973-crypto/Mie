#include <gameplay/spawnPressure.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	REQUIRE(!isNightSpawnPhase(0.25f));
	REQUIRE(isNightSpawnPhase(0.5f));
	REQUIRE(isNightSpawnPhase(-0.25f));
	REQUIRE(!isNightSpawnPhase(std::numeric_limits<float>::quiet_NaN()));

	WorldDifficultySettings normal;
	const auto day = getNaturalSpawnPressure(0.3f, 1, 1, 1, normal, false);
	REQUIRE(!day.night);
	REQUIRE(!day.hostileSpawningEnabled);
	REQUIRE(day.passiveCap == 12);
	REQUIRE(day.hostileCap == 0);

	const auto night = getNaturalSpawnPressure(0.75f, 1, 1, 1, normal, false);
	REQUIRE(night.night);
	REQUIRE(night.hostileSpawningEnabled);
	REQUIRE(night.passiveCap == 6);
	REQUIRE(night.hostileCap == 5);
	REQUIRE(night.hostileIntervalMin == 9.f);

	WorldDifficultySettings easy;
	easy.difficulty = WorldDifficulty::Easy;
	const auto easyNight = getNaturalSpawnPressure(0.75f, 7, 2, 2, easy, false);
	REQUIRE(easyNight.hostileCap == 8);
	REQUIRE(easyNight.hostileIntervalMin == 12.f);

	WorldDifficultySettings hard;
	hard.difficulty = WorldDifficulty::Hard;
	const auto hardNight = getNaturalSpawnPressure(0.75f, 13, 2, 2, hard, false);
	REQUIRE(hardNight.hostileCap == 18);
	REQUIRE(hardNight.hostileIntervalMin == 4.f);
	const auto hardGroup = getNaturalSpawnPressure(0.75f, 1, 6, 6, hard, false);
	REQUIRE(hardGroup.hostileCap == 42);

	WorldDifficultySettings hardcore;
	hardcore.hardcore = true;
	const auto hardcoreNight = getNaturalSpawnPressure(0.75f, 1, 1, 1, hardcore, false);
	REQUIRE(hardcoreNight.hostileCap == 7);
	REQUIRE(hardcoreNight.hostileIntervalMin == 6.f);

	WorldDifficultySettings peaceful;
	peaceful.difficulty = WorldDifficulty::Peaceful;
	const auto peacefulNight = getNaturalSpawnPressure(0.75f, 20, 4, 4, peaceful, false);
	REQUIRE(peacefulNight.night);
	REQUIRE(!peacefulNight.hostileSpawningEnabled);
	REQUIRE(peacefulNight.hostileCap == 0);

	const auto siegeNight = getNaturalSpawnPressure(0.75f, 20, 4, 4, hard, true);
	REQUIRE(!siegeNight.hostileSpawningEnabled);
	REQUIRE(siegeNight.hostileCap == 0);

	const auto emptyServer = getNaturalSpawnPressure(0.75f, 20, 0, 0, hard, false);
	REQUIRE(!emptyServer.hostileSpawningEnabled);
	REQUIRE(emptyServer.passiveCap == 0);
	REQUIRE(emptyServer.hostileCap == 0);

	const auto creativeOnly = getNaturalSpawnPressure(0.75f, 20, 2, 0, hard, false);
	REQUIRE(creativeOnly.passiveCap == 12);
	REQUIRE(!creativeOnly.hostileSpawningEnabled);

	const auto saturated = getNaturalSpawnPressure(0.75f, 999, 1000, 1000, hard, false);
	REQUIRE(saturated.hostileCap == 48);

	std::cout << "Day/night spawn pressure tests passed.\n";
	return 0;
}
