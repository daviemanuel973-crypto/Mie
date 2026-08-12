#include <native/prototypeMachine.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <unordered_set>

namespace mie::native
{
	namespace
	{
		constexpr std::array<unsigned char, 8> machineMagic = {
			'M', 'I', 'E', 'M', 'A', 'C', 'H', 'N'
		};
		constexpr std::uint32_t maxMachines = 100000;
		constexpr std::uint32_t maxUnits = 1000000;

		template<typename T>
		void append(std::vector<unsigned char> &data, const T &value)
		{
			const std::size_t oldSize = data.size();
			data.resize(oldSize + sizeof(value));
			std::memcpy(data.data() + oldSize, &value, sizeof(value));
		}

		template<typename T>
		bool read(const char *data, std::size_t size, std::size_t &offset, T &value)
		{
			if (!data || offset > size || sizeof(value) > size - offset) { return false; }
			std::memcpy(&value, data + offset, sizeof(value));
			offset += sizeof(value);
			return true;
		}

		int floorChunk(int block)
		{
			return block >= 0 ? block / 16 : -((15 - block) / 16);
		}
	}

	bool PrototypeMachineRuntime::createMachine(std::uint64_t id, MachinePosition position)
	{
		if (id == 0 || machines.size() >= maxMachines || machines.find(id) != machines.end())
		{
			return false;
		}
		PrototypeMachineState machine;
		machine.id = id;
		machine.position = position;
		machine.dirty = DirtyFlag::Persistence | DirtyFlag::Network | DirtyFlag::Topology;
		machines.emplace(id, machine);
		persistenceDirty = true;
		eventStream.publish("mie:machine/created", id, 0);
		return true;
	}

	bool PrototypeMachineRuntime::removeMachine(std::uint64_t id)
	{
		const auto job = machineToJob.find(id);
		if (job != machineToJob.end())
		{
			scheduler.cancel(job->second);
			jobToMachine.erase(job->second);
			machineToJob.erase(job);
		}
		if (machines.erase(id) == 0) { return false; }
		persistenceDirty = true;
		eventStream.publish("mie:machine/removed", id, 0);
		return true;
	}

	bool PrototypeMachineRuntime::insertInput(std::uint64_t id, std::uint32_t units,
		std::uint64_t currentTick)
	{
		auto found = machines.find(id);
		if (found == machines.end() || units == 0 ||
			units > maxUnits - found->second.inputUnits)
		{
			return false;
		}
		PrototypeMachineState &machine = found->second;
		machine.inputUnits += units;
		if (machine.status == PrototypeMachineStatus::Idle)
		{
			machine.status = PrototypeMachineStatus::Processing;
			machine.processTicksRemaining = PROTOTYPE_MACHINE_PROCESS_TICKS;
		}
		markChanged(machine, DirtyFlag::Persistence | DirtyFlag::Network |
			DirtyFlag::Inventory | DirtyFlag::Process);
		ensureScheduled(machine, currentTick);
		return true;
	}

	std::uint32_t PrototypeMachineRuntime::takeOutput(std::uint64_t id, std::uint32_t maxTaken)
	{
		auto found = machines.find(id);
		if (found == machines.end() || maxTaken == 0) { return 0; }
		PrototypeMachineState &machine = found->second;
		const std::uint32_t taken = std::min(maxTaken, machine.outputUnits);
		if (taken != 0)
		{
			machine.outputUnits -= taken;
			markChanged(machine, DirtyFlag::Persistence | DirtyFlag::Network |
				DirtyFlag::Inventory);
		}
		return taken;
	}

	bool PrototypeMachineRuntime::setSimulationLevel(std::uint64_t id, SimulationLevel level)
	{
		auto found = machines.find(id);
		if (found == machines.end()) { return false; }
		found->second.simulationLevel = level;
		const auto job = machineToJob.find(id);
		if (job != machineToJob.end()) { scheduler.setSimulationLevel(job->second, level); }
		return true;
	}

	void PrototypeMachineRuntime::update(std::uint64_t currentTick, std::uint32_t budget)
	{
		const SchedulerRunResult result = scheduler.run(currentTick, budget);
		for (std::uint64_t jobId : result.executed)
		{
			const auto machine = jobToMachine.find(jobId);
			if (machine != jobToMachine.end()) { processMachine(machine->second, currentTick); }
		}
		++runtimeMetrics.updateCalls;
	}

	const PrototypeMachineState *PrototypeMachineRuntime::find(std::uint64_t id) const
	{
		const auto found = machines.find(id);
		return found == machines.end() ? nullptr : &found->second;
	}

