#pragma once
#include <worldGenerator.h>
#include <multyPlayer/chunkSaver.h>
#include <multyPlayer/serverChunkStorer.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

struct Client;

#include <threadPool.h>


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




