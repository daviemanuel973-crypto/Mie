#include <native/gameplayScheduler.h>
#include <native/villagerSociety.h>
#include <rendering/chunkDistanceOrder.h>
#include <gameplay/navigationField.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

#define REQUIRE(condition) do { if (!(condition)) { std::cerr << "Failure at line " << __LINE__ << ": " #condition "\n"; std::exit(1); } } while (false)

using namespace mie::native;

void testScheduler()
{
	GameplayScheduler scheduler;
	std::vector<ScheduledGameplayJob> reference;
	std::minstd_rand rng(101);
	for (int i = 0; i < 250; ++i)
	{
		ScheduledGameplayJob job;
		job.priority = static_cast<GameplayJobPriority>(rng() % 4);
		job.category = static_cast<GameplayJobCategory>(rng() % 5);
		job.simulationLevel = static_cast<SimulationLevel>(rng() % 4);
		job.nextTick = rng() % 300;
		job.intervalTicks = 1 + rng() % 20;
		job.estimatedCost = 1 + rng() % 15;
		job.recurring = rng() % 2 != 0;
		job.id = scheduler.schedule(job);
		reference.push_back(job);
	}
	for (std::uint64_t tick = 0; tick < 700; ++tick)
	{
		if (tick % 7 == 0 && !reference.empty())
		{
			auto &job = reference[rng() % reference.size()];
			job.nextTick = tick + rng() % 100;
			REQUIRE(scheduler.setNextTick(job.id, job.nextTick));
		}
		if (tick % 13 == 0 && !reference.empty())
		{
			auto &job = reference[rng() % reference.size()];
			job.simulationLevel = static_cast<SimulationLevel>(rng() % 4);
			REQUIRE(scheduler.setSimulationLevel(job.id, job.simulationLevel));
		}
		if (tick % 17 == 0 && !reference.empty())
		{
			REQUIRE(scheduler.cancel(reference.back().id));
			reference.pop_back();
		}
		std::sort(reference.begin(), reference.end(), [](const auto &a, const auto &b)
		{
			if (a.priority != b.priority) { return a.priority < b.priority; }
			if (a.nextTick != b.nextTick) { return a.nextTick < b.nextTick; }
			return a.id < b.id;
		});
		const std::uint32_t budget = rng() % 80;
		SchedulerRunResult expected;
		for (auto &job : reference)
		{
			if (job.nextTick > tick) { continue; }
			const auto next = tick + static_cast<std::uint64_t>(job.intervalTicks) *
				simulationIntervalMultiplier(job.simulationLevel);
			if (job.simulationLevel == SimulationLevel::Unloaded) { job.nextTick = next; continue; }
			const bool critical = job.priority == GameplayJobPriority::Critical ||
				job.category == GameplayJobCategory::Combat || job.category == GameplayJobCategory::Interaction;
			if (!critical && job.estimatedCost > (expected.costUsed >= budget ? 0 : budget - expected.costUsed))
			{
				expected.deferred.push_back(job.id);
				continue;
			}
			expected.executed.push_back(job.id);
			expected.costUsed += job.estimatedCost;
			if (job.recurring) { job.nextTick = next; }
			else { job.id = 0; }
		}
		reference.erase(std::remove_if(reference.begin(), reference.end(),
			[](const auto &job) { return job.id == 0; }), reference.end());
		const auto actual = scheduler.run(tick, budget);
		REQUIRE(actual.executed == expected.executed);
		REQUIRE(actual.deferred == expected.deferred);
		REQUIRE(actual.costUsed == expected.costUsed);
		REQUIRE(scheduler.size() == reference.size());
		for (const auto &job : reference) { REQUIRE(scheduler.find(job.id)->nextTick == job.nextTick); }
	}
	scheduler.clear();
	ScheduledGameplayJob future;
	future.nextTick = 10000;
	for (int i = 0; i < 10000; ++i) { scheduler.schedule(future); }
	for (int tick = 0; tick < 100; ++tick) { REQUIRE(scheduler.run(tick, 64).executed.empty()); }
	REQUIRE(scheduler.metrics().jobsExamined == 0);
	scheduler.clear();
	future.nextTick = 0;
	future.priority = GameplayJobPriority::Critical;
	future.estimatedCost = std::numeric_limits<std::uint32_t>::max();
	scheduler.schedule(future);
	scheduler.schedule(future);
	const auto full = scheduler.run(std::numeric_limits<std::uint64_t>::max(), 1);
	REQUIRE(full.executed.size() == 2);
	REQUIRE(full.costUsed == std::numeric_limits<std::uint32_t>::max());
	REQUIRE(scheduler.find(full.executed.front())->nextTick == std::numeric_limits<std::uint64_t>::max());
	GameplayScheduler copy = scheduler;
	REQUIRE(copy.cancel(full.executed.front()));
	REQUIRE(scheduler.size() == 2 && copy.size() == 1);
}

