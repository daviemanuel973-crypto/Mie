#include <gameplay/droppedItem.h>
#include <multyPlayer/serverChunkStorer.h>
#include <multyPlayer/enetServerFunction.h>
namespace
{
	struct DroppedItemDiskData
	{
		PhysicalEntity entity;
		float restantTime = 0.f;
		float stayTimer = 0.f;
		float dontPickTimer = 0.f;
		std::uint16_t type = 0;
		std::uint16_t counter = 0;
		std::uint32_t metadataSize = 0;
	};

	constexpr std::uint32_t maxPersistedItemMetadata = 64u * 1024u;
}


void DroppedItem::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter)
{


	//THIS IS SHARED CODE!!!!
	PhysicalSettings ps;
	ps.gravityModifier = 0.5f;
	ps.sideFriction = 1.f;
	updateForces(deltaTime, true, ps);
	resolveConstrainsAndUpdatePositions(chunkGetter, deltaTime, getMaxColliderSize(), ps);

}

glm::vec3 DroppedItem::getMaxColliderSize()
{
	return {0.4,0.4,0.4};
}

glm::vec3 DroppedItem::getColliderSize()
{
	return getMaxColliderSize();
}

glm::vec3 DroppedItemServer::getMaxColliderSize()
{
	return {0.4,0.4,0.4};
}

glm::vec3 DroppedItemServer::getColliderSize()
{
	return getMaxColliderSize();
}

DroppedItem DroppedItemServer::getDataToSend()
{
	DroppedItem ret;
	ret.type = item.type;
	ret.count = item.counter;

	ret.forces = entity.forces;
	ret.lastPosition = entity.lastPosition;
	ret.position = entity.position;

	return ret;
}

bool DroppedItemServer::update(float deltaTime, decltype(chunkGetterSignature) *chunkGetter,
	ServerChunkStorer &serverChunkStorer, std::minstd_rand &rng, std::uint64_t yourEID,
	std::unordered_set<std::uint64_t> &othersDeleted,
	PathFindingFieldView &pathFinding,
	std::unordered_map<std::uint64_t, glm::dvec3> &playersPositionSirvival,
	std::unordered_map < std::uint64_t, Client *> &allClients
)
{
	//todo use persistent timer
	stayTimer -= deltaTime;
	
	if (dontPickTimer>0)
	dontPickTimer -= deltaTime;

	
	
	if (stayTimer < 0)
	{
		return 0;
	}

	glm::ivec2 chunkPosition = determineChunkThatIsEntityIn(getPosition());

	for (auto offset : *getChunkNeighboursOffsets())
	{
		glm::ivec2 pos = chunkPosition + offset;
		auto c = serverChunkStorer.getChunkOrGetNull(pos.x, pos.y);
		if (c)
		{
			for (auto &p : c->entityData.droppedItems)
			{
				if (p.first != yourEID)
				{
					if (glm::distance(getPosition(), p.second.getPosition()) < 1.f && item.type == p.second.item.type)
					{

						const int stackSize = p.second.item.getStackSize();
						if (item.counter + p.second.item.counter < stackSize)
						{
							//merge the 2 items
							item.counter += p.second.item.counter;
							othersDeleted.insert(p.first);
							c->entityData.droppedItems.erase(p.first);

							stayTimer = std::max(stayTimer, p.second.stayTimer);

							break;
						}
						else if(item.counter < stackSize && p.second.item.counter < stackSize)
						{
							//steal sum
							p.second.item.counter = (item.counter + p.second.item.counter) - stackSize;
							item.counter = stackSize;
						}

						
					};

				}
			}
		}
	}


	if(dontPickTimer<=0)
	for (auto &p : allClients)
	{
		

		if (glm::distance(getPosition(), p.second->playerData.getPosition()) < 1.f)
		{

			auto client = p.second;

			//pickupped this item
			int pickupped = client->playerData.inventory.tryPickupItem(item);
			if (pickupped)
			{
				sendPlayerInventoryAndIncrementRevision(*client);

				item.counter -= pickupped;
				if(item.counter <= 0)
				{
					return 0;
				}
			}


		}
	}

	//
	//auto client = getClient(0);
	//sendPlayerInventory(client);

	doCollisionWithOthers(getPosition(), getMaxColliderSize(), entity.forces,
		serverChunkStorer, yourEID);
	
	//THIS IS SHARED CODE!!!!
	PhysicalSettings ps;
	ps.gravityModifier = 0.5f;
	ps.sideFriction = 1.f;
	entity.updateForces(deltaTime, true, ps);
	entity.resolveConstrainsAndUpdatePositions(chunkGetter, deltaTime, getMaxColliderSize(), ps);

	return true;
}

void DroppedItemServer::appendDataToDisk(std::ofstream &f, std::uint64_t eId)
{
	if (!f || item.type == 0 || item.counter == 0 ||
		item.metaData.size() > maxPersistedItemMetadata) { return; }
	DroppedItemDiskData data;
	data.entity = entity;
	data.restantTime = restantTime;
	data.stayTimer = stayTimer;
	data.dontPickTimer = dontPickTimer;
	data.type = item.type;
	data.counter = item.counter;
	data.metadataSize = static_cast<std::uint32_t>(item.metaData.size());
	basicEntitySave(f, Markers::droppedItem, eId, &data, sizeof(data));
	if (!item.metaData.empty())
	{
		appendData(f, item.metaData.data(), item.metaData.size());
	}
}

bool DroppedItemServer::loadFromDisk(std::ifstream &f)
{
	DroppedItemDiskData data;
	if (!readData(f, &data, sizeof(data)) || data.type == 0 || data.counter == 0 ||
		data.metadataSize > maxPersistedItemMetadata ||
		!(data.type < BlocksCount || isItem(data.type)))
	{
		return false;
	}
	std::vector<unsigned char> metadata(data.metadataSize);
	if (!metadata.empty() && !readData(f, metadata.data(), metadata.size())) { return false; }
	entity = data.entity;
	restantTime = data.restantTime;
	stayTimer = data.stayTimer;
	dontPickTimer = data.dontPickTimer;
	item = itemCreator(data.type, data.counter);
	item.metaData = std::move(metadata);
	item.sanitize();
	return item.type != 0;
}

void DroppedItemClient::update(float deltaTime, 
	decltype(chunkGetterSignature) *chunkGetter)
{
	entityBuffered.update(deltaTime, chunkGetter);
}

void DroppedItemClient::setEntityMatrix(glm::mat4 *skinningMatrix)
{
}
