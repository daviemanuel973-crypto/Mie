#include <native/gameplayScheduler.h>
#include <native/villagerSociety.h>
#include <rendering/chunkDistanceOrder.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using namespace mie::native;

template<class Setup, class Step>
void measure(const char *name, int iterations, Setup setup, Step step)
{
	std::vector<double> timings;
	std::uint64_t checksum = 0;
	for (int repeat = 0; repeat < 7; ++repeat)
	{
		auto state = setup();
		const auto start = std::chrono::steady_clock::now();
		for (int tick = 0; tick < iterations; ++tick) { checksum += step(state, tick); }
		timings.push_back(std::chrono::duration<double, std::micro>(
			std::chrono::steady_clock::now() - start).count() / iterations);
	}
	std::sort(timings.begin(), timings.end());
	std::cout << "{\"case\":\"" << name << "\",\"iterations\":" << iterations
		<< ",\"repetitions\":7,\"median_us\":" << timings[3]
		<< ",\"min_us\":" << timings.front() << ",\"max_us\":" << timings.back()
		<< ",\"checksum\":" << checksum << "}\n";
}

int main()
{
	measure("scheduler_50000_sleeping", 2000, []()
	{
		GameplayScheduler scheduler;
		ScheduledGameplayJob job;
		job.nextTick = 1000000;
		for (int i = 0; i < 50000; ++i) { scheduler.schedule(job); }
		return scheduler;
	}, [](auto &scheduler, int tick) { return scheduler.run(tick, 64).costUsed; });

	measure("scheduler_2048_due_budget64", 1000, []()
	{
		GameplayScheduler scheduler;
		for (int i = 0; i < 2048; ++i) { scheduler.schedule({}); }
		return scheduler;
	}, [](auto &scheduler, int tick)
	{
		const auto result = scheduler.run(tick, 64);
		std::uint64_t checksum = result.costUsed + result.deferred.size();
		for (auto id : result.executed) { checksum += id; }
		return checksum;
	});

	measure("villagers_2048_unloaded", 3000, []()
	{
		VillagerSocietyRuntime villagers;
		for (std::uint64_t id = 1; id <= MAX_VILLAGERS; ++id)
		{
			villagers.createVillager(id, 1, VillagerProfession::Farmer, {}, {}, "Ari");
		}
		return villagers;
	}, [](auto &villagers, int tick)
	{
		villagers.update(tick, 128);
		return villagers.metrics().updatesExecuted;
	});

	struct ChunkOrderState { mie::rendering::ChunkDistanceOrder cache; };
	for (int side : {10, 30, 102})
	{
		const auto name = std::string("chunk_order_") + std::to_string(side * side);
		measure(name.c_str(), 1000, []() { return ChunkOrderState{}; }, [side](auto &state, int)
		{
#ifdef MIE_PERF_BASELINE
			(void)state;
			std::vector<int> order(side * side);
			std::iota(order.begin(), order.end(), 0);
			std::sort(order.begin(), order.end(), [side](int a, int b)
			{
				const int ax = a / side - side / 2, az = a % side - side / 2;
				const int bx = b / side - side / 2, bz = b % side - side / 2;
				const int da = ax * ax + az * az, db = bx * bx + bz * bz;
				return da != db ? da > db : a < b;
			});
#else
			const auto &order = state.cache.backToFront(side, side / 2, side / 2);
#endif
			std::uint64_t checksum = 0;
			for (auto index : order) { checksum = checksum * 31 + static_cast<unsigned int>(index); }
			return checksum;
		});
	}
}
