#include <blocks.h>
#include <chunk.h>
#include <gameplay/entityId.h>
#include <gameplay/environmentMotion.h>
#include <gameplay/combatBalance.h>
#include <multyPlayer/serverActionValidation.h>
#include <multyPlayer/dataIntegrity.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <unordered_set>

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

	// v0.9.2: region IDs must round-trip all signed 32-bit quadrants.
	for (const glm::ivec2 region : {glm::ivec2{0, 0}, glm::ivec2{1, -1},
		glm::ivec2{-1, 1}, glm::ivec2{-2048, -4096}, glm::ivec2{250000, -125000}})
	{
		int decodedX = 0;
		int decodedZ = 0;
		getRegionCenterFromId(getRegionId(region.x, region.y), decodedX, decodedZ);
		REQUIRE(decodedX == region.x);
		REQUIRE(decodedZ == region.y);
	}

	// Nearby chunk coordinates used to collapse into almost the same hash bucket.
	Ivec2Hash chunkHash;
	std::unordered_set<size_t> nearbyHashes;
	for (int x = -8; x <= 8; ++x)
	{
		for (int z = -8; z <= 8; ++z)
		{
			nearbyHashes.insert(chunkHash({x, z}));
		}
	}
	REQUIRE(nearbyHashes.size() >= 280);

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

	// Existing resistance now participates in combat knockback.
	REQUIRE(std::abs(getKnockBackResistanceMultiplier(0.f) - 1.f) < 0.0001f);
	REQUIRE(std::abs(getKnockBackResistanceMultiplier(25.f) - 0.75f) < 0.0001f);
	REQUIRE(std::abs(getKnockBackResistanceMultiplier(100.f)) < 0.0001f);
	REQUIRE(std::abs(getKnockBackResistanceMultiplier(-100.f) - 2.f) < 0.0001f);
	REQUIRE(std::abs(getKnockBackResistanceMultiplier(-1000.f) - 4.f) < 0.0001f);

	// v0.9.3: the server must never accept block actions at arbitrary distance
	// or outside the valid world height. Normal 20-block client raycasts remain valid.
	using namespace mie::serverValidation;
	REQUIRE(isWorldBlockPositionSane({-250000, 0, 125000}));
	REQUIRE(isWorldBlockPositionSane({0, CHUNK_HEIGHT - 1, 0}));
	REQUIRE(!isWorldBlockPositionSane({0, -1, 0}));
	REQUIRE(!isWorldBlockPositionSane({0, CHUNK_HEIGHT, 0}));
	REQUIRE(!isWorldBlockPositionSane({30000001, 64, 0}));
	REQUIRE(isBlockActionWithinReach({0.0, 64.0, 0.0}, {20, 64, 0}));
	REQUIRE(isBlockActionWithinReach({0.0, 64.0, 0.0}, {21, 64, 0}));
	REQUIRE(!isBlockActionWithinReach({0.0, 64.0, 0.0}, {30, 64, 0}));
	REQUIRE(!isBlockActionWithinReach({NAN, 64.0, 0.0}, {0, 64, 0}));
	REQUIRE(isServerBlockActionPositionValid({-100.0, 80.0, -100.0}, {-100, 80, -100}));
	REQUIRE(!isServerBlockActionPositionValid({0.0, 64.0, 0.0}, {0, CHUNK_HEIGHT, 0}));

	// v0.9.3.1: entities that step beyond an unloaded streaming boundary are
	// retained inside the last authoritative chunk instead of being discarded.
	const glm::ivec2 retainedChunk{2, -3};
	const glm::dvec3 escapedPosition{
		static_cast<double>((retainedChunk.x + 1) * CHUNK_SIZE) + 4.0,
		64.0,
		static_cast<double>(retainedChunk.y * CHUNK_SIZE) - 4.0};
	const glm::dvec3 clampedPosition = mie::dataIntegrity::clampEntityPositionToChunk(
		escapedPosition, retainedChunk);
	REQUIRE(determineChunkThatIsEntityIn(clampedPosition) == retainedChunk);

	REQUIRE(mie::dataIntegrity::isDroppedItemSpawnPositionValid(
		{0.0, 64.0, 0.0}, {3.0, 64.0, 2.0}));
	REQUIRE(!mie::dataIntegrity::isDroppedItemSpawnPositionValid(
		{0.0, 64.0, 0.0}, {20.0, 64.0, 0.0}));
	REQUIRE(!mie::dataIntegrity::isDroppedItemSpawnPositionValid(
		{0.0, 64.0, 0.0}, {NAN, 64.0, 0.0}));

	const auto collisionQuery = mie::dataIntegrity::makeBlockPlacementCollisionQuery(
		{128.0, 70.0, -64.0}, {0.8f, 1.8f, 0.8f}, {4, 70, 9});
	REQUIRE(collisionQuery.entityPosition == glm::dvec3(128.0, 70.0, -64.0));
	REQUIRE(collisionQuery.blockPosition == glm::ivec3(4, 70, 9));

	std::cout << "Survival rule tests passed.\n";
	return 0;
}