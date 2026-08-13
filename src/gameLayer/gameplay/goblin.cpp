#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtx/transform.hpp>
#include <gameplay/goblin.h>
#include <multyPlayer/serverChunkStorer.h>
#include <iostream>
#include <glm/gtx/quaternion.hpp>
#include <rendering/model.h>

void Goblin::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	updateForces(deltaTime, true);
	resolveConstrainsAndUpdatePositions(chunkGetter, deltaTime, getColliderSize());
}

glm::vec3 Goblin::getColliderSize()
{
	return getMaxColliderSize();
}

glm::vec3 Goblin::getMaxColliderSize()
{
	return glm::vec3(0.8, 1.8, 0.8);
}

void GoblinClient::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	entityBuffered.update(deltaTime, chunkGetter);
}

void GoblinClient::setEntityMatrix(glm::mat4 *skinningMatrix)
{
	(void)skinningMatrix;
}

int GoblinClient::getTextureIndex()
{
	return ModelsManager::TexturesLoaded::GoblinTexture;
}

void GoblinServer::appendDataToDisk(std::ofstream &f, std::uint64_t eId)
{
	(void)f;
	(void)eId;
}

static unsigned int getGoblinVariantRoll(std::uint64_t eId)
{
	std::uint64_t value = eId + 0xD1B54A32D192ED03ULL;
	value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
	value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
	value ^= value >> 31;
	return static_cast<unsigned int>(value % 100ULL);
}

void GoblinServer::configureVariant(std::uint64_t eId)
{
	if (variantConfigured) { return; }

	const unsigned int roll = getGoblinVariantRoll(eId);
	const float oldMaxLife = entity.life.maxLife > 0.f ? entity.life.maxLife : 30.f;
	const float lifePercent = glm::clamp(entity.life.life / oldMaxLife, 0.f, 1.f);
	float newMaxLife = 30.f;

	if (roll < 75)
	{
		variant = Common;
		moveSpeedMultiplier = 1.f;
		newMaxLife = 30.f;
	}
	else if (roll < 97)
	{
		variant = Warrior;
		moveSpeedMultiplier = 1.08f;
		newMaxLife = 48.f;
	}
	else
	{
		variant = Chief;
		moveSpeedMultiplier = 0.92f;
		newMaxLife = 90.f;
	}

	entity.life.maxLife = newMaxLife;
	entity.life.life = glm::clamp(newMaxLife * lifePercent, 0.f, newMaxLife);
	variantConfigured = true;
}

void GoblinServer::forceTarget(std::uint64_t playerId)
{
	basicEnemyBehaviour.playerLockedOn = playerId;
	basicEnemyBehaviour.currentState = BasicEnemyBehaviour::stateTargetedPlayer;
	basicEnemyBehaviour.worriedTimer = 60.f;
}

static bool hasNearbyGoblinChief(ServerChunkStorer &serverChunkStorer,
	const glm::dvec3 &position, std::uint64_t yourEID)
{
	constexpr double chiefProtectionRadiusSquared = 18.0 * 18.0;
	for (const auto &chunkEntry : serverChunkStorer.savedChunks)
	{
		const SavedChunk *chunk = chunkEntry.second;
		if (!chunk || !chunk->otherData.withinSimulationDistance) { continue; }

		for (const auto &goblinEntry : chunk->entityData.goblins)
		{
			if (goblinEntry.first == yourEID) { continue; }
			const GoblinServer &other = goblinEntry.second;
			if (!other.variantConfigured || other.variant != GoblinServer::Chief) { continue; }

			const glm::dvec3 delta = other.getPosition() - position;
			if (glm::dot(delta, delta) <= chiefProtectionRadiusSquared) { return true; }
		}
	}
	return false;
}

