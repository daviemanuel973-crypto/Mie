#include <gameplay/siege.h>
#include <gameplay/worldTime.h>

#include <cstdlib>
#include <iostream>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)

	SiegeTuning fastTuning()
	{
		SiegeTuning tuning;
		tuning.warningSeconds = 3.f;
		tuning.intermissionSeconds = 1.f;
		tuning.cyclesPerSiege = 7;
		return tuning;
	}
}

int main()
{
	SiegeDirector director(fastTuning());

	director.update(1.f, 0, 0, 1, false);
	REQUIRE(director.getStatus(0).phase == SiegePhase::Peace);
	REQUIRE(director.getStatus(0).cyclesUntilSiege == 6);

	director.update(1.f, 1, 0, 6, true);
	REQUIRE(director.getStatus(0).phase == SiegePhase::Peace);
	REQUIRE(director.getStatus(0).cyclesUntilSiege == 1);

	director.update(0.1f, 1, 0, 7, false);
	REQUIRE(director.getStatus(0).phase == SiegePhase::Peace);
	director.update(0.1f, 1, 0, 7, true);
	REQUIRE(director.getStatus(0).phase == SiegePhase::Warning);
	REQUIRE(director.getStatus(0).secondsRemaining == 3);
	REQUIRE(director.getStatus(0).cyclesUntilSiege == 7);
	REQUIRE(director.getNextSiegeCycle() == 14);

	director.update(3.f, 1, 0, 7, true);
	auto status = director.getStatus(0);
	REQUIRE(status.phase == SiegePhase::Wave);
	REQUIRE(status.currentWave == 1);
	REQUIRE(status.enemiesRemaining == 6);

	REQUIRE(director.takeSpawnRequest(2) == 2);
	REQUIRE(director.getStatus(2).enemiesRemaining == 6);
	REQUIRE(director.takeSpawnRequest(99) == 4);
	director.update(0.1f, 1, 0, 7, true);
	REQUIRE(director.getStatus(0).phase == SiegePhase::Intermission);

	director.update(1.f, 2, 0, 7, true);
	status = director.getStatus(0);
	REQUIRE(status.phase == SiegePhase::Wave);
	REQUIRE(status.currentWave == 2);
	REQUIRE(status.enemiesRemaining == 12);

	for (int wave = 2; wave <= 3; ++wave)
	{
		director.takeSpawnRequest(99);
		director.update(0.1f, 2, 0, 7, true);
		if (wave < 3) { director.update(1.f, 2, 0, 7, true); }
	}

	status = director.getStatus(0);
	REQUIRE(status.phase == SiegePhase::Peace);
	REQUIRE(status.completedSieges == 1);
	REQUIRE(status.secondsRemaining == 0);
	REQUIRE(status.cyclesUntilSiege == 7);

	SiegeDirector forced(fastTuning());
	forced.forceWarning();
	REQUIRE(forced.getStatus(0).phase == SiegePhase::Warning);
	forced.update(3.f, 8, 0, 1, false);
	REQUIRE(forced.getStatus(0).enemiesRemaining <= 32);
	forced.returnSpawnRequest(4);
	REQUIRE(forced.getPendingSpawns() <= 32);

	SiegeDirector restored(fastTuning());
	restored.restoreSchedule(13, 14, 3);
	restored.update(0.1f, 1, 0, 13, true);
	REQUIRE(restored.getStatus(0).phase == SiegePhase::Peace);
	REQUIRE(restored.getStatus(0).completedSieges == 3);
	restored.update(0.1f, 1, 0, 14, true);
	REQUIRE(restored.getStatus(0).phase == SiegePhase::Warning);
	REQUIRE(restored.getNextSiegeCycle() == 21);

	WorldCycleClock clock(10.0);
	clock.restore(9.0, 6);
	REQUIRE(clock.update(0.5, true) == 0);
	REQUIRE(clock.getCompletedCycles() == 6);
	REQUIRE(clock.getVisibleDayNumber() == 8);
	REQUIRE(!clock.isNight());
	clock.restore(5.0, 6);
	REQUIRE(clock.getDayPhase() == 0.75f);
	REQUIRE(clock.getVisibleDayNumber() == 7);
	REQUIRE(clock.isNight());
	clock.restore(7.5, 6);
	REQUIRE(clock.getDayPhase() == 0.f);
	REQUIRE(clock.getVisibleDayNumber() == 8);
	REQUIRE(!clock.isNight());
	clock.restore(9.5, 6);
	REQUIRE(clock.update(0.5, true) == 1);
	REQUIRE(clock.getCompletedCycles() == 7);
	REQUIRE(clock.getDayPhase() == 0.25f);
	REQUIRE(clock.update(100.0, false) == 0);

	SiegeDirector cancelled(fastTuning());
	cancelled.update(0.1f, 1, 0, 7, true);
	REQUIRE(cancelled.getStatus(0).phase == SiegePhase::Warning);
	cancelled.cancelCurrentSiege();
	REQUIRE(cancelled.getStatus(0).phase == SiegePhase::Peace);
	REQUIRE(cancelled.getPendingSpawns() == 0);

	WorldProgressSnapshot snapshot;
	snapshot.cycleProgressSeconds = 123.5;
	snapshot.completedCycles = 6;
	snapshot.nextSiegeCycle = 7;
	snapshot.completedSieges = 2;
	const auto encoded = formatWorldProgressSnapshot(snapshot);
	WorldProgressSnapshot decoded;
	REQUIRE(parseWorldProgressSnapshot(reinterpret_cast<const char *>(encoded.data()),
		encoded.size(), decoded));
	REQUIRE(decoded.cycleProgressSeconds == snapshot.cycleProgressSeconds);
	REQUIRE(decoded.completedCycles == snapshot.completedCycles);
	REQUIRE(decoded.nextSiegeCycle == snapshot.nextSiegeCycle);
	REQUIRE(decoded.completedSieges == snapshot.completedSieges);
	REQUIRE(!parseWorldProgressSnapshot(reinterpret_cast<const char *>(encoded.data()),
		encoded.size() - 1, decoded));

	std::cout << "Siege director and world cycle tests passed.\n";
	return 0;
}
