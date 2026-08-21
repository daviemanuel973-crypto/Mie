#pragma once
#include <gameplay/entity.h>
#include <random>
#include <gameplay/ai.h>
#include <unordered_map>
#include <unordered_set>



struct Pig: public PhysicalEntity, public HasOrientationAndHeadTurnDirection,
	public MovementSpeedForLegsAnimations, public CanPushOthers, public CollidesWithPlacedBlocks,
	public CanBeKilled, public CanBeAttacked
{
	void update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter);

	glm::vec3 getColliderSize();

	static glm::vec3 getMaxColliderSize();

	//todo frustum culling size

	Life life{100};
	
	Armour getArmour() { return {0}; };
};


struct PigClient: public ClientEntity<Pig, PigClient>
{
	void update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter);
	void setEntityMatrix(glm::mat4 *skinningMatrix);

	int getTextureIndex();
};



struct PigServer: public ServerEntity<Pig>,
	public AnimalBehaviour < PigServer, PigDefaultSettings >
{
	bool update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
		ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
		std::unordered_set<std::uint64_t> &othersDeleted,
		PathFindingFieldView &pathFinding,
		std::unordered_map<std::uint64_t, glm::dvec3> &playersPosition,
		std::unordered_map < std::uint64_t, Client *> &allClients
		);

	void appendDataToDisk(std::ofstream &f, std::uint64_t eId);
	bool loadFromDisk(std::ifstream &f);

	//todo change to init
	void configureSpawnSettings(std::minstd_rand &rng);

	//todo
	bool isUnaware() { return  false; }
	void signalHit(glm::vec3 direction) {};

	LootTable &getLootTable() { return getEmptyLootTable(); }
};









