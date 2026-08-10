#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gameplay/zombie.h>
#include <multyPlayer/serverChunkStorer.h>
#include <rendering/model.h>

static const auto frontHands = glm::rotate(glm::radians(90.f), glm::vec3{1.f, 0.f, 0.f});

void animatePlayerHandsZombie(glm::mat4 *poseVector, float &currentAngle, float deltaTime)
{
	(void)deltaTime;
	poseVector[2] = poseVector[2] * frontHands
		* glm::rotate(cosf(currentAngle) * 0.02f, glm::vec3{1.f, 0.f, 0.f})
		* glm::rotate(cosf(currentAngle + 5.f) * 0.05f, glm::vec3{0.f, 0.f, 1.f});
	poseVector[3] = poseVector[3] * frontHands
		* glm::rotate(cosf(currentAngle + 10.f) * 0.02f, glm::vec3{1.f, 0.f, 0.f})
		* glm::rotate(cosf(currentAngle + 15.f) * 0.05f, glm::vec3{0.f, 0.f, 1.f});
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
	return glm::vec3(0.8f, 1.8f, 0.8f);
}

void ZombieClient::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	currentHandsAngle += deltaTime * 2.f;
	if (currentHandsAngle > glm::radians(360.f))
	{
		currentHandsAngle -= glm::radians(360.f);
	}

	entityBuffered.update(deltaTime, chunkGetter);
}

void ZombieClient::setEntityMatrix(glm::mat4 *skinningMatrix)
{
	animatePlayerHandsZombie(skinningMatrix, currentHandsAngle, 0.f);
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

void ZombieServer::initializeVariant(std::minstd_rand &rng)
{
	if (variantInitialized)
	{
		return;
	}

	variantInitialized = true;
	const unsigned int roll = rng() % 100u;

	if (roll < 72u)
	{
		variant = Walker;
		moveSpeedMultiplier = 1.f;
		entity.life = Life{200};
	}
	else if (roll < 93u)
	{
		variant = Runner;
		moveSpeedMultiplier = 1.65f;
		entity.life = Life{125};
	}
	else
	{
		variant = Brute;
		moveSpeedMultiplier = 0.72f;
		entity.life = Life{420};
	}
}

bool ZombieServer::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
	ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
	std::unordered_set<std::uint64_t> &othersDeleted,
	std::unordered_map<std::uint64_t, std::unordered_map<glm::ivec3, PathFindingNode>> &pathFinding,
	std::unordered_map<std::uint64_t, glm::dvec3> &playersPosition,
	std::unordered_map < std::uint64_t, Client *> &allClients)
{
	initializeVariant(rng);

	BasicEnemyBehaviourOtherSettings settings;
	settings.searchDistance = 42.f;
	settings.hearBonus = 0.16f;
	settings.sightBonus = 0.08f;
	settings.runSpeed = 2.f * moveSpeedMultiplier;

	if (variant == Runner)
	{
		settings.searchDistance = 48.f;
		settings.hearBonus = 0.24f;
	}
	else if (variant == Brute)
	{
		settings.searchDistance = 36.f;
		settings.hearBonus = 0.22f;
	}

	basicEnemyBehaviour.update(this, deltaTime, chunkGetter, serverChunkStorer, rng, yourEID,
		othersDeleted, pathFinding, playersPosition, getPosition(), allClients, settings);

	doCollisionWithOthers(getPosition(), entity.getMaxColliderSize(), entity.forces,
		serverChunkStorer, yourEID);

	entity.update(deltaTime, chunkGetter);
	return true;
}

WeaponStats ZombieServer::getWeaponStats()
{
	WeaponStats stats;
	stats.armourPenetration = 1;
	stats.accuracy = 7;
	stats.range = 1.45f;

	switch (variant)
	{
		case Runner:
			stats.damage = 13;
			stats.critDamage = 20;
			stats.surprizeDamage = 18;
			stats.critChance = 0.06f;
			stats.speed = 0.72f;
			stats.knockBack = 2.f;
			break;

		case Brute:
			stats.damage = 34;
			stats.critDamage = 48;
			stats.surprizeDamage = 44;
			stats.critChance = 0.12f;
			stats.speed = 1.55f;
			stats.range = 1.7f;
			stats.knockBack = 6.f;
			stats.accuracy = 8;
			break;

		case Walker:
		default:
			stats.damage = 18;
			stats.critDamage = 26;
			stats.surprizeDamage = 24;
			stats.critChance = 0.08f;
			stats.speed = 1.1f;
			stats.knockBack = 3.f;
			break;
	}

	return stats;
}

static LootTable walkerLootTable
{
	{
		LootEntry{1.f, 1, 3, ItemTypes::cloth},
		LootEntry{3.f, 1, 2, ItemTypes::bone},
		LootEntry{9.f, 1, 1, ItemTypes::bandage}
	},
	glm::ivec2{1, 7},
	0.14f,
	{
		LootEntry{1.f, 1, 2, ItemTypes::bone},
		LootEntry{5.f, 1, 1, ItemTypes::apple}
	}
};

static LootTable runnerLootTable
{
	{
		LootEntry{1.f, 1, 2, ItemTypes::cloth},
		LootEntry{2.4f, 1, 2, ItemTypes::bone},
		LootEntry{6.f, 1, 2, ItemTypes::bandage},
		LootEntry{14.f, 1, 1, ItemTypes::speedPotion}
	},
	glm::ivec2{3, 12},
	0.24f,
	{
		LootEntry{1.f, 1, 2, ItemTypes::bandage},
		LootEntry{7.f, 1, 1, ItemTypes::healingPotion}
	}
};

static LootTable bruteLootTable
{
	{
		LootEntry{1.f, 2, 5, ItemTypes::bone},
		LootEntry{2.f, 2, 4, ItemTypes::cloth},
		LootEntry{7.f, 1, 2, ItemTypes::copperIngot},
		LootEntry{16.f, 1, 1, ItemTypes::ironIngot}
	},
	glm::ivec2{12, 36},
	0.55f,
	{
		LootEntry{1.f, 1, 2, ItemTypes::bandage},
		LootEntry{4.f, 1, 1, ItemTypes::healingPotion},
		LootEntry{12.f, 1, 1, ItemTypes::strengthPotion}
	}
};

LootTable &ZombieServer::getLootTable()
{
	switch (variant)
	{
		case Runner: return runnerLootTable;
		case Brute: return bruteLootTable;
		case Walker:
		default: return walkerLootTable;
	}
}
