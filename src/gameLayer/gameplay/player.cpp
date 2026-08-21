#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "gameplay/player.h"
#include <iostream>
#include <rendering/model.h>
#include <rendering/camera.h>
#include <metrics.h>
#include <algorithm>
#include <cmath>

void Player::flyFPS(glm::vec3 direction, glm::vec3 lookDirection)
{
	lookDirection.y = 0;
	float l = glm::length(lookDirection);
	if (!std::isfinite(l) || l <= 0.000001f) { return; }
	lookDirection /= l;

	float forward = -direction.z;
	float leftRight = direction.x;
	float upDown = direction.y;

	glm::vec3 move = {};
	move += glm::vec3(0, 1, 0) * upDown;
	move += glm::normalize(glm::cross(lookDirection, glm::vec3(0, 1, 0))) * leftRight;
	move += lookDirection * forward;
	this->position += move;
}

void Player::moveFPS(glm::vec3 direction, glm::vec3 lookDirection, float deltaTime)
{
	if (!std::isfinite(deltaTime) || deltaTime <= 0.f) { return; }
	lookDirection.y = 0;
	const float lookLength = glm::length(lookDirection);
	if (!std::isfinite(lookLength) || lookLength <= 0.000001f) { return; }
	lookDirection /= lookLength;

	float forward = -direction.z;
	float leftRight = direction.x;

	glm::vec3 move = {};
	move += glm::normalize(glm::cross(lookDirection, glm::vec3(0, 1, 0))) * leftRight;
	move += lookDirection * forward;

	this->moveDynamic({move.x, move.z}, deltaTime);
}

glm::vec3 Player::getColliderSize()
{
	return getMaxColliderSize();
}

void Player::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	const float submersion = fly ? 0.f : getWaterSubmersion(chunkGetter);
	const bool climbing = !fly && submersion <= 0.f && isOnClimbable(chunkGetter);
	if (submersion > 0.f)
	{
		const float horizontalDrag = std::exp(-2.6f * submersion * deltaTime);
		const float verticalDrag = std::exp(-1.8f * submersion * deltaTime);
		forces.velocity.x *= horizontalDrag;
		forces.velocity.z *= horizontalDrag;
		forces.velocity.y *= verticalDrag;
		forces.acceleration.y += 22.f * submersion;

		PhysicalSettings waterPhysics;
		waterPhysics.gravityModifier = glm::mix(1.f, 0.30f, submersion);
		waterPhysics.sideFriction = 0.15f;
		updateForces(deltaTime, true, waterPhysics);
		forces.velocity.y = glm::clamp(forces.velocity.y, -4.f, 5.f);
	}
	else if (climbing)
	{
		const float horizontalDrag = std::exp(-4.f * deltaTime);
		const float verticalDrag = std::exp(-6.f * deltaTime);
		forces.velocity.x *= horizontalDrag;
		forces.velocity.z *= horizontalDrag;
		forces.velocity.y *= verticalDrag;

		PhysicalSettings ladderPhysics;
		ladderPhysics.gravityModifier = 0.08f;
		ladderPhysics.sideFriction = 0.18f;
		updateForces(deltaTime, true, ladderPhysics);
		forces.velocity.y = glm::clamp(forces.velocity.y, -5.5f, 7.5f);
	}
	else
	{
		updateForces(deltaTime, !fly);
	}

	resolveConstrainsAndUpdatePositions(chunkGetter, deltaTime, getColliderSize());
}

float Player::getWaterSubmersion(decltype(chunkGetterSignature) *chunkGetter) const
{
	if (!chunkGetter) { return 0.f; }

	const float sampleHeights[] = {0.15f, 0.85f, 1.45f};
	int waterSamples = 0;
	for (float height : sampleHeights)
	{
		const auto worldBlock = from3DPointToBlock(position + glm::dvec3(0, height, 0));
		if (worldBlock.y < 0 || worldBlock.y >= CHUNK_HEIGHT) { continue; }

		const auto chunkPosition = fromBlockPosToChunkPos(worldBlock);
		auto *chunk = chunkGetter(chunkPosition);
		if (!chunk) { continue; }

		const auto localBlock = fromBlockPosToBlockPosInChunk(worldBlock);
		auto *block = chunk->safeGet(localBlock);
		if (block && block->getType() == BlockTypes::water) { ++waterSamples; }
	}

	return static_cast<float>(waterSamples) /
		static_cast<float>(sizeof(sampleHeights) / sizeof(sampleHeights[0]));
}

