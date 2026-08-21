#include <gameplay/items.h>
#include <gameplay/weaponStats.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)

	void requireFiniteDerivedValues(WeaponStats stats)
	{
		stats.normalize();
		const auto cooldown = stats.getTimerCulldownRangeForAttacks();
		REQUIRE(std::isfinite(stats.getUIMoveSpeed()));
		REQUIRE(std::isfinite(stats.getSpeedNormalizedInSecconds()));
		REQUIRE(std::isfinite(stats.getDrawSpeedNormalizedInSecconds()));
		REQUIRE(std::isfinite(stats.getKnockBackNormalized()));
		REQUIRE(std::isfinite(stats.getAccuracyAdjusted()));
		REQUIRE(std::isfinite(stats.getAccuracyNormalized()));
		REQUIRE(std::isfinite(stats.getAccuracyNormalizedNegative()));
		REQUIRE(std::isfinite(cooldown.x));
		REQUIRE(std::isfinite(cooldown.y));
		REQUIRE(cooldown.x > 0.f);
		REQUIRE(cooldown.y >= cooldown.x);
	}
}

int main()
{
	WeaponStats slow;
	slow.speed = -4.f;
	slow.drawSpeed = -6.f;
	slow.normalize();
	REQUIRE(std::isfinite(slow.getUIMoveSpeed()));
	REQUIRE(slow.getUIMoveSpeed() > 0.f);

	WeaponStats minimumSpeed;
	minimumSpeed.speed = -10.f;
	minimumSpeed.normalize();
	REQUIRE(std::abs(minimumSpeed.getUIMoveSpeed() - 0.6f) < 0.0001f);

	WeaponStats maximumSpeed;
	maximumSpeed.speed = 20.f;
	maximumSpeed.normalize();
	REQUIRE(std::abs(maximumSpeed.getUIMoveSpeed() - 1.3f) < 0.0001f);

	WeaponStats corrupted;
	const float nan = std::numeric_limits<float>::quiet_NaN();
	corrupted.damage = nan;
	corrupted.critChance = nan;
	corrupted.critDamage = nan;
	corrupted.surprizeDamage = nan;
	corrupted.speed = nan;
	corrupted.drawSpeed = nan;
	corrupted.armourPenetration = nan;
	corrupted.accuracy = nan;
	corrupted.range = nan;
	corrupted.knockBack = nan;
	requireFiniteDerivedValues(corrupted);

	// Hammer presets intentionally use negative attack/draw speed. This was the
	// production path that exposed sqrt(speed) and produced NaN before v0.9.5.
	for (const unsigned short hammerType : {
		ItemTypes::trainingWarHammer, ItemTypes::copperWarHammer,
		ItemTypes::leadWarHammer, ItemTypes::ironWarHammer,
		ItemTypes::silverWarHammer, ItemTypes::goldWarHammer})
	{
		Item hammer;
		hammer.type = hammerType;
		requireFiniteDerivedValues(hammer.getWeaponStats());
	}

	std::cout << "Weapon stats regression tests passed.\n";
	return 0;
}