	std::vector<PrototypeMachineDelta> PrototypeMachineRuntime::collectDeltas(
		const ObserverContext &observer, std::uint64_t afterRevision)
	{
		std::vector<PrototypeMachineDelta> result;
		for (const auto &entry : machines)
		{
			const PrototypeMachineState &machine = entry.second;
			InterestDescriptor interest;
			interest.scope = InterestScope::Chunk;
			interest.chunkX = floorChunk(machine.position.x);
			interest.chunkZ = floorChunk(machine.position.z);
			if (machine.revision > afterRevision && isRelevantToObserver(interest, observer))
			{
				result.push_back({machine});
				runtimeMetrics.bytesReplicated += sizeof(PrototypeMachineState);
			}
		}
		std::sort(result.begin(), result.end(), [](const PrototypeMachineDelta &left,
			const PrototypeMachineDelta &right) { return left.state.id < right.state.id; });
		return result;
	}

	void PrototypeMachineRuntime::acknowledgePersisted()
	{
		for (auto &entry : machines)
		{
			entry.second.dirty = without(entry.second.dirty, DirtyFlag::Persistence);
		}
		persistenceDirty = false;
	}

	bool PrototypeMachineRuntime::hasPersistenceChanges() const
	{
		return persistenceDirty;
	}

	void PrototypeMachineRuntime::clear()
	{
		machines.clear();
		machineToJob.clear();
		jobToMachine.clear();
		scheduler.clear();
		eventStream.clear();
		runtimeMetrics = {};
		persistenceDirty = false;
	}

	std::vector<unsigned char> PrototypeMachineRuntime::formatSnapshot()
	{
		std::vector<unsigned char> result(machineMagic.begin(), machineMagic.end());
		append(result, PROTOTYPE_MACHINE_FORMAT_VERSION);
		const std::uint32_t count = static_cast<std::uint32_t>(machines.size());
		append(result, count);
		std::vector<std::uint64_t> ids;
		ids.reserve(machines.size());
		for (const auto &entry : machines) { ids.push_back(entry.first); }
		std::sort(ids.begin(), ids.end());
		for (std::uint64_t id : ids)
		{
			const PrototypeMachineState &machine = machines.at(id);
			append(result, machine.id);
			append(result, machine.position.x);
			append(result, machine.position.y);
			append(result, machine.position.z);
			append(result, machine.inputUnits);
			append(result, machine.outputUnits);
			append(result, machine.processTicksRemaining);
			append(result, machine.revision);
			append(result, static_cast<std::uint8_t>(machine.simulationLevel));
			append(result, static_cast<std::uint8_t>(machine.status));
		}
		runtimeMetrics.bytesSaved += result.size();
		return result;
	}

	bool PrototypeMachineRuntime::restoreSnapshot(const char *data, std::size_t size,
		PrototypeMachineParseReport *report)
	{
		if (!data || size < machineMagic.size() + sizeof(std::uint32_t) * 2 ||
			!std::equal(machineMagic.begin(), machineMagic.end(),
				reinterpret_cast<const unsigned char *>(data)))
		{
			return false;
		}
		std::size_t offset = machineMagic.size();
		std::uint32_t version = 0;
		std::uint32_t count = 0;
		if (!read(data, size, offset, version) || version != PROTOTYPE_MACHINE_FORMAT_VERSION ||
			!read(data, size, offset, count) || count > maxMachines)
		{
			return false;
		}

		PrototypeMachineRuntime restored;
		PrototypeMachineParseReport localReport;
		for (std::uint32_t index = 0; index < count; ++index)
		{
			PrototypeMachineState machine;
			std::uint8_t level = 0;
			std::uint8_t status = 0;
			if (!read(data, size, offset, machine.id) ||
				!read(data, size, offset, machine.position.x) ||
				!read(data, size, offset, machine.position.y) ||
				!read(data, size, offset, machine.position.z) ||
				!read(data, size, offset, machine.inputUnits) ||
				!read(data, size, offset, machine.outputUnits) ||
				!read(data, size, offset, machine.processTicksRemaining) ||
				!read(data, size, offset, machine.revision) ||
				!read(data, size, offset, level) || !read(data, size, offset, status))
			{
				return false;
			}
			if (machine.id == 0 || restored.machines.find(machine.id) != restored.machines.end())
			{
				return false;
			}
			if (machine.inputUnits > maxUnits)
			{
				machine.inputUnits = maxUnits;
				++localReport.sanitizedValues;
			}
			if (machine.outputUnits > maxUnits)
			{
				machine.outputUnits = maxUnits;
				++localReport.sanitizedValues;
			}
			if (machine.processTicksRemaining > PROTOTYPE_MACHINE_PROCESS_TICKS)
			{
				machine.processTicksRemaining = PROTOTYPE_MACHINE_PROCESS_TICKS;
				++localReport.sanitizedValues;
			}
			if (level > static_cast<std::uint8_t>(SimulationLevel::Unloaded))
			{
				level = static_cast<std::uint8_t>(SimulationLevel::Dormant);
				++localReport.fallbackValues;
			}
			if (status > static_cast<std::uint8_t>(PrototypeMachineStatus::Blocked))
			{
				status = static_cast<std::uint8_t>(PrototypeMachineStatus::Idle);
				++localReport.fallbackValues;
			}
			machine.simulationLevel = static_cast<SimulationLevel>(level);
			machine.status = static_cast<PrototypeMachineStatus>(status);
			if (machine.inputUnits == 0 && machine.status == PrototypeMachineStatus::Processing)
			{
				machine.status = PrototypeMachineStatus::Idle;
				machine.processTicksRemaining = 0;
				++localReport.sanitizedValues;
			}
			machine.revision = std::max<std::uint64_t>(machine.revision, 1);
			machine.dirty = DirtyFlag::None;
			restored.machines.emplace(machine.id, machine);
		}
		if (offset != size) { return false; }

		for (auto &entry : restored.machines)
		{
			if (entry.second.status == PrototypeMachineStatus::Processing)
			{
				restored.ensureScheduled(entry.second, 0);
			}
		}
		restored.runtimeMetrics.sanitizedValues = localReport.sanitizedValues;
		restored.runtimeMetrics.fallbackValues = localReport.fallbackValues;
		*this = std::move(restored);
		if (report) { *report = localReport; }
		return true;
	}

