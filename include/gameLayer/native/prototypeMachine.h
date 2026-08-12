#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include <native/gameplayFoundation.h>
#include <native/gameplayScheduler.h>

namespace mie::native
{
	constexpr std::uint32_t PROTOTYPE_MACHINE_FORMAT_VERSION = 1;
	constexpr std::uint32_t PROTOTYPE_MACHINE_PROCESS_TICKS = 100;

	struct MachinePosition
	{
		int x = 0;
		int y = 0;
		int z = 0;
	};

	enum class PrototypeMachineStatus : std::uint8_t
	{
		Idle,
		Processing,
		Blocked,
	};

	struct PrototypeMachineState
	{
		std::uint64_t id = 0;
		MachinePosition position;
		std::uint32_t inputUnits = 0;
		std::uint32_t outputUnits = 0;
		std::uint32_t processTicksRemaining = 0;
		std::uint64_t revision = 1;
		SimulationLevel simulationLevel = SimulationLevel::Full;
		PrototypeMachineStatus status = PrototypeMachineStatus::Idle;
		DirtyFlag dirty = DirtyFlag::None;
	};

	struct PrototypeMachineDelta
	{
		PrototypeMachineState state;
	};

	struct PrototypeMachineMetrics
	{
		std::uint64_t updateCalls = 0;
		std::uint64_t completedProcesses = 0;
		std::uint64_t sanitizedValues = 0;
		std::uint64_t fallbackValues = 0;
		std::uint64_t bytesSaved = 0;
		std::uint64_t bytesReplicated = 0;
		std::uint32_t activeMachines = 0;
		std::uint32_t reducedMachines = 0;
		std::uint32_t dormantMachines = 0;
		SchedulerMetrics scheduler;
	};

	struct PrototypeMachineParseReport
	{
		std::uint64_t sanitizedValues = 0;
		std::uint64_t fallbackValues = 0;
	};

	class PrototypeMachineRuntime
	{
	public:
		bool createMachine(std::uint64_t id, MachinePosition position);
		bool removeMachine(std::uint64_t id);
		bool insertInput(std::uint64_t id, std::uint32_t units, std::uint64_t currentTick);
		std::uint32_t takeOutput(std::uint64_t id, std::uint32_t maxUnits);
		bool setSimulationLevel(std::uint64_t id, SimulationLevel level);
		void update(std::uint64_t currentTick, std::uint32_t budget);

		const PrototypeMachineState *find(std::uint64_t id) const;
		std::vector<PrototypeMachineDelta> collectDeltas(const ObserverContext &observer,
			std::uint64_t afterRevision = 0);
		void acknowledgePersisted();
		bool hasPersistenceChanges() const;
		void clear();

		std::vector<unsigned char> formatSnapshot();
		bool restoreSnapshot(const char *data, std::size_t size,
			PrototypeMachineParseReport *report = nullptr);
		PrototypeMachineMetrics metrics() const;
		const std::unordered_map<std::uint64_t, PrototypeMachineState> &allMachines() const
		{
			return machines;
		}

	private:
		void markChanged(PrototypeMachineState &machine, DirtyFlag flags);
		void ensureScheduled(PrototypeMachineState &machine, std::uint64_t currentTick);
		void processMachine(std::uint64_t machineId, std::uint64_t currentTick);

		std::unordered_map<std::uint64_t, PrototypeMachineState> machines;
		std::unordered_map<std::uint64_t, std::uint64_t> machineToJob;
		std::unordered_map<std::uint64_t, std::uint64_t> jobToMachine;
		GameplayScheduler scheduler;
		GameplayEventStream eventStream;
		PrototypeMachineMetrics runtimeMetrics;
		bool persistenceDirty = false;
	};
}
