#include <gameplay/worldDifficulty.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	WorldDifficultySettings normal;
	REQUIRE(normal.difficulty == WorldDifficulty::Normal);
	REQUIRE(!normal.hardcore);
	REQUIRE(getStarvationHealthFloor(normal) == 10);
	REQUIRE(scaleIncomingDamageForDifficulty(-10, normal) == -10);

	WorldDifficultySettings peaceful;
	peaceful.difficulty = WorldDifficulty::Peaceful;
	REQUIRE(!areNaturalSiegesEnabled(peaceful));
	REQUIRE(getSiegeEnemyMultiplier(peaceful) == 0.65f);
	REQUIRE(scaleIncomingDamageForDifficulty(-10, peaceful) == -5);

	WorldDifficultySettings easy;
	easy.difficulty = WorldDifficulty::Easy;
	REQUIRE(getStarvationHealthFloor(easy) == 20);
	REQUIRE(scaleIncomingDamageForDifficulty(-10, easy) == -8);

	WorldDifficultySettings hardcore;
	hardcore.difficulty = WorldDifficulty::Easy;
	hardcore.hardcore = true;
	hardcore.sanitize();
	REQUIRE(hardcore.difficulty == WorldDifficulty::Hard);
	REQUIRE(getStarvationHealthFloor(hardcore) == 0);
	REQUIRE(scaleIncomingDamageForDifficulty(-10, hardcore) == -15);

	const auto encoded = formatWorldDifficultySettings(hardcore);
	WorldDifficultySettings decoded;
	REQUIRE(parseWorldDifficultySettings(reinterpret_cast<const char *>(encoded.data()),
		encoded.size(), decoded));
	REQUIRE(decoded == hardcore);
	REQUIRE(!parseWorldDifficultySettings(reinterpret_cast<const char *>(encoded.data()),
		encoded.size() - 1, decoded));

	auto invalid = encoded;
	invalid.back() = 2;
	REQUIRE(!parseWorldDifficultySettings(reinterpret_cast<const char *>(invalid.data()),
		invalid.size(), decoded));

	const auto testRoot = std::filesystem::temp_directory_path() /
		"mie-world-difficulty-contract";
	std::error_code error;
	std::filesystem::create_directories(testRoot, error);
	REQUIRE(!error);
	REQUIRE(saveWorldDifficultySettings(testRoot, hardcore));
	REQUIRE(loadWorldDifficultySettings(testRoot, decoded));
	REQUIRE(decoded == hardcore);
	std::filesystem::remove_all(testRoot, error);

	std::cout << "World difficulty and hardcore tests passed.\n";
	return 0;
}
