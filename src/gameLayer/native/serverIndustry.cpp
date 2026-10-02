#include <native/serverIndustry.h>
#include <multyPlayer/serverChunkStorer.h>
#include <multyPlayer/enetServerFunction.h>
#include <algorithm>
#include <map>
#include <set>
#include <runtimeSmoke.h>
#include <atomic>
#include <iostream>

namespace
{
	IndustryNetwork network;
	struct ChunkIndex
	{
		std::uint64_t generation=0,topology=0,activity=0;
		std::vector<IndustryPosition> nodes;
	};
	std::map<std::pair<int,int>,ChunkIndex> indexes;
	std::atomic<bool> smokeVerified{false};
	bool fixtureInitialized=false;
	IndustryPosition fixtureAnchor;

	void prepareFixture(ServerChunkStorer &chunks)
	{
		if (!runtimeSmokeRequested() || fixtureInitialized || getAllClientsReff().empty()) { return; }
		const auto playerPos=determineChunkThatIsEntityIn(getAllClientsReff().begin()->second.playerData.entity.position);
		auto *c=chunks.getChunkOrGetNull(playerPos.x,playerPos.y);
		if (!c || !c->otherData.withinSimulationDistance) { return; }
		fixtureAnchor={playerPos.x*CHUNK_SIZE,220,playerPos.y*CHUNK_SIZE};
		fixtureInitialized=true;
		if (runtimeSmokeReusesExistingWorld())
		{
			unsigned copper=0;
			bool valid=true;
			for (auto pos : {glm::ivec3{4,220,2},glm::ivec3{6,220,3}})
			{
				auto *machine=c->blockData.getFurnaceBlock(pos.x,pos.y,pos.z);
				if (!machine) { valid=false; continue; }
				for (const auto &item:machine->items)
					if (item.type==copperOre || item.type==copperPowder || item.type==copperIngot) { copper+=item.counter; }
			}
			auto *destination=c->blockData.getChestBlock(7,220,4);
			if (!destination) { valid=false; }
			else for (const auto &item:destination->items) if (item.type==copperIngot) { copper+=item.counter; }
			auto *filter=c->blockData.getFurnaceBlock(6,220,4);
			valid=valid && filter && filter->items[0].type==copperIngot && filter->items[0].counter==1;
			if (valid && copper==3)
			{ smokeVerified.store(true); std::cout<<"[industry-smoke] PASS: persisted ore/powder/ingots conserve three units after restart\n"; }
			return;
		}
		const struct { int x,z; std::uint16_t type; } layout[]={{2,2,fuelGenerator},{3,2,powerCable},
			{4,2,oreCrusher},{5,2,powerCable},{6,2,powerCable},{4,3,itemExtractor},
			{5,3,itemPipe},{6,3,electricFurnace},{6,4,itemExtractor},{7,4,woddenChest}};
		for (auto cell:layout)
		{
			auto &b=c->chunk.blocks[cell.x][cell.z][220]; b={}; b.setType(cell.type);
			if (cell.type==woddenChest) { c->blockData.getOrCreateChestBlock(cell.x,220,cell.z); }
			else { c->blockData.getOrCreateFurnaceBlock(cell.x,220,cell.z)->processingType=cell.type; }
			Packet packet; packet.cid=0; packet.header=headerPlaceBlocks;
			Packet_PlaceBlocks placed; placed.blockPos={fixtureAnchor.x+cell.x,220,fixtureAnchor.z+cell.z}; placed.blockInfo=b;
			broadCastNotLocked(packet,&placed,sizeof(placed),nullptr,true,channelChunksAndBlocks);
		}
		c->blockData.getOrCreateFurnaceBlock(2,220,2)->items[3]=Item(charcoal,4);
		c->blockData.getOrCreateFurnaceBlock(4,220,2)->items[0]=Item(copperOre,3);
		c->blockData.getOrCreateFurnaceBlock(6,220,4)->items[0]=Item(copperIngot,1);
		++c->industryTopologyRevision; c->otherData.dirty=true; c->otherData.dirtyBlockData=true;
	}
}

void resetServerIndustry() { network.clear(); indexes.clear(); fixtureInitialized=false; smokeVerified.store(false); }
IndustryMetrics getServerIndustryMetrics() { return network.metrics(); }
bool industrySmokeVerified() { return smokeVerified.load(); }

