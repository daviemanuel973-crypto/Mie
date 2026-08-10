#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtx/transform.hpp>
#include <gameplay/zombie.h>
#include <gameplay/basicEnemyBehaviour.h>
#include <multyPlayer/serverChunkStorer.h>
#include <iostream>
#include <glm/gtx/quaternion.hpp>
#include <rendering/model.h>

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
	(void)f;
	(void)eId;
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
	const float oldMaxLife = entity.life.maxLife > 0.f ? entity.life.maxLife : 200.f;
	const float lifePercent = glm::clamp(entity.life.life / oldMaxLife, 0.f, 1.f);
	float newMaxLife = 180.f;

	if (roll < 70)
	{
		variant = Walker;
		moveSpeedMultiplier = 1.f;
		newMaxLife = 180.f;
	}
	else if (roll < 90)
	{
		variant = Runner;
		moveSpeedMultiplier = 1.55f;
		newMaxLife = 120.f;
	}
	else
	{
		variant = Brute;
		moveSpeedMultiplier = 0.72f;
		newMaxLife = 360.f;
	}

	entity.life.maxLife = newMaxLife;
	entity.life.life = glm::clamp(newMaxLife * lifePercent, 0.f, newMaxLife);
	variantConfigured = true;
}

bool ZombieServer::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
	ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
	std::unordered_set<std::uint64_t> &othersDeleted,
	std::unordered_map<std::uint64_t, std::unordered_map<glm::ivec3, PathFindingNode>> &pathFinding,
	std::unordered_map<std::uint64_t, glm::dvec3> &playersPosition,
	std::unordered_map < std::uint64_t, Client *> &allClients)
{
	configureVariant(yourEID);

	BasicEnemyBehaviourOtherSettings settings;
	settings.runSpeed = 2.f * moveSpeedMultiplier;
	settings.searchDistance = 40.f;
	settings.hearBonus = 0.08f;
	settings.sightBonus = 0.08f;

	if (variant == Runner)
	{
		settings.searchDistance = 46.f;
		settings.hearBonus = 0.16f;
	}
	else if (variant == Brute)
	{
		settings.searchDistance = 34.f;
		settings.hearBonus = 0.12f;
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
		weaponStats.damage = 12.f;
		weaponStats.critDamage = 20.f;
		weaponStats.surprizeDamage = 18.f;
		weaponStats.speed = 4.f;
		weaponStats.range = 1.45f;
		weaponStats.knockBack = 2.f;
		weaponStats.accuracy = 8.f;
		break;
	case Brute:
		weaponStats.damage = 32.f;
		weaponStats.critDamage = 48.f;
		weaponStats.surprizeDamage = 42.f;
		weaponStats.speed = 0.f;
		weaponStats.range = 1.8f;
		weaponStats.knockBack = 8.f;
		weaponStats.accuracy = 6.f;
		break;
	case Walker:
	default:
		weaponStats.damage = 16.f;
		weaponStats.critDamage = 25.f;
		weaponStats.surprizeDamage = 24.f;
		weaponStats.speed = 1.5f;
		weaponStats.knockBack = 3.f;
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
