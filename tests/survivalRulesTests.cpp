#include <blocks.h>
#include <gameplay/entityId.h>
#include <gameplay/environmentMotion.h>
#include <gameplay/combatBalance.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

void assertFuncInternal(const char *, const char *, unsigned int, const char *)
{
	std::abort();
}

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	const std::uint64_t rawId = 0x0000123456789ABCULL;
	const std::uint64_t encoded = rawId | (std::uint64_t{5} << 56);
	REQUIRE(getEntityTypeFromEID(encoded) == 5);
	REQUIRE(getOnlyIdFromEID(encoded) == rawId);

	for (const BlockType type : {BlockTypes::crate, BlockTypes::smallCrate,
		BlockTypes::pot, BlockTypes::jar})
	{
		REQUIRE(isFragileContainer(type));
		REQUIRE(std::abs(getBlockBaseMineDuration(type) - 0.12f) < 0.0001f);
	}
	REQUIRE(!isFragileContainer(BlockTypes::cookingPot));
	REQUIRE(!isFragileContainer(BlockTypes::workBench));
	REQUIRE(getBlockBaseMineDuration(BlockTypes::crate) <
		getBlockBaseMineDuration(BlockTypes::wooden_plank));

	glm::vec3 velocity(10.f, -20.f, 8.f);
	glm::vec3 acceleration(4.f, -12.f, 2.f);
	applyCobwebMotion(velocity, acceleration, 0.1f);
	REQUIRE(std::abs(velocity.x) < 5.f);
	REQUIRE(std::abs(velocity.z) < 5.f);
	REQUIRE(velocity.y >= -2.5f);
	REQUIRE(std::abs(acceleration.x - 0.6f) < 0.0001f);
	REQUIRE(ZOMBIE_WALKER_ATTACK_COOLDOWN > 0.f);
	REQUIRE(ZOMBIE_RUNNER_ATTACK_COOLDOWN > 0.f);
	REQUIRE(ZOMBIE_BRUTE_ATTACK_COOLDOWN >= 1.05f);
	REQUIRE(ZOMBIE_BRUTE_ATTACK_COOLDOWN <= 2.35f);

	std::cout << "Survival rule tests passed.\n";
	return 0;
}
