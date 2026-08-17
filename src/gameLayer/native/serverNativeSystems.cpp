#include <native/serverNativeSystems.h>
#include <native/contentRegistry.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <blocks.h>
#include <gameplay/allentities.h>
#include <gameplay/fieldGuide.h>
#include <gameplay/fieldGuideProtocol.h>
#include <gameplay/items.h>
#include <multyPlayer/chunkSaver.h>
#include <multyPlayer/enetServerFunction.h>
#include <multyPlayer/server.h>
#include <safeSave.h>

namespace mie::native
{
	static_assert(BlockTypes::BlocksCount == V05_BLOCK_COUNT,
		"v0.5 block IDs changed; add new blocks only after the frozen legacy range");
	static_assert(BlockTypes::reinforcedBarricade == 210 && BlockTypes::woodenSpikeTrap == 211,
		"v0.5 defence block IDs must remain stable");
	static_assert(ItemTypes::stick == V05_FIRST_ITEM_ID &&
		ItemTypes::fieldGuide == V07_FIRST_ITEM_ID &&
		ItemTypes::bronzeSword + 1 == V07_LAST_ITEM_EXCLUSIVE &&
		ItemTypes::bedroll == V07_LAST_ITEM_EXCLUSIVE,
		"persisted v0.5/v0.7 item IDs changed; append new content after the frozen legacy range");
	static_assert(EntitiesTypesCount == V05_ENTITY_TYPE_COUNT,
		"v0.5 entity type namespace changed without a migration");

	namespace
	{
		struct ServerNativeState
		{
			ContentRegistry contentRegistry = createV07ContentRegistry();
			ProcessingRecipeRegistry processingRecipes =
				createV07ProcessingRecipeRegistry(contentRegistry);
			WorldSchemaManifest manifest = makeV06WorldSchemaManifest();
			PrototypeMachineRuntime machines;
			VillagerSocietyRuntime villagers;
			ServerNativeSystemsMetrics metrics;
			std::unordered_map<std::uint64_t, GuideProgress> sentGuideProgress;
			std::uint64_t currentTick = 0;
			float interestRefreshTimer = 0.f;
			bool manifestDirty = false;
			bool manifestExists = false;
		};

		ServerNativeState state;

		std::string manifestPath(const WorldSaver &worldSaver)
		{
			return (std::filesystem::path(worldSaver.savePath) / "schemaManifest").string();
		}

		std::string machinesPath(const WorldSaver &worldSaver)
		{
			return (std::filesystem::path(worldSaver.savePath) / "prototypeMachines").string();
		}

		std::string villagersPath(const WorldSaver &worldSaver)
		{
			return (std::filesystem::path(worldSaver.savePath) / "villagerSociety").string();
		}

		bool inventoryContainsType(const PlayerInventory &inventory, std::uint16_t type)
		{
			for (const Item &item : inventory.items)
			{
				if (item.type == type && item.counter > 0) { return true; }
			}
			return inventory.heldInMouse.type == type && inventory.heldInMouse.counter > 0;
		}

		bool ensureStarterFieldGuide(PlayerServer &player)
		{
			if (player.starterFieldGuideGranted) { return false; }

			const Item guide = itemCreator(ItemTypes::fieldGuide, 1);
			if (guide.type != ItemTypes::fieldGuide || player.inventory.tryPickupItem(guide) != 1)
			{
				return false;
			}

			player.starterFieldGuideGranted = true;
			player.guideProgressDirty = true;
			return true;
		}

		bool refreshGuideObjectives(PlayerServer &player)
		{
			bool changed = false;
			if (inventoryContainsGuideWoodLog(player.inventory))
			{
				changed |= completeGuideObjective(player.guideProgress, GuideObjective::GatherWood);
			}
			if (inventoryContainsType(player.inventory, 144))
			{
				changed |= completeGuideObjective(player.guideProgress, GuideObjective::BuildWorkbench);
			}
			if (inventoryContainsType(player.inventory, 204))
			{
				changed |= completeGuideObjective(player.guideProgress, GuideObjective::BuildFurnace);
			}
			if (inventoryContainsType(player.inventory, ItemTypes::charcoal))
			{
				changed |= completeGuideObjective(player.guideProgress, GuideObjective::MakeCharcoal);
			}
			if (inventoryContainsType(player.inventory, ItemTypes::bronzeIngot))
			{
				changed |= completeGuideObjective(player.guideProgress, GuideObjective::EnterBronzeAge);
			}
			if (changed) { player.guideProgressDirty = true; }
			return changed;
		}

