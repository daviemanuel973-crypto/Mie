#pragma once
#include <gameplay/entity.h>
#include <gameplay/life.h>
#include <memory>
#include <random>
#include <unordered_map>
#include <unordered_set>

struct BasicEnemyBehaviour;

struct Zombie: public PhysicalEntity, public CanPushOthers
	, public HasOrientationAndHeadTurnDirection, public CollidesWithPlacedBlocks,
	public CanBeKilled, public CanBeAttacked, public HasEyesAndPupils<EYE_ANIMATION_TYPE_PLAYER>,
	public Animatable
{

	void update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter);

	glm::vec3 getColliderSize();

	glm::vec3 getMaxColliderSize();

	Life life{200};
	
	Armour getArmour() { return {1}; };
};


struct ZombieClient: public ClientEntity<Zombie, ZombieClient>
{
	float currentHandsAngle = 0;

	void update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter);
	void setEntityMatrix(glm::mat4 *skinningMatrix);

	int getTextureIndex();
};

struct ZombieServer: public ServerEntity<Zombie>
{
	enum Variant : unsigned char
	{
		Walker = 0,
		Runner,
		Brute,
	};

	// Kept behind a pointer so zombie.h does not pull BasicEnemyBehaviour -> Client
	// -> Packet -> zombie.h into a circular include chain.
	std::shared_ptr<BasicEnemyBehaviour> basicEnemyBehaviour;
	Variant variant = Walker;
	bool variantConfigured = false;
	float moveSpeedMultiplier = 1.f;

	void ensureBehaviour();
	void configureVariant(std::uint64_t eId);

	void appendDataToDisk(std::ofstream &f, std::uint64_t eId);

	bool update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
		ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
		std::unordered_set<std::uint64_t> &othersDeleted,
		std::unordered_map<std::uint64_t, std::unordered_map<glm::ivec3, PathFindingNode>> &pathFinding,
		std::unordered_map<std::uint64_t, glm::dvec3> &playersPosition,
		std::unordered_map < std::uint64_t, Client *> &allClients);

	bool isUnaware();
	void signalHit(glm::vec3 direction);

	WeaponStats getWeaponStats();
	LootTable &getLootTable();

};

void animatePlayerHandsZombie(glm::mat4 *poseVector, float &currentAngle, float deltaTime);