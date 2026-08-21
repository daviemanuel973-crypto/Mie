#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtx/transform.hpp>
#include <gameplay/zombie.h>
#include <gameplay/basicEnemyBehaviour.h>
#include <gameplay/combatBalance.h>
#include <gameplay/serverSiegeRuntime.h>
#include <multyPlayer/serverChunkStorer.h>
#include <iostream>
#include <glm/gtx/quaternion.hpp>
#include <rendering/model.h>
#include <cmath>

namespace
{
	struct ZombieDiskData
	{
		Zombie entity;
		BasicEnemyBehaviour behaviour;
		std::uint8_t variant = ZombieServer::Walker;
		bool variantConfigured = false;
		float moveSpeedMultiplier = 1.f;
	};
}

static const auto frontHands = glm::rotate(glm::radians(90.f), glm::vec3{1.f, 0.f, 0.f});

void animatePlayerHandsZombie(glm::mat4 *poseVector, float &currentAngle, float deltaTime)
{
	(void)deltaTime;

	poseVector[2] = poseVector[2] * frontHands
		* glm::rotate(cosf(currentAngle) * 0.02f, glm::vec3{1.f, 0.f, 0.f})
		* glm::rotate(cosf(currentAngle + 5) * 0.05f, glm::vec3{0.f, 0.f, 1.f});
	poseVector[3] = poseVector[3] * frontHands
		* glm::rotate(cosf(currentAngle + 10) * 0.02f, glm::vec3{1.f, 0.f, 0.f})
		* glm::rotate(cosf(currentAngle + 15) * 0.05f, glm::vec3{0.f, 0.f, 1.f});
}

void Zombie::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	updateForces(deltaTime, true);
	resolveConstrainsAndUpdatePositions(chunkGetter, deltaTime, getColliderSize());
}

glm::vec3 Zombie::getColliderSize()
{
	return getMaxColliderSize();
}

glm::vec3 Zombie::getMaxColliderSize()
{
	return glm::vec3(0.8, 1.8, 0.8);
}

void ZombieClient::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	currentHandsAngle += deltaTime;
	if (currentHandsAngle > glm::radians(360.f))
	{
		currentHandsAngle -= glm::radians(360.f);
	}

	entityBuffered.update(deltaTime, chunkGetter);
}

void ZombieClient::setEntityMatrix(glm::mat4 *skinningMatrix)
{
	(void)skinningMatrix;
}

int ZombieClient::getTextureIndex()
{
	return ModelsManager::TexturesLoaded::ZombieTexture;
}

void ZombieServer::appendDataToDisk(std::ofstream &f, std::uint64_t eId)
{
	ensureBehaviour();
	ZombieDiskData data;
	data.entity = entity;
	data.behaviour = *basicEnemyBehaviour;
	data.variant = static_cast<std::uint8_t>(variant);
	data.variantConfigured = variantConfigured;
	data.moveSpeedMultiplier = moveSpeedMultiplier;
	basicEntitySave(f, Markers::zombie, eId, &data, sizeof(data));
}

bool ZombieServer::loadFromDisk(std::ifstream &f)
{
	ZombieDiskData data;
	if (!readData(f, &data, sizeof(data)) || data.variant > Brute ||
		!std::isfinite(data.moveSpeedMultiplier) || data.moveSpeedMultiplier <= 0.f)
	{
		return false;
	}
	entity = data.entity;
	basicEnemyBehaviour = std::make_shared<BasicEnemyBehaviour>(data.behaviour);
	variant = static_cast<Variant>(data.variant);
	variantConfigured = data.variantConfigured;
	moveSpeedMultiplier = data.moveSpeedMultiplier;
	return true;
}

void ZombieServer::ensureBehaviour()
{
	if (!basicEnemyBehaviour)
	{
		basicEnemyBehaviour = std::make_shared<BasicEnemyBehaviour>();
	}
}

bool ZombieServer::isUnaware()
{
	return basicEnemyBehaviour ? basicEnemyBehaviour->isUnaware() : true;
}

void ZombieServer::signalHit(glm::vec3 direction)
{
	ensureBehaviour();
	basicEnemyBehaviour->signalHit(direction, this);
}

static unsigned int getZombieVariantRoll(std::uint64_t eId)
{
	std::uint64_t value = eId + 0x9E3779B97F4A7C15ULL;
	value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
	value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
	value ^= value >> 31;
	return static_cast<unsigned int>(value % 100ULL);
}

void ZombieServer::configureVariant(std::uint64_t eId)
{
	ensureBehaviour();
	if (variantConfigured) { return; }

	const unsigned int roll = getZombieVariantRoll(eId);
	const float oldMaxLife = entity.life.maxLife > 0.f ? entity.life.maxLife : 40.f;
	const float lifePercent = glm::clamp(entity.life.life / oldMaxLife, 0.f, 1.f);
	float newMaxLife = 40.f;

	if (roll < 70)
	{
		variant = Walker;
		moveSpeedMultiplier = 1.f;
		newMaxLife = 40.f;
	}
	else if (roll < 90)
	{
		variant = Runner;
		moveSpeedMultiplier = 1.45f;
		newMaxLife = 32.f;
	}
	else
	{
		variant = Brute;
		moveSpeedMultiplier = 0.72f;
		newMaxLife = 80.f;
	}

	entity.life.maxLife = newMaxLife;
	entity.life.life = glm::clamp(newMaxLife * lifePercent, 0.f, newMaxLife);
	variantConfigured = true;
}