		void synchronizeGuideProgress(std::uint64_t cid, Client &client)
		{
			const GuideProgress progress = client.playerData.guideProgress;
			auto previous = state.sentGuideProgress.find(cid);
			if (previous != state.sentGuideProgress.end() &&
				previous->second.completedMask == progress.completedMask &&
				previous->second.rewardedMask == progress.rewardedMask)
			{
				return;
			}

			if (client.peer)
			{
				GuideProgress payload = progress;
				sendPacket(client.peer, headerUpdateGuideProgress, &payload, sizeof(payload),
					true, channelEffects);
				state.sentGuideProgress[cid] = progress;
			}
		}

		void updatePlayerFieldGuides()
		{
			auto &clients = getAllClientsReff();
			for (auto &entry : clients)
			{
				Client &client = entry.second;
				PlayerServer &player = client.playerData;
				bool inventoryChanged = ensureStarterFieldGuide(player);
				refreshGuideObjectives(player);
				if (deliverPendingGuideRewards(player.guideProgress, player.inventory))
				{
					player.guideProgressDirty = true;
					inventoryChanged = true;
				}
				const bool discoveryChanged = player.inventory.learnCurrentInventoryTypes();

				if (inventoryChanged)
				{
					sendPlayerInventoryAndIncrementRevision(client);
				}
				else if (discoveryChanged)
				{
					sendPlayerInventoryNotIncrementRevision(client);
				}
				synchronizeGuideProgress(entry.first, client);
			}

			for (auto it = state.sentGuideProgress.begin(); it != state.sentGuideProgress.end();)
			{
				if (clients.find(it->first) == clients.end()) { it = state.sentGuideProgress.erase(it); }
				else { ++it; }
			}
		}

		void refreshMachineSimulationLevels()
		{
			auto &clients = getAllClientsReff();
			ServerChunkStorer &chunks = getServerChunkStorer();
			const int reducedRadius = std::max(getServerSettingsReff().simulationDistanceRadius, 2);
			for (const auto &entry : state.machines.allMachines())
			{
				const PrototypeMachineState &machine = entry.second;
				const int machineChunkX = divideChunk(machine.position.x);
				const int machineChunkZ = divideChunk(machine.position.z);
				SimulationLevel level = chunks.getChunkOrGetNull(machineChunkX, machineChunkZ) ?
					SimulationLevel::Dormant : SimulationLevel::Unloaded;
				for (const auto &clientEntry : clients)
				{
					const glm::ivec2 playerChunk = determineChunkThatIsEntityIn(
						clientEntry.second.playerData.entity.position);
					const int distance = std::max(std::abs(machineChunkX - playerChunk.x),
						std::abs(machineChunkZ - playerChunk.y));
					if (distance <= 2)
					{
						level = SimulationLevel::Full;
						break;
					}
					if (distance <= reducedRadius && level != SimulationLevel::Full)
					{
						level = SimulationLevel::Reduced;
					}
				}
				state.machines.setSimulationLevel(machine.id, level);
			}
		}
	}

	void resetServerNativeSystems()
	{
		state = ServerNativeState{};
	}

