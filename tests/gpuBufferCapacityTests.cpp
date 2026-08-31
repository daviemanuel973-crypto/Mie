#include <rendering/gpuBufferCapacity.h>

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	using mie::rendering::growGpuBufferCapacity;
	REQUIRE(growGpuBufferCapacity(0, 0) == 0);
	REQUIRE(growGpuBufferCapacity(0, 1) == 256);
	REQUIRE(growGpuBufferCapacity(256, 128) == 256);
	REQUIRE(growGpuBufferCapacity(256, 257) == 512);
	REQUIRE(growGpuBufferCapacity(512, 1025) == 2048);
	const auto nearLimit = std::numeric_limits<std::size_t>::max() / 2u + 1u;
	REQUIRE(growGpuBufferCapacity(nearLimit, nearLimit + 1u) == nearLimit + 1u);
	std::cout << "GPU buffer capacity tests passed.\n";
	return 0;
}
