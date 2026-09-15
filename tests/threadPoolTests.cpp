#include <threadPool.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#define REQUIRE(condition) do { if (!(condition)) { std::cerr << "Thread-pool failure at " << __LINE__ << "\n"; std::exit(1); } } while (false)

namespace
{
	std::atomic<unsigned int> nextTask{0};
	std::atomic<unsigned int> completions{0};
	std::vector<unsigned int> results;
	unsigned int currentBatch = 0;

	void worker(int index, ThreadPool &pool)
	{
		while (pool.waitForWork(index))
		{
			for (;;)
			{
				const auto task = nextTask.fetch_add(1);
				if (task >= results.size()) { break; }
				results[task] = currentBatch + task + 1;
				++completions;
			}
			pool.markWorkFinished(index);
		}
	}
}

int main()
{
	ThreadPool pool;
	pool.waitForEveryoneToFinish(); // empty pool is already complete
	for (int count : {1, 4, 2, 0, 3, 1})
	{
		pool.setThreadsNumber(count, worker);
		REQUIRE(pool.currentCounter == count);
		const auto completedBeforeIdle = completions.load();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		REQUIRE(completions.load() == completedBeforeIdle);
		for (int batch = 0; batch < 100; ++batch)
		{
			++currentBatch;
			results.assign(count == 0 ? 0 : 129, 0);
			nextTask = 0;
			const auto before = completions.load();
			pool.setThrerIsWork(); // may notify before a newly created worker starts waiting
			pool.waitForEveryoneToFinish();
			REQUIRE(completions.load() - before == results.size());
			for (std::size_t i = 0; i < results.size(); ++i)
			{
				REQUIRE(results[i] == currentBatch + i + 1);
			}
		}
	}
	pool.cleanup(); // wakes idle workers and joins without a pending job
	REQUIRE(pool.currentCounter == 0);
	pool.setThreadsNumber(2, worker);
	pool.cleanup(); // restart then shutdown before first work notification
	std::cout << "Thread-pool wake, completion, resize and shutdown contracts passed.\n";
}
