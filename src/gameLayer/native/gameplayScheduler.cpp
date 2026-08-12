#include <native/gameplayScheduler.h>

#include <algorithm>
#include <limits>

namespace mie::native
{
	std::uint32_t simulationIntervalMultiplier(SimulationLevel level)
	{
		switch (level)
		{
			case SimulationLevel::Full: return 1;
			case SimulationLevel::Reduced: return 4;
			case SimulationLevel::Dormant: return 20;
			case SimulationLevel::Unloaded: return 60;
		}
		return 1;
	}

	std::uint64_t GameplayScheduler::schedule(ScheduledGameplayJob job)
	{
		job.id = nextJobId++;
		if (nextJobId == 0) { nextJobId = 1; }
		job.intervalTicks = std::max(job.intervalTicks, 1u);
		job.estimatedCost = std::max(job.estimatedCost, 1u);
		jobs[job.id] = job;
		schedulerMetrics.peakQueueSize = std::max(schedulerMetrics.peakQueueSize,
			static_cast<std::uint32_t>(jobs.size()));
		return job.id;
	}

	bool GameplayScheduler::cancel(std::uint64_t jobId)
	{
		return jobs.erase(jobId) != 0;
	}

	bool GameplayScheduler::setSimulationLevel(std::uint64_t jobId, SimulationLevel level)
	{
		auto found = jobs.find(jobId);
		if (found == jobs.end()) { return false; }
		found->second.simulationLevel = level;
		return true;
	}

	bool GameplayScheduler::setNextTick(std::uint64_t jobId, std::uint64_t nextTick)
	{
		auto found = jobs.find(jobId);
		if (found == jobs.end()) { return false; }
		found->second.nextTick = nextTick;
		return true;
	}

	SchedulerRunResult GameplayScheduler::run(std::uint64_t currentTick, std::uint32_t budget)
	{
		std::vector<std::uint64_t> due;
		due.reserve(jobs.size());
		for (const auto &entry : jobs)
		{
			if (entry.second.nextTick <= currentTick) { due.push_back(entry.first); }
		}
		std::sort(due.begin(), due.end(), [&](std::uint64_t leftId, std::uint64_t rightId)
		{
			const ScheduledGameplayJob &left = jobs.at(leftId);
			const ScheduledGameplayJob &right = jobs.at(rightId);
			if (left.priority != right.priority) { return left.priority < right.priority; }
			if (left.nextTick != right.nextTick) { return left.nextTick < right.nextTick; }
			return left.id < right.id;
		});

		SchedulerRunResult result;
		for (std::uint64_t jobId : due)
		{
			auto found = jobs.find(jobId);
			if (found == jobs.end()) { continue; }
			ScheduledGameplayJob &job = found->second;
			const std::uint64_t interval = static_cast<std::uint64_t>(job.intervalTicks) *
				simulationIntervalMultiplier(job.simulationLevel);
			if (job.simulationLevel == SimulationLevel::Unloaded)
			{
				job.nextTick = currentTick + interval;
				++schedulerMetrics.jobsDiscarded;
				continue;
			}

			const bool critical = job.priority == GameplayJobPriority::Critical ||
				job.category == GameplayJobCategory::Combat ||
				job.category == GameplayJobCategory::Interaction;
			const std::uint32_t remainingBudget = result.costUsed >= budget ?
				0 : budget - result.costUsed;
			if (!critical && job.estimatedCost > remainingBudget)
			{
				result.deferred.push_back(jobId);
				++schedulerMetrics.jobsDeferred;
				continue;
			}

			result.executed.push_back(jobId);
			result.costUsed += job.estimatedCost;
			++schedulerMetrics.jobsExecuted;
			++schedulerMetrics.executedByCategory[static_cast<std::size_t>(job.category)];
			if (job.recurring) { job.nextTick = currentTick + interval; }
			else { jobs.erase(found); }
		}
		return result;
	}

	void GameplayScheduler::clear()
	{
		jobs.clear();
		nextJobId = 1;
		schedulerMetrics = {};
	}

	const ScheduledGameplayJob *GameplayScheduler::find(std::uint64_t jobId) const
	{
		const auto found = jobs.find(jobId);
		return found == jobs.end() ? nullptr : &found->second;
	}
}
