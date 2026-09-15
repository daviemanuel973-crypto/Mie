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
		if (static_cast<unsigned int>(job.priority) > 3u ||
			static_cast<unsigned int>(job.category) >=
				static_cast<unsigned int>(GameplayJobCategory::Count) ||
			static_cast<unsigned int>(job.simulationLevel) > 3u) { return 0; }
		while (nextJobId == 0 || jobs.find(nextJobId) != jobs.end()) { ++nextJobId; }
		job.id = nextJobId++;
		if (nextJobId == 0) { nextJobId = 1; }
		job.intervalTicks = std::max(job.intervalTicks, 1u);
		job.estimatedCost = std::max(job.estimatedCost, 1u);
		jobs[job.id] = job;
		deadlines[static_cast<std::size_t>(job.priority)].emplace(job.nextTick, job.id);
		schedulerMetrics.peakQueueSize = std::max(schedulerMetrics.peakQueueSize,
			static_cast<std::uint32_t>(jobs.size()));
		return job.id;
	}

	bool GameplayScheduler::cancel(std::uint64_t jobId)
	{
		const auto found = jobs.find(jobId);
		if (found == jobs.end()) { return false; }
		const auto &job = found->second;
		deadlines[static_cast<std::size_t>(job.priority)].erase({job.nextTick, job.id});
		jobs.erase(found);
		return true;
	}

	bool GameplayScheduler::setSimulationLevel(std::uint64_t jobId, SimulationLevel level)
	{
		auto found = jobs.find(jobId);
		if (found == jobs.end()) { return false; }
		if (static_cast<unsigned int>(level) > 3u) { return false; }
		found->second.simulationLevel = level;
		return true;
	}

	bool GameplayScheduler::setNextTick(std::uint64_t jobId, std::uint64_t nextTick)
	{
		auto found = jobs.find(jobId);
		if (found == jobs.end()) { return false; }
		auto &queue = deadlines[static_cast<std::size_t>(found->second.priority)];
		auto node = queue.extract({found->second.nextTick, jobId});
		found->second.nextTick = nextTick;
		node.value().first = nextTick;
		queue.insert(std::move(node));
		return true;
	}

	SchedulerRunResult GameplayScheduler::run(std::uint64_t currentTick, std::uint32_t budget)
	{
		// Snapshot only due IDs, in the same priority/tick/ID order as v0.10.0.
		// A recurring job executes at most once per call, even at UINT64_MAX.
		dueJobs.clear();
		for (const auto &queue : deadlines)
		{
			for (auto it = queue.begin(); it != queue.end() && it->first <= currentTick; ++it)
			{
				dueJobs.push_back(it->second);
			}
		}

		SchedulerRunResult result;
		for (std::uint64_t jobId : dueJobs)
		{
			++schedulerMetrics.jobsExamined;
			auto found = jobs.find(jobId);
			if (found == jobs.end()) { continue; }
			ScheduledGameplayJob &job = found->second;
			const std::uint64_t interval = static_cast<std::uint64_t>(job.intervalTicks) *
				simulationIntervalMultiplier(job.simulationLevel);
			const std::uint64_t nextTick = currentTick >
				std::numeric_limits<std::uint64_t>::max() - interval ?
				std::numeric_limits<std::uint64_t>::max() : currentTick + interval;
			if (job.simulationLevel == SimulationLevel::Unloaded)
			{
				setNextTick(jobId, nextTick);
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
			result.costUsed = job.estimatedCost >
				std::numeric_limits<std::uint32_t>::max() - result.costUsed ?
				std::numeric_limits<std::uint32_t>::max() : result.costUsed + job.estimatedCost;
			++schedulerMetrics.jobsExecuted;
			++schedulerMetrics.executedByCategory[static_cast<std::size_t>(job.category)];
			if (job.recurring) { setNextTick(jobId, nextTick); }
			else { cancel(jobId); }
		}
		return result;
	}

	void GameplayScheduler::clear()
	{
		jobs.clear();
		for (auto &queue : deadlines) { queue.clear(); }
		std::vector<std::uint64_t>().swap(dueJobs);
		nextJobId = 1;
		schedulerMetrics = {};
	}

	const ScheduledGameplayJob *GameplayScheduler::find(std::uint64_t jobId) const
	{
		const auto found = jobs.find(jobId);
		return found == jobs.end() ? nullptr : &found->second;
	}
}
