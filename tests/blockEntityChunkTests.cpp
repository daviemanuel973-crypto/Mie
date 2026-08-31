#include <gameplay/blockEntityChunk.h>

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
	using mie::clientEntities::blockPositionBelongsToChunk;

	REQUIRE(blockPositionBelongsToChunk({0, 0, 0}, {0, 0}));
	REQUIRE(blockPositionBelongsToChunk({15, 255, 15}, {0, 0}));
	REQUIRE(blockPositionBelongsToChunk({16, 0, 16}, {1, 1}));
	REQUIRE(!blockPositionBelongsToChunk({16, 0, 15}, {0, 0}));

	REQUIRE(blockPositionBelongsToChunk({-1, 0, -1}, {-1, -1}));
	REQUIRE(blockPositionBelongsToChunk({-16, 255, -16}, {-1, -1}));
	REQUIRE(blockPositionBelongsToChunk({-17, 0, -17}, {-2, -2}));
	REQUIRE(!blockPositionBelongsToChunk({-17, 0, -16}, {-1, -1}));

	std::cout << "Block entity chunk tests passed.\n";
	return 0;
}
