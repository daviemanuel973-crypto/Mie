#pragma once
#include <worldGenerator.h>
#include <multyPlayer/chunkSaver.h>
#include <multyPlayer/serverChunkStorer.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

struct Client;

struct ThreadPool
{
	ThreadPool() {};

	constexpr static int MAX_THREADS = 12;

	int currentCounter = 0;
	
	std::thread threads[MAX_THREADS] = {};
	std::atomic_bool running[MAX_THREADS] = {};
	std::atomic_bool threIsWork[MAX_THREADS] = {};
	std::deque<std::atomic<bool>> taskTaken;

	// v0.9.2: workers sleep while idle and the owner sleeps while waiting for a batch.
	// This replaces the previous tight spin loops that could consume a full CPU core.
	std::mutex workMutex;
	std::condition_variable workAvailable;
	std::condition_variable workFinished;

	void setThreadsNumber(int nr, void(*worker)(int, ThreadPool&));

	void setThrerIsWork();

	bool waitForWork(int index);
	void markWorkFinished(int index);
	void waitForEveryoneToFinish();

	void cleanup();
};


void closeThreadPool();

int getThredPoolSize();

void splitUpdatesLogic(float tickDeltaTime,
	int tickDeltaTimeMs,
	std::uint64_t currentTimer,
	ServerChunkStorer &chunkCache, unsigned int seed,
	std::unordered_map<std::uint64_t, Client> &clients,
	WorldSaver &worldSaver, std::vector<ServerTask> &waitingTasks,
	Profiler &serverProfiler
);