bool Player::isInWater(decltype(chunkGetterSignature) *chunkGetter) const
{
	return getWaterSubmersion(chunkGetter) > 0.f;
}

void Player::swimUp(decltype(chunkGetterSignature) *chunkGetter)
{
	const float submersion = getWaterSubmersion(chunkGetter);
	if (submersion <= 0.f) { return; }

	forces.acceleration.y += 38.f * std::max(0.45f, submersion);
	forces.velocity.y = std::min(forces.velocity.y, 7.f);
}

bool Player::isOnClimbable(decltype(chunkGetterSignature) *chunkGetter) const
{
	if (!chunkGetter) { return false; }
	const float sampleHeights[] = {0.2f, 0.9f, 1.55f};
	for (float height : sampleHeights)
	{
		const auto worldBlock = from3DPointToBlock(position + glm::dvec3(0, height, 0));
		if (worldBlock.y < 0 || worldBlock.y >= CHUNK_HEIGHT) { continue; }
		auto *chunk = chunkGetter(fromBlockPosToChunkPos(worldBlock));
		if (!chunk) { continue; }
		auto *block = chunk->safeGet(fromBlockPosToBlockPosInChunk(worldBlock));
		if (block && (block->getType() == BlockTypes::ladder ||
			block->getType() == BlockTypes::vines))
		{
			return true;
		}
	}
	return false;
}

void Player::climb(float direction, bool fast, decltype(chunkGetterSignature) *chunkGetter)
{
	if (direction == 0.f || !isOnClimbable(chunkGetter)) { return; }
	const float climbSpeed = fast ? 7.f : 5.5f;
	forces.velocity.y = glm::clamp(direction, -1.f, 1.f) * climbSpeed;
	forces.acceleration.y = 0.f;
}

glm::vec3 Player::getMaxColliderSize()
{
	return glm::vec3(0.8, 1.8, 0.8);
}

void PlayerClient::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{
	entityBuffered.update(deltaTime, chunkGetter);
}

void PlayerClient::setEntityMatrix(glm::mat4 *skinningMatrix)
{
	(void)skinningMatrix;
}

int PlayerClient::getTextureIndex()
{
	return ModelsManager::TexturesLoaded::SteveTexture;
}

void PlayerServer::kill()
{
	killed = true;
	effects = {};
	newLife.life = 0;
	lifeLastFrame.life = 0;
	notIncreasedLifeSinceTimeSecconds = 0;
	interactingWithBlock = 0;
	revisionNumberInteraction = 0;
	effectsTimers = {};
}

float PlayerServer::calculateHealingDelayTime()
{
	float rez = BASE_HEALTH_DELAY_TIME;

	for (int i = PlayerInventory::EQUIPEMENT_START_INDEX; i < PlayerInventory::EQUIPEMENT_START_INDEX +
		PlayerInventory::MAX_EQUIPEMENT_SLOTS; i++)
	{
		auto item = inventory.getItemFromIndex(i, 0);
		if (item && item->type == ItemTypes::bandage)
		{
			rez -= 5;
		}
	}

	return std::max(rez, 0.f);
}

float PlayerServer::calculateHealingRegenTime()
{
	return BASE_HEALTH_REGEN_TIME;
}

short PlayerServer::getKnockBackResistance()
{
	return getPlayerStats(inventory).knockBackResistance;
}

EntityStats getPlayerStats(PlayerInventory &inventory)
{
	EntityStats rez;

	// Base player modifier. Equipment is aggregated here so all systems consume
	// one authoritative view of movement/combat/utility stats.
	rez.runningSpeed = 8;

	for (int i = PlayerInventory::EQUIPEMENT_START_INDEX;
		i < PlayerInventory::EQUIPEMENT_START_INDEX + PlayerInventory::MAX_EQUIPEMENT_SLOTS; ++i)
	{
		Item *item = inventory.getItemFromIndex(i, 0);
		if (!item || !item->type) { continue; }
		EntityStats itemStats = item->getItemStats();
		rez.add(itemStats);
	}

	rez.normalize();
	return rez;
}
