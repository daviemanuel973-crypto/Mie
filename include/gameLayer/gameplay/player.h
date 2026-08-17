#pragma once
#include <gameplay/entity.h>
#include <multyPlayer/server.h>
#include <gameplay/items.h>
#include <gameplay/fieldGuide.h>
#include <gl2d/gl2d.h>
#include <gameplay/life.h>
#include <gameplay/gameplayRules.h>
#include <gameplay/worldDifficulty.h>
#include <gameplay/effects.h>

#define PLAYER_DEFAULT_LIFE Life(100)

//this is the shared data
struct Player : public PhysicalEntity, public CollidesWithPlacedBlocks,
	public CanPushOthers, public CanBeKilled, public CanBeAttacked,
	public CanHaveEffects, public HasOrientationAndHeadTurnDirection, 
	public HasEyesAndPupils<EYE_ANIMATION_TYPE_PLAYER>
{

	bool operator== (Player & other)
	{
		if(
			lookDirectionAnimation == other.lookDirectionAnimation &&
			bodyOrientation == other.bodyOrientation &&
			fly == other.fly
			)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool operator!= (Player & other)
	{
		return !(*this == other);
	}

	void flyFPS(glm::vec3 direction, glm::vec3 lookDirection);
	void moveFPS(glm::vec3 direction, glm::vec3 lookDirection, float deltaTime);

	int chunkDistance = 10;

	glm::vec3 getColliderSize();
	void update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter);
	float getWaterSubmersion(decltype(chunkGetterSignature) *chunkGetter) const;
	bool isInWater(decltype(chunkGetterSignature) *chunkGetter) const;
	void swimUp(decltype(chunkGetterSignature) *chunkGetter);
	bool isOnClimbable(decltype(chunkGetterSignature) *chunkGetter) const;
	void climb(float direction, bool fast, decltype(chunkGetterSignature) *chunkGetter);

	static glm::vec3 getMaxColliderSize();

	bool fly = 0;
};

struct OtherPlayerSettings
{
	constexpr static int SURVIVAL = 0;
	constexpr static int CREATIVE = 1;

	unsigned char gameMode = 0;
	char commandPermisionLevel = 2;
};

struct LocalPlayer
{
	PlayerInventory inventory;
	Player entity = {};
	std::uint64_t entityId = 0;
	OtherPlayerSettings otherPlayerSettings = {};

	glm::ivec3 currentBlockInteractWith = {0,-1,0};
	unsigned char isInteractingWithBlock = 0;

	Life life = PLAYER_DEFAULT_LIFE;
	Life lastLife = PLAYER_DEFAULT_LIFE;
	SurvivalStats survivalStats = {};
	float justHealedTimer = 0;
	float justRecievedDamageTimer = 0;
	GuideProgress guideProgress = {};

	Effects effects;
};

struct PlayerClient: public ClientEntity<Player, PlayerClient>
{
	void update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter);
	void setEntityMatrix(glm::mat4 *skinningMatrix);
	int getTextureIndex();
};

struct PlayerServer: public ServerEntity<Player>
{
	OtherPlayerSettings otherPlayerSettings = {};
	PlayerInventory inventory;

	// v0.7 survival-guide state. This is persisted in player save version 3.
	bool starterFieldGuideGranted = false;
	GuideProgress guideProgress = {};
	bool guideProgressDirty = true;

	unsigned char interactingWithBlock = 0;
	unsigned char revisionNumberInteraction = 0;
	glm::ivec3 currentBlockInteractWithPosition = {0, -1, 0};

	Life lifeLastFrame = PLAYER_DEFAULT_LIFE;
	Life newLife = PLAYER_DEFAULT_LIFE;
	SurvivalStats survivalStats = {};
	float hungerExhaustion = 0;
	float starvationTimer = 0;
	glm::dvec3 lastHungerPosition = {};
	bool hungerPositionInitialized = false;
	bool forceUpdateLife = 0;

	short updateEffectsTicksTimer = 20;

	void applyDamageOrLife(short difference)
	{
		if (otherPlayerSettings.gameMode == OtherPlayerSettings::CREATIVE) { return; }
		if (difference < 0)
		{
			difference = scaleIncomingDamageForDifficulty(difference,
				getServerWorldDifficultySettings());
		}

		int life = newLife.life;
		life += difference;
		if (life > newLife.maxLife) { life = newLife.maxLife; }

		newLife.life = life;
		if (difference < 0)
		{
			healingDelayCounterSecconds = 0;
		}
	}

	struct EffectsTimer
	{
		float regen = 0;
		float poison = 0;
	}effectsTimers;

	bool killed = 0;
	float notIncreasedLifeSinceTimeSecconds = 0;
	float healingDelayCounterSecconds = BASE_HEALTH_DELAY_TIME;

	void kill();

	Armour getArmour() 
	{
		Armour rez{};
		rez.armour += inventory.headArmour.getItemStats().armour;
		rez.armour += inventory.chestArmour.getItemStats().armour;
		rez.armour += inventory.bootsArmour.getItemStats().armour;
		rez.armour += effects.getArmour();
		rez.normalize();
		return rez;
	};

	glm::ivec2 lastChunkPositionWhenAnUpdateWasSent = {};

	float calculateHealingDelayTime();
	float calculateHealingRegenTime();

	bool isUnaware() { return false; }
	void signalHit(glm::vec3 d) {};
};

EntityStats getPlayerStats(PlayerInventory &inventory);
