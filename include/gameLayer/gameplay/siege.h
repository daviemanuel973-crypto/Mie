#pragma once

#include <algorithm>
#include <cstdint>

enum class SiegePhase : std::uint8_t
{
	Peace = 0,
	Warning,
	Wave,
	Intermission,
};

struct SiegeStatus
{
	SiegePhase phase = SiegePhase::Peace;
	std::uint8_t currentWave = 0;
	std::uint8_t totalWaves = 3;
	std::uint16_t enemiesRemaining = 0;
	std::uint16_t completedSieges = 0;
	std::uint16_t secondsRemaining = 0;
	std::uint8_t cyclesUntilSiege = 7;

	bool operator==(const SiegeStatus &other) const
	{
		return phase == other.phase && currentWave == other.currentWave &&
			totalWaves == other.totalWaves && enemiesRemaining == other.enemiesRemaining &&
			completedSieges == other.completedSieges && secondsRemaining == other.secondsRemaining &&
			cyclesUntilSiege == other.cyclesUntilSiege;
	}

	bool operator!=(const SiegeStatus &other) const { return !(*this == other); }
};

struct SiegeTuning
{
	float warningSeconds = 45.f;
	float intermissionSeconds = 12.f;
	std::uint8_t totalWaves = 3;
	std::uint8_t cyclesPerSiege = 7;
};

class SiegeDirector
{
public:
	explicit SiegeDirector(SiegeTuning tuning = {});

	void reset();
	void restoreSchedule(std::uint64_t currentWorldDay,
		std::uint64_t nextSiegeCycle, unsigned int restoredCompletedSieges);
	void update(float deltaTime, unsigned int survivalPlayers, unsigned int activeSiegeEnemies,
		std::uint64_t currentWorldDay, bool scheduledNight);
	unsigned int takeSpawnRequest(unsigned int maxCount);
	void returnSpawnRequest(unsigned int count);
	void forceWarning();
	void cancelCurrentSiege();

	SiegeStatus getStatus(unsigned int activeSiegeEnemies) const;
	unsigned int getPendingSpawns() const { return pendingSpawns; }
	unsigned int getCompletedSieges() const { return completedSieges; }
	std::uint64_t getNextSiegeCycle() const { return nextSiegeCycle; }

private:
	unsigned int waveEnemyCount(unsigned int wave, unsigned int survivalPlayers) const;
	void beginWave(unsigned int survivalPlayers);

	SiegeTuning tuning;
	SiegePhase phase = SiegePhase::Peace;
	float timer = 0.f;
	unsigned int currentWave = 0;
	unsigned int pendingSpawns = 0;
	unsigned int lastPlayerCount = 1;
	unsigned int completedSieges = 0;
	std::uint64_t observedWorldCycles = 0;
	std::uint64_t nextSiegeCycle = 7;
};

const char *getSiegePhaseName(SiegePhase phase);

void setClientSiegeStatus(const SiegeStatus &status);
const SiegeStatus &getClientSiegeStatus();
void resetClientSiegeStatus();
