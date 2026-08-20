#include <multyPlayer/entityIdAllocator.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

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

	std::filesystem::remove_all(root, error);
	std::cout << "Entity ID allocator tests passed.\n";
	return 0;
}
