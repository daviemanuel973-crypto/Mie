#include <multyPlayer/chunkStreamingBudget.h>

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
	using mie::network::canQueueAnotherChunkPacket;
	REQUIRE(canQueueAnotherChunkPacket(0));
	REQUIRE(canQueueAnotherChunkPacket(4));
	REQUIRE(!canQueueAnotherChunkPacket(5));
	REQUIRE(!canQueueAnotherChunkPacket(6));
	std::cout << "Chunk streaming budget tests passed.\n";
	return 0;
}