bool GoblinServer::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
	ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
	std::unordered_set<std::uint64_t> &othersDeleted,
	std::unordered_map<std::uint64_t, std::unordered_map<glm::ivec3, PathFindingNode>> &pathFindingSurvival,
	std::unordered_map<std::uint64_t, glm::dvec3> &playersPositionSurvival,
	std::unordered_map < std::uint64_t, Client *> &allClients)
{
	configureVariant(yourEID);

	const bool commonProtectedByChief = variant == Common &&
		hasNearbyGoblinChief(serverChunkStorer, getPosition(), yourEID);
	const bool aggressive = variant != Common || commonProtectedByChief;

	if (!aggressive)
	{
		// Common goblins are neutral society members when they are not accompanying a Chief.
		// Clear any stale target so a Chief leaving the group immediately restores neutrality.
		basicEnemyBehaviour.playerLockedOn = 0;
		basicEnemyBehaviour.worriedTimer = 0.f;
		basicEnemyBehaviour.hitTimer = 0.f;
		basicEnemyBehaviour.currentState = BasicEnemyBehaviour::stateStaying;
		entity.animationStateServer.runningTime = 0;
		entity.animationStateServer.attacked = 0;
		wantToLookDirection = entity.getLookDirection();
	}
	else
	{
		BasicEnemyBehaviourOtherSettings settings;
		settings.hearBonus = -0.1f;
		settings.runSpeed = 2.f * moveSpeedMultiplier;
		settings.searchDistance = commonProtectedByChief ? 14.f : 16.f;

		if (variant == Warrior)
		{
			settings.hearBonus = 0.f;
			settings.searchDistance = 16.f;
		}
		else if (variant == Chief)
		{
			settings.hearBonus = 0.06f;
			settings.sightBonus = 0.1f;
			settings.searchDistance = 20.f;
		}

		basicEnemyBehaviour.update(this, deltaTime, chunkGetter, serverChunkStorer, rng, yourEID,
			othersDeleted, pathFindingSurvival, playersPositionSurvival, getPosition(), allClients, settings);
	}

	doCollisionWithOthers(getPosition(), entity.getMaxColliderSize(), entity.forces,
		serverChunkStorer, yourEID);
	entity.update(deltaTime, chunkGetter);
	return true;
}

WeaponStats GoblinServer::getWeaponStats()
{
	WeaponStats weaponStats;
	weaponStats.armourPenetration = 1.f;
	weaponStats.range = 1.5f;
	weaponStats.critChance = 0.1f;
	weaponStats.accuracy = 8.f;

	switch (variant)
	{
	case Warrior:
		weaponStats.damage = 8.f;
		weaponStats.critDamage = 12.f;
		weaponStats.surprizeDamage = 11.f;
		weaponStats.speed = 1.8f;
		weaponStats.knockBack = 3.f;
		weaponStats.armourPenetration = 2.f;
		break;
	case Chief:
		weaponStats.damage = 12.f;
		weaponStats.critDamage = 18.f;
		weaponStats.surprizeDamage = 16.f;
		weaponStats.speed = 1.2f;
		weaponStats.range = 1.8f;
		weaponStats.knockBack = 5.f;
		weaponStats.armourPenetration = 3.f;
		weaponStats.accuracy = 9.f;
		break;
	case Common:
	default:
		weaponStats.damage = 6.f;
		weaponStats.critDamage = 9.f;
		weaponStats.surprizeDamage = 9.f;
		weaponStats.speed = 1.f;
		weaponStats.knockBack = 2.f;
		break;
	}

	return weaponStats;
}

LootTable goblinLootTable
{
	{LootEntry{1, 1, 4, ItemTypes::cloth}, LootEntry{30, 7, 14, ItemTypes::cloth}},
	glm::ivec2{30, 75},
	0.30f,
	{LootEntry{1, 1, 2, ItemTypes::fang}, LootEntry{30, 5, 6, ItemTypes::fang}, LootEntry{3, 1, 1, ItemTypes::apple}},
};

LootTable goblinWarriorLootTable
{
	{LootEntry{1, 2, 5, ItemTypes::cloth}, LootEntry{2, 1, 3, ItemTypes::fang}},
	glm::ivec2{55, 145},
	0.50f,
	{LootEntry{1, 1, 2, ItemTypes::copperIngot}, LootEntry{6, 1, 1, ItemTypes::ironIngot}, LootEntry{8, 1, 1, ItemTypes::healingPotion}},
};

LootTable goblinChiefLootTable
{
	{LootEntry{1, 3, 7, ItemTypes::cloth}, LootEntry{1.5f, 2, 5, ItemTypes::fang}},
	glm::ivec2{180, 420},
	0.85f,
	{LootEntry{1, 2, 4, ItemTypes::ironIngot}, LootEntry{3, 1, 2, ItemTypes::silverIngot}, LootEntry{5, 1, 1, ItemTypes::goldIngot}, LootEntry{5, 1, 2, ItemTypes::healingPotion}},
};

LootTable &GoblinServer::getLootTable()
{
	switch (variant)
	{
	case Warrior: return goblinWarriorLootTable;
	case Chief: return goblinChiefLootTable;
	case Common:
	default: return goblinLootTable;
	}
}
