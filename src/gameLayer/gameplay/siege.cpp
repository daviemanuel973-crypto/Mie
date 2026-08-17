#include <gameplay/siege.h>

#include <cmath>
#include <limits>

namespace
{
	SiegeStatus clientStatus;
}

SiegeDirector::SiegeDirector(SiegeTuning tuning): tuning(tuning)
{
	reset();
}

void SiegeDirector::reset()
{
	phase = SiegePhase::Peace;
	timer = 0.f;
	currentWave = 0;
	pendingSpawns = 0;
	lastPlayerCount = 1;
	completedSieges = 0;
	observedWorldCycles = 0;
	nextSiegeCycle = std::max<std::uint64_t>(tuning.cyclesPerSiege, 1u);
}

void SiegeDirector::restoreSchedule(std::uint64_t currentWorldDay,
	std::uint64_t restoredNextSiegeCycle, unsigned int restoredCompletedSieges)
{
	observedWorldCycles = currentWorldDay;
	nextSiegeCycle = std::max<std::uint64_t>(restoredNextSiegeCycle, 1u);
	completedSieges = restoredCompletedSieges;
}

unsigned int SiegeDirector::waveEnemyCount(unsigned int wave, unsigned int survivalPlayers) const
{
	const unsigned int players = std::clamp(survivalPlayers, 1u, 8u);
	const unsigned int safeWave = std::clamp(wave, 1u, static_cast<unsigned int>(tuning.totalWaves));
	const unsigned int baseCount = 2u + safeWave * 2u;
	const unsigned int perPlayer = 1u + safeWave;
	return std::min(baseCount + players * perPlayer, 32u);
}

void SiegeDirector::beginWave(unsigned int survivalPlayers)
{
	phase = SiegePhase::Wave;
	currentWave = std::min(currentWave + 1u, static_cast<unsigned int>(tuning.totalWaves));
	pendingSpawns = waveEnemyCount(currentWave, survivalPlayers);
	timer = 0.f;
}

void SiegeDirector::update(float deltaTime, unsigned int survivalPlayers,
	unsigned int activeSiegeEnemies, std::uint64_t currentWorldDay, bool scheduledNight)
{
	if (deltaTime <= 0.f) { return; }
	observedWorldCycles = currentWorldDay;
	if (survivalPlayers == 0)
	{
		lastPlayerCount = 1;
		return;
	}

	lastPlayerCount = survivalPlayers;

	switch (phase)
	{
	case SiegePhase::Peace:
		if (scheduledNight && currentWorldDay >= nextSiegeCycle)
		{
			const std::uint64_t interval = std::max<std::uint64_t>(tuning.cyclesPerSiege, 1u);
			do
			{
				const auto maximumCycle = std::numeric_limits<std::uint64_t>::max();
				nextSiegeCycle = nextSiegeCycle > maximumCycle - interval ?
					maximumCycle : nextSiegeCycle + interval;
			}
			while (nextSiegeCycle <= currentWorldDay &&
				nextSiegeCycle != std::numeric_limits<std::uint64_t>::max());
			phase = SiegePhase::Warning;
			timer = std::max(tuning.warningSeconds, 0.f);
			currentWave = 0;
		}
		break;

	case SiegePhase::Warning:
		timer -= deltaTime;
		if (timer <= 0.f) { beginWave(survivalPlayers); }
		break;

	case SiegePhase::Wave:
		if (pendingSpawns == 0 && activeSiegeEnemies == 0)
		{
			if (currentWave >= tuning.totalWaves)
			{
				completedSieges++;
				phase = SiegePhase::Peace;
				timer = 0.f;
				currentWave = 0;
			}
			else
			{
				phase = SiegePhase::Intermission;
				timer = std::max(tuning.intermissionSeconds, 0.f);
			}
		}
		break;

	case SiegePhase::Intermission:
		timer -= deltaTime;
		if (timer <= 0.f) { beginWave(survivalPlayers); }
		break;
	}
}

unsigned int SiegeDirector::takeSpawnRequest(unsigned int maxCount)
{
	const unsigned int count = std::min(maxCount, pendingSpawns);
	pendingSpawns -= count;
	return count;
}

void SiegeDirector::returnSpawnRequest(unsigned int count)
{
	if (phase != SiegePhase::Wave) { return; }
	pendingSpawns = std::min(pendingSpawns + count, 32u);
}

void SiegeDirector::forceWarning()
{
	if (phase == SiegePhase::Wave || phase == SiegePhase::Intermission) { return; }
	phase = SiegePhase::Warning;
	timer = std::max(tuning.warningSeconds, 0.f);
	currentWave = 0;
	pendingSpawns = 0;
}

void SiegeDirector::cancelCurrentSiege()
{
	phase = SiegePhase::Peace;
	timer = 0.f;
	currentWave = 0;
	pendingSpawns = 0;
}

SiegeStatus SiegeDirector::getStatus(unsigned int activeSiegeEnemies) const
{
	SiegeStatus result;
	result.phase = phase;
	result.currentWave = static_cast<std::uint8_t>(currentWave);
	result.totalWaves = tuning.totalWaves;
	result.enemiesRemaining = static_cast<std::uint16_t>(
		std::min(activeSiegeEnemies + pendingSpawns, 65535u));
	result.completedSieges = static_cast<std::uint16_t>(std::min(completedSieges, 65535u));
	result.secondsRemaining = static_cast<std::uint16_t>(
		std::clamp(static_cast<unsigned int>(std::ceil(std::max(timer, 0.f))), 0u, 65535u));
	const std::uint64_t remainingCycles = nextSiegeCycle > observedWorldCycles ?
		nextSiegeCycle - observedWorldCycles : 0;
	result.cyclesUntilSiege = static_cast<std::uint8_t>(std::min<std::uint64_t>(remainingCycles, 255));
	return result;
}

const char *getSiegePhaseName(SiegePhase phase)
{
	switch (phase)
	{
	case SiegePhase::Peace: return "Peace";
	case SiegePhase::Warning: return "Warning";
	case SiegePhase::Wave: return "Wave";
	case SiegePhase::Intermission: return "Intermission";
	}
	return "Unknown";
}

void setClientSiegeStatus(const SiegeStatus &status)
{
	clientStatus = status;
}

const SiegeStatus &getClientSiegeStatus()
{
	return clientStatus;
}

void resetClientSiegeStatus()
{
	clientStatus = {};
}
