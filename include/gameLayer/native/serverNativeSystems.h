#pragma once

#include <cstdint>

#include <native/prototypeMachine.h>
#include <native/worldSchema.h>

struct WorldSaver;

namespace mie::native
{
	struct ServerNativeSystemsMetrics
	{
		PrototypeMachineMetrics machines;
		std::uint64_t accumulatedCpuMicroseconds = 0;
		std::uint64_t peakCpuMicroseconds = 0;
		std::uint64_t manifestFallbacks = 0;
		std::uint64_t saveFailures = 0;
	};

	void resetServerNativeSystems();
	bool loadServerNativeSystems(const WorldSaver &worldSaver);
	bool saveServerNativeSystems(const WorldSaver &worldSaver, bool force = false);
	void updateServerNativeSystems(float deltaTime);
	ServerNativeSystemsMetrics getServerNativeSystemsMetrics();
	PrototypeMachineRuntime &getPrototypeMachineRuntime();
}
