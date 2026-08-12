#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr double DEFAULT_WORLD_CYCLE_SECONDS = 20.0 * 60.0;
constexpr std::uint32_t WORLD_PROGRESS_FORMAT_VERSION = 1;

struct WorldProgressSnapshot
{
	double cycleProgressSeconds = 0.0;
	std::uint64_t completedCycles = 0;
	std::uint64_t nextSiegeCycle = 7;
	std::uint32_t completedSieges = 0;
};

class WorldCycleClock
{
public:
	explicit WorldCycleClock(double cycleDurationSeconds = DEFAULT_WORLD_CYCLE_SECONDS);

	void reset();
	void restore(double cycleProgressSeconds, std::uint64_t completedCycles);
	std::uint64_t update(double deltaTime, bool advancing);

	float getDayPhase() const;
	double getCycleProgressSeconds() const { return cycleProgressSeconds; }
	std::uint64_t getCompletedCycles() const { return completedCycles; }
	double getCycleDurationSeconds() const { return cycleDurationSeconds; }

private:
	double cycleDurationSeconds = DEFAULT_WORLD_CYCLE_SECONDS;
	double cycleProgressSeconds = 0.0;
	std::uint64_t completedCycles = 0;
};

std::vector<unsigned char> formatWorldProgressSnapshot(const WorldProgressSnapshot &snapshot);
bool parseWorldProgressSnapshot(const char *data, std::size_t size,
	WorldProgressSnapshot &snapshot);

void setClientWorldTime(float dayPhase, std::uint64_t completedCycles, bool advancing);
void updateClientWorldTime(float deltaTime);
float getClientWorldDayPhase();
std::uint64_t getClientCompletedWorldCycles();
void resetClientWorldTime();