	PrototypeMachineMetrics PrototypeMachineRuntime::metrics() const
	{
		PrototypeMachineMetrics result = runtimeMetrics;
		result.scheduler = scheduler.metrics();
		for (const auto &entry : machines)
		{
			if (entry.second.status == PrototypeMachineStatus::Processing) { ++result.activeMachines; }
			if (entry.second.simulationLevel == SimulationLevel::Reduced) { ++result.reducedMachines; }
			if (entry.second.simulationLevel == SimulationLevel::Dormant) { ++result.dormantMachines; }
		}
		return result;
	}

	void PrototypeMachineRuntime::markChanged(PrototypeMachineState &machine, DirtyFlag flags)
	{
		machine.dirty |= flags;
		if (hasFlag(flags, DirtyFlag::Persistence)) { persistenceDirty = true; }
		++machine.revision;
		if (machine.revision == 0) { machine.revision = 1; }
	}

	void PrototypeMachineRuntime::ensureScheduled(PrototypeMachineState &machine,
		std::uint64_t currentTick)
	{
		if (machineToJob.find(machine.id) != machineToJob.end()) { return; }
		ScheduledGameplayJob job;
		job.category = GameplayJobCategory::Machine;
		job.priority = GameplayJobPriority::Normal;
		job.simulationLevel = machine.simulationLevel;
		job.nextTick = currentTick + 1;
		job.intervalTicks = 1;
		job.estimatedCost = 1;
		const std::uint64_t jobId = scheduler.schedule(job);
		machineToJob[machine.id] = jobId;
		jobToMachine[jobId] = machine.id;
	}

	void PrototypeMachineRuntime::processMachine(std::uint64_t machineId,
		std::uint64_t currentTick)
	{
		auto found = machines.find(machineId);
		if (found == machines.end()) { return; }
		PrototypeMachineState &machine = found->second;
		if (machine.status != PrototypeMachineStatus::Processing || machine.inputUnits == 0)
		{
			const auto job = machineToJob.find(machineId);
			if (job != machineToJob.end())
			{
				scheduler.cancel(job->second);
				jobToMachine.erase(job->second);
				machineToJob.erase(job);
			}
			return;
		}

		const std::uint32_t elapsed = simulationIntervalMultiplier(machine.simulationLevel);
		if (elapsed >= machine.processTicksRemaining)
		{
			--machine.inputUnits;
			if (machine.outputUnits < maxUnits) { ++machine.outputUnits; }
			++runtimeMetrics.completedProcesses;
			eventStream.publish("mie:machine/process_completed", machine.id, currentTick);
			if (machine.inputUnits == 0)
			{
				machine.status = PrototypeMachineStatus::Idle;
				machine.processTicksRemaining = 0;
				const auto job = machineToJob.find(machine.id);
				if (job != machineToJob.end())
				{
					scheduler.cancel(job->second);
					jobToMachine.erase(job->second);
					machineToJob.erase(job);
				}
			}
			else { machine.processTicksRemaining = PROTOTYPE_MACHINE_PROCESS_TICKS; }
		}
		else { machine.processTicksRemaining -= elapsed; }
		markChanged(machine, DirtyFlag::Persistence | DirtyFlag::Network |
			DirtyFlag::Process | DirtyFlag::Inventory);
	}
}
