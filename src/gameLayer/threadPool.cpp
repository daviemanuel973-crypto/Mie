#include <threadPool.h>

void ThreadPool::setThreadsNumber(int nr, void(*worker)(int, ThreadPool &))
{
	if (nr < 0) { nr = 0; }
	if (nr >= MAX_THREADS)
	{
		nr = MAX_THREADS - 1;
	}

	if (nr == currentCounter) { return; }

	if (nr > currentCounter)
	{
		for (int i = currentCounter; i < nr; i++)
		{
			running[i] = true;
			threIsWork[i] = false;
		}

		for (int i = currentCounter; i < nr; i++)
		{
			threads[i] = std::thread(worker, i, std::ref(*this));
		}
	}
	else
	{
		{
			std::lock_guard<std::mutex> lock(workMutex);
			for (int i = nr; i < currentCounter; i++)
			{
				running[i] = false;
				threIsWork[i] = false;
			}
		}
		workAvailable.notify_all();
		workFinished.notify_all();

		for (int i = nr; i < currentCounter; i++)
		{
			if (threads[i].joinable()) { threads[i].join(); }
		}
	}

	currentCounter = nr;
}

void ThreadPool::setThrerIsWork()
{
	{
		std::lock_guard<std::mutex> lock(workMutex);
		for (int i = 0; i < currentCounter; i++)
		{
			threIsWork[i] = true;
		}
	}
	workAvailable.notify_all();
}

bool ThreadPool::waitForWork(int index)
{
	std::unique_lock<std::mutex> lock(workMutex);
	workAvailable.wait(lock, [&]()
	{
		return !running[index].load() || threIsWork[index].load();
	});
	return running[index].load() && threIsWork[index].load();
}

void ThreadPool::markWorkFinished(int index)
{
	{
		std::lock_guard<std::mutex> lock(workMutex);
		threIsWork[index] = false;
	}
	workFinished.notify_all();
}

void ThreadPool::waitForEveryoneToFinish()
{
	std::unique_lock<std::mutex> lock(workMutex);
	workFinished.wait(lock, [&]()
	{
		for (int i = 0; i < currentCounter; ++i)
		{
			if (threIsWork[i].load()) { return false; }
		}
		return true;
	});
}

void ThreadPool::cleanup()
{
	{
		std::lock_guard<std::mutex> lock(workMutex);
		for (int i = 0; i < currentCounter; i++)
		{
			threIsWork[i] = false;
		}
	}
	workFinished.notify_all();
	setThreadsNumber(0, nullptr);
	taskTaken.clear();
}