	bool loadServerNativeSystems(const WorldSaver &worldSaver)
	{
		state.manifest = makeV06WorldSchemaManifest();
		std::vector<char> manifestData;
		const sfs::Errors manifestLoad = sfs::safeLoad(manifestData,
			manifestPath(worldSaver).c_str(), false);
		if (manifestLoad == sfs::noError)
		{
			WorldSchemaManifest loaded;
			if (parseWorldSchemaManifest(manifestData.data(), manifestData.size(), loaded))
			{
				state.manifest = std::move(loaded);
				state.manifestExists = true;
			}
			else
			{
				++state.metrics.manifestFallbacks;
				state.manifestDirty = true;
				std::cerr << "Warning: invalid v0.6 schema manifest; using safe defaults.\n";
			}
		}
		else if (manifestLoad != sfs::couldNotOpenFinle)
		{
			++state.metrics.manifestFallbacks;
			state.manifestDirty = true;
		}

		const WorldSchemaManifest required = makeV06WorldSchemaManifest();
		for (const SchemaEntry &entry : required.schemas)
		{
			if (state.manifest.versionOf(entry.subsystem) == 0)
			{
				state.manifest.setVersion(entry.subsystem, entry.version);
				state.manifestDirty = true;
			}
		}

		std::vector<char> machineData;
		const sfs::Errors machineLoad = sfs::safeLoad(machineData,
			machinesPath(worldSaver).c_str(), false);
		if (machineLoad == sfs::noError)
		{
			PrototypeMachineParseReport report;
			if (!state.machines.restoreSnapshot(machineData.data(), machineData.size(), &report))
			{
				std::cerr << "Warning: invalid prototype machine data; keeping the safe empty runtime.\n";
				return false;
			}
		}

		std::vector<char> villagerData;
		const sfs::Errors villagerLoad = sfs::safeLoad(villagerData,
			villagersPath(worldSaver).c_str(), false);
		if (villagerLoad == sfs::noError)
		{
			if (!state.villagers.restoreSnapshot(villagerData.data(), villagerData.size()))
			{
				std::cerr << "Warning: invalid villager society data; keeping the safe empty runtime.\n";
				return false;
			}
		}
		return true;
	}

	bool saveServerNativeSystems(const WorldSaver &worldSaver, bool force)
	{
		bool success = true;
		if (force || state.manifestDirty || !state.manifestExists)
		{
			const std::vector<unsigned char> data = formatWorldSchemaManifest(state.manifest);
			if (data.empty() || sfs::safeSave(data.data(), data.size(),
				manifestPath(worldSaver).c_str(), true) != sfs::noError)
			{
				++state.metrics.saveFailures;
				success = false;
			}
			else
			{
				state.manifestDirty = false;
				state.manifestExists = true;
			}
		}

		if (force || state.machines.hasPersistenceChanges())
		{
			const std::vector<unsigned char> data = state.machines.formatSnapshot();
			if (data.empty() || sfs::safeSave(data.data(), data.size(),
				machinesPath(worldSaver).c_str(), true) != sfs::noError)
			{
				++state.metrics.saveFailures;
				success = false;
			}
			else { state.machines.acknowledgePersisted(); }
		}

		if (force || state.villagers.hasPersistenceChanges())
		{
			const std::vector<unsigned char> data = state.villagers.formatSnapshot();
			if (data.empty() || sfs::safeSave(data.data(), data.size(),
				villagersPath(worldSaver).c_str(), true) != sfs::noError)
			{
				++state.metrics.saveFailures;
				success = false;
			}
			else { state.villagers.acknowledgePersisted(); }
		}
		return success;
	}

	void updateServerNativeSystems(float deltaTime)
	{
		const auto start = std::chrono::steady_clock::now();
		updatePlayerFieldGuides();
		state.interestRefreshTimer -= deltaTime;
		if (state.interestRefreshTimer <= 0.f)
		{
			state.interestRefreshTimer = 1.f;
			refreshMachineSimulationLevels();
		}
		++state.currentTick;
		state.machines.update(state.currentTick, 64);
		state.villagers.update(state.currentTick, 128);
		const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now() - start).count();
		const std::uint64_t elapsedMicros = elapsed < 0 ? 0u : static_cast<std::uint64_t>(elapsed);
		state.metrics.accumulatedCpuMicroseconds += elapsedMicros;
		state.metrics.peakCpuMicroseconds = std::max(state.metrics.peakCpuMicroseconds,
			elapsedMicros);
	}

	ServerNativeSystemsMetrics getServerNativeSystemsMetrics()
	{
		ServerNativeSystemsMetrics result = state.metrics;
		result.machines = state.machines.metrics();
		result.villagers = state.villagers.metrics();
		return result;
	}

	PrototypeMachineRuntime &getPrototypeMachineRuntime()
	{
		return state.machines;
	}

	VillagerSocietyRuntime &getVillagerSocietyRuntime()
	{
		return state.villagers;
	}

	const ProcessingRecipeRegistry &getProcessingRecipeRegistry()
	{
		return state.processingRecipes;
	}
}
