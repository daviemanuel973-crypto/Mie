#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mie::native
{
	enum class GameplayJobCategory : std::uint8_t
	{
		Combat,
		Interaction,
		Machine,
		NetworkTopology,
		Routine,
		Count,
	};

	enum class GameplayJobPriority : std::uint8_t
	{
		Critical,
		High,
		Normal,
		Low,
	};

	enum class SimulationLevel : std::uint8_t
	{
		Full,
		Reduced,
		Dormant,
		Unloaded,
	};

	std::uint32_t simulationIntervalMultiplier(SimulationLevel level);

	struct ScheduledGameplayJob
	{
		std::uint64_t id = 0;
		GameplayJobCategory category = GameplayJobCategory::Routine;
		GameplayJobPriority priority = GameplayJobPriority::Normal;
		SimulationLevel simulationLevel = SimulationLevel::Full;
		std::uint64_t nextTick = 0;
		std::uint32_t intervalTicks = 1;
		std::uint32_t estimatedCost = 1;
		bool recurring = true;
	};

	struct SchedulerMetrics
	{
		std::uint64_t jobsExecuted = 0;
		std::uint64_t jobsDeferred = 0;
		std::uint64_t jobsDiscarded = 0;
		std::uint32_t peakQueueSize = 0;
		std::array<std::uint64_t,
			static_cast<std::size_t>(GameplayJobCategory::Count)> executedByCategory{};
	};

	struct SchedulerRunResult
	{
		std::vector<std::uint64_t> executed;
		std::vector<std::uint64_t> deferred;
		std::uint32_t costUsed = 0;
	};

	class GameplayScheduler
	{
	public:
		std::uint64_t schedule(ScheduledGameplayJob job);
		bool cancel(std::uint64_t jobId);
		bool setSimulationLevel(std::uint64_t jobId, SimulationLevel level);
		bool setNextTick(std::uint64_t jobId, std::uint64_t nextTick);
		SchedulerRunResult run(std::uint64_t currentTick, std::uint32_t budget);
		void clear();

		const ScheduledGameplayJob *find(std::uint64_t jobId) const;
		std::size_t size() const { return jobs.size(); }
		const SchedulerMetrics &metrics() const { return schedulerMetrics; }

	private:
		std::uint64_t nextJobId = 1;
		std::unordered_map<std::uint64_t, ScheduledGameplayJob> jobs;
		SchedulerMetrics schedulerMetrics;
	};
}
