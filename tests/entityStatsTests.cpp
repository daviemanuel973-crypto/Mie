#include <gameplay/entityStats.h>

#include <cmath>
#include <iostream>
#include <limits>

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

	stats.runningSpeed = std::numeric_limits<float>::quiet_NaN();
	stats.normalize();
	REQUIRE(stats.runningSpeed == 0.f);

	EntityStats bonus;
	bonus.runningSpeed = 25.f;
	stats.runningSpeed = 10.f;
	stats.add(bonus);
	stats.normalize();
	REQUIRE(std::abs(stats.runningSpeed - 35.f) < 0.001f);

	// v0.9.5: accumulation happens in a wider integer and saturates before
	// converting back to short, so large equipment/stat combinations cannot wrap.
	EntityStats positiveOverflow;
	positiveOverflow.armour = std::numeric_limits<short>::max();
	positiveOverflow.meleDamage = std::numeric_limits<short>::max();
	EntityStats positiveBonus;
	positiveBonus.armour = 100;
	positiveBonus.meleDamage = 100;
	positiveOverflow.add(positiveBonus);
	REQUIRE(positiveOverflow.armour == std::numeric_limits<short>::max());
	REQUIRE(positiveOverflow.meleDamage == std::numeric_limits<short>::max());
	positiveOverflow.normalize();
	REQUIRE(positiveOverflow.armour == 300);
	REQUIRE(positiveOverflow.meleDamage == 300);

	EntityStats negativeOverflow;
	negativeOverflow.knockBackResistance = std::numeric_limits<short>::min();
	negativeOverflow.luck = std::numeric_limits<short>::min();
	EntityStats negativeBonus;
	negativeBonus.knockBackResistance = -100;
	negativeBonus.luck = -100;
	negativeOverflow.add(negativeBonus);
	REQUIRE(negativeOverflow.knockBackResistance == std::numeric_limits<short>::min());
	REQUIRE(negativeOverflow.luck == std::numeric_limits<short>::min());
	negativeOverflow.normalize();
	REQUIRE(negativeOverflow.knockBackResistance == -300);
	REQUIRE(negativeOverflow.luck == -100);

	std::cout << "Entity stats tests passed.\n";
	return 0;
}
