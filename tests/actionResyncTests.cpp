#include <multyPlayer/actionResync.h>

#include <cstdlib>
#include <iostream>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	using mie::network::shouldRequestActionResync;
	REQUIRE(!shouldRequestActionResync(19'999, 0, 0));
	REQUIRE(shouldRequestActionResync(20'000, 0, 0));
	REQUIRE(!shouldRequestActionResync(24'999, 0, 20'000));
	REQUIRE(shouldRequestActionResync(25'000, 0, 20'000));
	REQUIRE(!shouldRequestActionResync(100, 200, 0));

	using mie::network::droppedItemRevisionRequiresInventoryResync;
	REQUIRE(!droppedItemRevisionRequiresInventoryResync(7, 7));
	REQUIRE(droppedItemRevisionRequiresInventoryResync(7, 6));
	REQUIRE(droppedItemRevisionRequiresInventoryResync(0, 255));

	std::cout << "Action resync tests passed.\n";
	return 0;
}
