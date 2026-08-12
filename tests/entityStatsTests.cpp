#include <gameplay/entityStats.h>

#include <cmath>
#include <iostream>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed: " #condition " at line " << __LINE__ << "\n"; \
		return 1; \
	} } while (false)
}

int main()
{
	EntityStats stats;
	stats.runningSpeed = 35.f;
	stats.normalize();
	REQUIRE(std::abs(stats.runningSpeed - 35.f) < 0.001f);

	stats.runningSpeed = 450.f;
	stats.normalize();
	REQUIRE(std::abs(stats.runningSpeed - 300.f) < 0.001f);

	stats.runningSpeed = -450.f;
	stats.normalize();
	REQUIRE(std::abs(stats.runningSpeed + 300.f) < 0.001f);

	EntityStats bonus;
	bonus.runningSpeed = 25.f;
	stats.runningSpeed = 10.f;
	stats.add(bonus);
	stats.normalize();
	REQUIRE(std::abs(stats.runningSpeed - 35.f) < 0.001f);

	std::cout << "Entity stats tests passed.\n";
	return 0;
}