void updateServerIndustry(ServerChunkStorer &chunks,float seconds)
{
	prepareFixture(chunks);
	std::set<std::pair<int,int>> present;
	for (const auto &entry:chunks.savedChunks)
	{
		auto *c=entry.second;
		if (!c || c->otherData.shouldUnload || !c->otherData.withinSimulationDistance) { continue; }
		const auto key=std::make_pair(entry.first.x,entry.first.y); present.insert(key);
		auto &index=indexes[key];
		if (index.generation!=c->industryGeneration || index.topology!=c->industryTopologyRevision)
		{
			for (auto p:index.nodes) { network.removeNode(p); }
			index.nodes.clear();
			auto add=[&](std::uint16_t hash,std::uint16_t type)
			{
				const auto local=fromHashValueToBlockPosinChunk(hash);
				IndustryPosition p{entry.first.x*CHUNK_SIZE+local.x,local.y,entry.first.y*CHUNK_SIZE+local.z};
				index.nodes.push_back(p); network.setNode(p,type);
			};
			for (const auto &machine:c->blockData.furnaceBlocks) { add(machine.first,machine.second.blockType()); }
			for (const auto &chest:c->blockData.chestBlocks)
			{
				const auto p=fromHashValueToBlockPosinChunk(chest.first);
				add(chest.first,c->chunk.blocks[p.x][p.z][p.y].getType());
			}
			index.generation=c->industryGeneration; index.topology=c->industryTopologyRevision;
		}
		if (index.activity!=c->industryActivityRevision)
		{ for (auto p:index.nodes) { network.wake(p); } index.activity=c->industryActivityRevision; }
	}
	for (auto it=indexes.begin();it!=indexes.end();)
	{
		if (!present.count(it->first))
		{ for (auto p:it->second.nodes) { network.removeNode(p); } it=indexes.erase(it); }
		else { ++it; }
	}
	std::set<IndustryPosition> changed;
	IndustryAccess access;
	access.machine=[&](IndustryPosition p)->FurnaceBlock *
	{
		SavedChunk *c=nullptr; auto *b=chunks.getBlockSafeAndChunk({p.x,p.y,p.z},c);
		if (!b || !c || !isProcessingBlock(b->getType()) || !c->otherData.withinSimulationDistance) { return nullptr; }
		return c->blockData.getFurnaceBlock(modBlockToChunk(p.x),p.y,modBlockToChunk(p.z));
	};
	access.chest=[&](IndustryPosition p)->ChestBlock *
	{
		SavedChunk *c=nullptr; auto *b=chunks.getBlockSafeAndChunk({p.x,p.y,p.z},c);
		if (!b || !c || !isChest(b->getType()) || !c->otherData.withinSimulationDistance) { return nullptr; }
		return c->blockData.getChestBlock(modBlockToChunk(p.x),p.y,modBlockToChunk(p.z));
	};
	access.changed=[&](IndustryPosition p)
	{
		changed.insert(p); network.wake(p);
		auto *c=chunks.getChunkOrGetNull(divideChunk(p.x),divideChunk(p.z));
		if (c) { c->otherData.dirtyBlockData=true; }
	};
	network.update(seconds,access);
	if (fixtureInitialized && !smokeVerified.load() && !runtimeSmokeReusesExistingWorld())
	{
		auto *chest=access.chest({fixtureAnchor.x+7,220,fixtureAnchor.z+4});
		if (chest && chest->items[0].type==copperIngot && chest->items[0].counter==3)
		{ smokeVerified.store(true); std::cout<<"[industry-smoke] PASS: generator/cables/crusher/tubes/electric furnace produce three ingots\n"; }
	}
	for (auto p:changed)
	{
		std::vector<unsigned char> payload;
		if (auto *m=access.machine(p)) { appendFurnaceBlock(payload,{p.x,p.y,p.z},*m); }
		else if (auto *c=access.chest(p)) { appendChestBlock(payload,{p.x,p.y,p.z},*c); }
		if (payload.empty()) { continue; }
		Packet packet; packet.header=headerRecieveUpdatesBlockDataForChunk; packet.cid=0;
		const glm::ivec2 chunkPos{divideChunk(p.x),divideChunk(p.z)};
		for (auto &client:getAllClientsReff())
		{
			const auto playerPos=determineChunkThatIsEntityIn(client.second.playerData.entity.position);
			if (!client.second.loadedChunks.count(chunkPos) || !checkIfPlayerShouldGetChunk(playerPos,chunkPos,client.second.playerData.entity.chunkDistance)) { continue; }
			sendPacketAndCompress(client.second.peer,packet,reinterpret_cast<char *>(payload.data()),payload.size(),true,channelChunksAndBlocks);
		}
	}
}