void testVillagers()
{
	VillagerSocietyRuntime villagers;
	for (std::uint64_t id = 1; id <= MAX_VILLAGERS; ++id)
	{
		REQUIRE(villagers.createVillager(id, 1, VillagerProfession::Farmer, {}, {}, "Ari"));
	}
	villagers.update(6000, 128);
	REQUIRE(villagers.metrics().updatesExecuted == 0);
	for (std::uint64_t id = 1; id <= MAX_VILLAGERS; ++id)
	{
		REQUIRE(villagers.setSimulationLevel(id, SimulationLevel::Full));
	}
	for (std::uint64_t tick = 6000; tick < 6016; ++tick) { villagers.update(tick, 128); }
	REQUIRE(villagers.metrics().updatesExecuted == MAX_VILLAGERS);
	for (const auto &entry : villagers.allVillagers()) { REQUIRE(entry.second.energy == 99); }
	const auto snapshot = villagers.formatSnapshot();
	VillagerSocietyRuntime restored;
	REQUIRE(restored.restoreSnapshot(reinterpret_cast<const char *>(snapshot.data()), snapshot.size()));
	for (std::uint64_t tick = 6016; tick < 6060; ++tick)
	{
		villagers.update(tick, 128);
		restored.update(tick, 128);
	}
	REQUIRE(villagers.formatSnapshot() == restored.formatSnapshot());
	const auto before = restored.formatSnapshot();
	REQUIRE(!restored.restoreSnapshot(reinterpret_cast<const char *>(snapshot.data()), snapshot.size() - 1));
	REQUIRE(restored.formatSnapshot() == before);
	REQUIRE(restored.setSimulationLevel(1, SimulationLevel::Unloaded));
	const auto energy = restored.find(1)->energy;
	restored.update(10000, MAX_VILLAGERS);
	REQUIRE(restored.find(1)->energy == energy);
	REQUIRE(restored.setSimulationLevel(1, SimulationLevel::Reduced));
	restored.update(10001, MAX_VILLAGERS);
	REQUIRE(restored.find(1)->energy == energy - 1);
	REQUIRE(restored.find(1)->nextUpdateTick == 10081);
	restored.clear();
	restored.update(20000, 128);
	REQUIRE(restored.metrics().updatesExecuted == 0);
}

void testChunkOrder()
{
	mie::rendering::ChunkDistanceOrder cache;
	for (int side : {2, 10, 30, 102, 10})
	{
		for (int origin : {-3, 0, side / 2, side + 3})
		{
			const auto &order = cache.backToFront(side, origin, origin + 1);
			REQUIRE(order.size() == static_cast<std::size_t>(side * side));
			std::vector<bool> seen(order.size());
			double previous = std::numeric_limits<double>::infinity();
			for (int index : order)
			{
				REQUIRE(index >= 0 && static_cast<std::size_t>(index) < seen.size() && !seen[index]);
				seen[index] = true;
				const double dx = index / side - origin, dz = index % side - origin - 1;
				const double distance = dx * dx + dz * dz;
				REQUIRE(distance <= previous);
				previous = distance;
			}
			const auto rebuilds = cache.rebuildCount();
			const auto data = order.data();
			for (int i = 0; i < 100; ++i) { REQUIRE(cache.backToFront(side, origin, origin + 1).data() == data); }
			REQUIRE(cache.rebuildCount() == rebuilds);
		}
	}
	REQUIRE(cache.backToFront(103, 0, 0).empty());
	REQUIRE(cache.backToFront(10, 5, 5).size() == 100);
}

int main()
{
	testScheduler();
	testVillagers();
	testChunkOrder();
	std::cout << "Low-first optimization contracts passed.\n";
}