void ZombieServer::forceTarget(std::uint64_t playerId)
{
	ensureBehaviour();
	basicEnemyBehaviour->playerLockedOn = playerId;
	basicEnemyBehaviour->currentState = BasicEnemyBehaviour::stateTargetedPlayer;
	basicEnemyBehaviour->worriedTimer = 60.f;
}

bool ZombieServer::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
	ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
	std::unordered_set<std::uint64_t> &othersDeleted,
	PathFindingFieldView &pathFinding,
	std::unordered_map<std::uint64_t, glm::dvec3> &playersPosition,
	std::unordered_map < std::uint64_t, Client *> &allClients)
{
	configureVariant(yourEID);

	BasicEnemyBehaviourOtherSettings settings;
	settings.runSpeed = 2.f * moveSpeedMultiplier;
	settings.searchDistance = 16.f;
	settings.hearBonus = 0.02f;
	settings.sightBonus = 0.05f;

	if (variant == Runner)
	{
		settings.searchDistance = 18.f;
		settings.hearBonus = 0.06f;
	}
	else if (variant == Brute)
	{
		settings.searchDistance = 14.f;
		settings.hearBonus = 0.03f;
		settings.sightBonus = 0.02f;
	}

	basicEnemyBehaviour->update(this, deltaTime, chunkGetter, serverChunkStorer, rng, yourEID,
		othersDeleted, pathFinding, playersPosition, getPosition(), allClients, settings);

	doCollisionWithOthers(getPosition(), entity.getMaxColliderSize(), entity.forces,
		serverChunkStorer, yourEID);
	entity.update(deltaTime, chunkGetter);
	return true;
}

WeaponStats ZombieServer::getWeaponStats()
{
	WeaponStats weaponStats;
	weaponStats.armourPenetration = 1.f;
	weaponStats.accuracy = 7.f;
	weaponStats.range = 1.55f;
	weaponStats.critChance = 0.06f;

	switch (variant)
	{
	case Runner:
		weaponStats.damage = 5.f;
		weaponStats.critDamage = 8.f;
		weaponStats.surprizeDamage = 8.f;
		weaponStats.speed = ZOMBIE_RUNNER_ATTACK_COOLDOWN;
		weaponStats.range = 1.45f;
		weaponStats.knockBack = 1.5f;
		weaponStats.accuracy = 8.f;
		break;
	case Brute:
		weaponStats.damage = 11.f;
		weaponStats.critDamage = 16.f;
		weaponStats.surprizeDamage = 14.f;
		weaponStats.speed = ZOMBIE_BRUTE_ATTACK_COOLDOWN;
		weaponStats.range = 1.8f;
		weaponStats.knockBack = 5.f;
		weaponStats.accuracy = 6.f;
		break;
	case Walker:
	default:
		weaponStats.damage = 6.f;
		weaponStats.critDamage = 9.f;
		weaponStats.surprizeDamage = 9.f;
		weaponStats.speed = ZOMBIE_WALKER_ATTACK_COOLDOWN;
		weaponStats.knockBack = 2.f;
		break;
	}

	return weaponStats;
}

LootTable zombieWalkerLootTable
{
	{LootEntry{1, 1, 2, ItemTypes::cloth}, LootEntry{3, 1, 2, ItemTypes::bone}},
	glm::ivec2{5, 22},
	0.20f,
	{LootEntry{1, 1, 1, ItemTypes::bone}, LootEntry{6, 1, 1, ItemTypes::bandage}},
};

LootTable zombieRunnerLootTable
{
	{LootEntry{1, 1, 2, ItemTypes::cloth}, LootEntry{4, 1, 1, ItemTypes::bandage}},
	glm::ivec2{10, 35},
	0.30f,
	{LootEntry{1, 1, 2, ItemTypes::bone}, LootEntry{5, 1, 1, ItemTypes::apple}},
};

LootTable zombieBruteLootTable
{
	{LootEntry{1, 2, 5, ItemTypes::bone}, LootEntry{2, 2, 4, ItemTypes::cloth}},
	glm::ivec2{35, 95},
	0.65f,
	{LootEntry{1, 1, 2, ItemTypes::copperIngot}, LootEntry{5, 1, 1, ItemTypes::ironIngot}, LootEntry{7, 1, 1, ItemTypes::healingPotion}},
};

LootTable &ZombieServer::getLootTable()
{
	switch (variant)
	{
	case Runner: return zombieRunnerLootTable;
	case Brute: return zombieBruteLootTable;
	case Walker:
	default: return zombieWalkerLootTable;
	}
}
