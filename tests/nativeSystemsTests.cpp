#include <native/contentRegistry.h>
#include <native/gameplayFoundation.h>
#include <native/gameplayScheduler.h>
#include <native/prototypeMachine.h>
#include <native/worldSchema.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
	int failures = 0;

	void check(bool condition, const char *message)
	{
		if (!condition)
		{
			std::cerr << "FAIL: " << message << '\n';
			++failures;
		}
	}

	void testContentRegistry()
	{
		using namespace mie::native;
		ContentRegistry registry = createV06ContentRegistry();
		check(registry.size(ContentKind::Block) == V05_BLOCK_COUNT,
			"all v0.5 block IDs are frozen in the registry");
		check(registry.size(ContentKind::Item) ==
			V05_LAST_ITEM_EXCLUSIVE - V05_FIRST_ITEM_ID,
			"all v0.5 item IDs are frozen in the registry");
		check(registry.size(ContentKind::Entity) == V05_ENTITY_TYPE_COUNT,
			"all v0.5 entity IDs are frozen in the registry");
		check(registry.resolve(ContentKind::Block, "mie:block/v0.5/211") == 211,
			"stable block key resolves to its frozen runtime ID");
		check(registry.stableKey(ContentKind::Item, 2048) == "mie:item/v0.5/2048",
			"runtime item ID resolves back to its stable key");
		bool fallback = false;
		check(registry.resolveOrFallback(ContentKind::Block, "mie:block/missing", 0,
			&fallback) == 0 && fallback, "missing content uses the explicit fallback");

		std::string error;
		check(!registry.registerContent({ContentKind::Block, 211, "mie:block/duplicate", false},
			&error), "duplicate runtime IDs are rejected");
		check(!registry.registerContent({ContentKind::Block, 4000, "mie:block/overflow", false},
			&error), "persisted block ID overflow is rejected");
	}

	void testSchemaAndMigrations()
	{
		using namespace mie::native;
		const WorldSchemaManifest expected = makeV06WorldSchemaManifest();
		const std::vector<unsigned char> bytes = formatWorldSchemaManifest(expected);
		WorldSchemaManifest parsed;
		check(!bytes.empty() && parseWorldSchemaManifest(
			reinterpret_cast<const char *>(bytes.data()), bytes.size(), parsed),
			"v0.6 schema manifest round-trips");
		check(parsed.schemas == expected.schemas, "all subsystem schema versions round-trip");
		check(!parseWorldSchemaManifest(reinterpret_cast<const char *>(bytes.data()),
			bytes.size() - 1, parsed), "truncated schema manifests are rejected");

		WorldSchemaManifest duplicate = expected;
		duplicate.schemas.push_back(duplicate.schemas.front());
		check(formatWorldSchemaManifest(duplicate).empty(),
			"duplicate subsystem declarations are rejected");

		MigrationRegistry migrations;
		check(migrations.registerMigration("mie:test", 1,
			[](std::vector<unsigned char> &payload)
			{
				payload.push_back(2);
				return true;
			}), "first sequential migration registers");
		check(migrations.registerMigration("mie:test", 2,
			[](std::vector<unsigned char> &payload)
			{
				payload.push_back(3);
				return true;
			}), "second sequential migration registers");
		std::uint32_t version = 1;
		std::vector<unsigned char> payload = {1};
		check(migrations.migrate("mie:test", version, 3, payload) && version == 3 &&
			payload == std::vector<unsigned char>({1, 2, 3}),
			"sequential migrations commit atomically");

		MigrationRegistry failing;
		failing.registerMigration("mie:test", 1, [](std::vector<unsigned char> &candidate)
		{
			candidate.push_back(2);
			return true;
		});
		failing.registerMigration("mie:test", 2, [](std::vector<unsigned char> &candidate)
		{
			candidate.push_back(3);
			return false;
		});
		version = 1;
		payload = {1};
		check(!failing.migrate("mie:test", version, 3, payload) && version == 1 &&
			payload == std::vector<unsigned char>({1}),
			"failed migration rolls back version and payload");
	}

	void testScheduler()
	{
		using namespace mie::native;
		check(simulationIntervalMultiplier(SimulationLevel::Full) == 1 &&
			simulationIntervalMultiplier(SimulationLevel::Reduced) == 4 &&
			simulationIntervalMultiplier(SimulationLevel::Dormant) == 20,
			"simulation levels have deterministic cadence multipliers");

		GameplayScheduler scheduler;
		ScheduledGameplayJob normal;
		normal.nextTick = 10;
		normal.estimatedCost = 1;
		const std::uint64_t normalId = scheduler.schedule(normal);
		ScheduledGameplayJob critical;
		critical.category = GameplayJobCategory::Combat;
		critical.priority = GameplayJobPriority::Critical;
		critical.nextTick = 10;
		critical.estimatedCost = 2;
		const std::uint64_t criticalId = scheduler.schedule(critical);
		ScheduledGameplayJob unloaded;
		unloaded.nextTick = 10;
		unloaded.simulationLevel = SimulationLevel::Unloaded;
		const std::uint64_t unloadedId = scheduler.schedule(unloaded);

		const SchedulerRunResult result = scheduler.run(10, 1);
		check(result.executed.size() == 1 && result.executed.front() == criticalId,
			"critical combat work may exceed the soft budget and runs first");
		check(result.deferred.size() == 1 && result.deferred.front() == normalId,
			"noncritical work is deferred when the budget is exhausted");
		check(scheduler.find(unloadedId) && scheduler.find(unloadedId)->nextTick == 70,
			"unloaded work is discarded and rescheduled at unloaded cadence");
		check(scheduler.metrics().jobsExecuted == 1 &&
			scheduler.metrics().jobsDeferred == 1 && scheduler.metrics().jobsDiscarded == 1,
			"scheduler exports execution, deferral and discard metrics");
	}

	void testDirtyEventsAndInterest()
	{
		using namespace mie::native;
		DirtyFlag flags = DirtyFlag::Persistence | DirtyFlag::Network;
		check(hasFlag(flags, DirtyFlag::Persistence), "dirty flag composition works");
		flags = without(flags, DirtyFlag::Persistence);
		check(!hasFlag(flags, DirtyFlag::Persistence) && hasFlag(flags, DirtyFlag::Network),
			"individual dirty flags can be acknowledged");

		GameplayEventStream events(2);
		events.publish("mie:test/one", 1, 1);
		events.publish("mie:test/two", 2, 2);
		events.publish("mie:test/three", 3, 3);
		check(events.events().size() == 2 && events.events().front().sequence == 2 &&
			events.droppedEvents() == 1, "bounded event stream drops only its oldest event");

		ObserverContext observer;
		observer.chunkX = -2;
		observer.chunkZ = 5;
		observer.chunkRadius = 2;
		observer.playerId = 9;
		check(isRelevantToObserver({InterestScope::Chunk, 0, 5}, observer),
			"chunk interest supports negative coordinates and radius edges");
		check(!isRelevantToObserver({InterestScope::Chunk, 1, 5}, observer),
			"chunk interest rejects positions outside the radius");
		InterestDescriptor owned;
		owned.scope = InterestScope::Player;
		owned.ownerId = 9;
		check(isRelevantToObserver(owned, observer), "player-scoped state reaches only its owner");
	}

	void testPrototypeMachine()
	{
		using namespace mie::native;
		PrototypeMachineRuntime machines;
		check(machines.createMachine(10, {0, 64, 0}), "prototype machine is created");
		check(machines.insertInput(10, 2, 0), "machine accepts bounded input");
		for (std::uint64_t tick = 1; tick <= 100; ++tick) { machines.update(tick, 8); }
		const PrototypeMachineState *machine = machines.find(10);
		check(machine && machine->inputUnits == 1 && machine->outputUnits == 1 &&
			machine->status == PrototypeMachineStatus::Processing,
			"authoritative machine completes one deterministic process in 100 ticks");

		ObserverContext nearObserver;
		nearObserver.chunkRadius = 1;
		check(machines.collectDeltas(nearObserver).size() == 1,
			"machine delta reaches observers interested in its chunk");
		ObserverContext farObserver;
		farObserver.chunkX = 20;
		farObserver.chunkZ = 20;
		farObserver.chunkRadius = 1;
		check(machines.collectDeltas(farObserver).empty(),
			"machine delta is filtered outside observer interest");

		const std::vector<unsigned char> snapshot = machines.formatSnapshot();
		PrototypeMachineRuntime restored;
		check(restored.restoreSnapshot(reinterpret_cast<const char *>(snapshot.data()),
			snapshot.size()), "machine snapshot round-trips");
		check(restored.find(10) && restored.find(10)->inputUnits == 1 &&
			restored.find(10)->processTicksRemaining == PROTOTYPE_MACHINE_PROCESS_TICKS,
			"machine process state survives restore");
		const std::size_t restoredCount = restored.allMachines().size();
		check(!restored.restoreSnapshot(reinterpret_cast<const char *>(snapshot.data()),
			snapshot.size() - 1) && restored.allMachines().size() == restoredCount,
			"truncated machine snapshot is rejected without mutating live state");

		restored.acknowledgePersisted();
		check(!restored.hasPersistenceChanges(), "persisted machine state can be acknowledged");
		check(restored.removeMachine(10) && restored.hasPersistenceChanges(),
			"removing the final machine still dirties persistence");

		PrototypeMachineRuntime stress;
		for (std::uint64_t id = 1; id <= 400; ++id)
		{
			check(stress.createMachine(id, {static_cast<int>(id), 64, 0}),
				"stress machine creation succeeds");
		}
		stress.acknowledgePersisted();
		for (std::uint64_t tick = 1; tick <= 200; ++tick) { stress.update(tick, 64); }
		check(stress.metrics().scheduler.jobsExecuted == 0 &&
			stress.metrics().activeMachines == 0,
			"hundreds of idle machines generate no scheduled gameplay work");
	}
}

int main()
{
	testContentRegistry();
	testSchemaAndMigrations();
	testScheduler();
	testDirtyEventsAndInterest();
	testPrototypeMachine();
	if (failures != 0)
	{
		std::cerr << failures << " native system test(s) failed\n";
		return 1;
	}
	std::cout << "native systems tests passed\n";
	return 0;
}
