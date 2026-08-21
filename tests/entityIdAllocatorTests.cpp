#include <multyPlayer/entityIdAllocator.h>

#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	const auto root = std::filesystem::temp_directory_path() / "mie_entity_id_allocator_tests";
	std::error_code error;
	std::filesystem::remove_all(root, error);
	std::filesystem::create_directories(root, error);
	REQUIRE(!error);

	constexpr unsigned int type = 4;
	PersistentEntityIdAllocator first;
	const auto firstId = first.allocate(root.string(), type);
	const auto secondId = first.allocate(root.string(), type);
	REQUIRE(firstId >= PersistentEntityIdAllocator::LegacyMigrationFloor);
	REQUIRE(secondId == firstId + 1);

	PersistentEntityIdAllocator second;
	const auto afterRestart = second.allocate(root.string(), type);
	REQUIRE(afterRestart > secondId);

	const std::uint64_t observedRaw = afterRestart + 5000;
	second.observe((std::uint64_t{type} << 56) | observedRaw);
	const auto afterObserved = second.allocate(root.string(), type);
	REQUIRE(afterObserved > observedRaw);

	// SafeSave keeps two checksummed mirrors. Corrupting the primary must still
	// allow a fresh allocator to recover from the backup without reusing IDs.
	{
		std::ofstream corrupt(root / "entityIdHighWater1.bin",
			std::ios::binary | std::ios::trunc);
		corrupt << "broken";
	}

	PersistentEntityIdAllocator recovered;
	const auto recoveredId = recovered.allocate(root.string(), type);
	REQUIRE(recoveredId > afterObserved);

	// Server region workers can spawn entities concurrently. The allocator must
	// serialize range reservation and never hand the same raw ID to two workers.
	constexpr std::size_t threadCount = 4;
	constexpr std::size_t idsPerThread = 512;
	PersistentEntityIdAllocator concurrent;
	std::vector<std::uint64_t> ids(threadCount * idsPerThread);
	std::vector<std::thread> workers;
	workers.reserve(threadCount);
	for (std::size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
	{
		workers.emplace_back([&, threadIndex]
		{
			for (std::size_t idIndex = 0; idIndex < idsPerThread; ++idIndex)
			{
				ids[threadIndex * idsPerThread + idIndex] =
					concurrent.allocate(root.string(), type);
			}
		});
	}
	for (auto &worker : workers) { worker.join(); }
	REQUIRE(std::all_of(ids.begin(), ids.end(), [](std::uint64_t id) { return id != 0; }));
	std::sort(ids.begin(), ids.end());
	REQUIRE(std::adjacent_find(ids.begin(), ids.end()) == ids.end());

	std::filesystem::remove_all(root, error);
	std::cout << "Entity ID allocator tests passed.\n";
	return 0;
}
