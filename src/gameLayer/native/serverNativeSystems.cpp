#include <native/serverNativeSystems.h>
#include <native/contentRegistry.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <blocks.h>
#include <gameplay/allentities.h>
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
		ItemTypes::lastItem == V07_LAST_ITEM_EXCLUSIVE,
		"persisted v0.5/v0.7 item IDs changed; append without renumbering existing content");
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
			ServerNativeSystemsMetrics metrics;
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
		return success;
	}

	void updateServerNativeSystems(float deltaTime)
	{
		const auto start = std::chrono::steady_clock::now();
		state.interestRefreshTimer -= deltaTime;
		if (state.interestRefreshTimer <= 0.f)
		{
			state.interestRefreshTimer = 1.f;
			refreshMachineSimulationLevels();
		}
		state.machines.update(++state.currentTick, 64);
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
		return result;
	}

	PrototypeMachineRuntime &getPrototypeMachineRuntime()
	{
		return state.machines;
	}

	const ProcessingRecipeRegistry &getProcessingRecipeRegistry()
	{
		return state.processingRecipes;
	}
}
