#pragma once
#include <gameplay/weaponStats.h>
#include <random>
#include <algorithm>
#include <limits>

struct Life
{
	Life() {}
	Life(int l) { life = l; maxLife = l; }

	short life = 0;
	short maxLife = 0;

	void sanitize()
	{
		maxLife = std::max<short>(maxLife, 1);
		life = std::clamp<short>(life, 0, maxLife);
	}
};

// Server-authoritative survival resource. 100 points map cleanly to 10 HUD pips.
struct SurvivalStats
{
	short hunger = 100;
	short maxHunger = 100;

	void sanitize()
	{
		maxHunger = std::max<short>(maxHunger, 1);
		hunger = std::clamp<short>(hunger, 0, maxHunger);
	}

	void addHunger(int amount)
	{
		const long long value = static_cast<long long>(hunger) + static_cast<long long>(amount);
		const long long bounded = std::clamp(value, 0LL, static_cast<long long>(maxHunger));
		hunger = static_cast<short>(bounded);
	}
};


struct Armour
{
	int armour = 0;

	void normalize() { armour = std::max(armour, 0); }
};


int calculateDamage(Armour armour, const WeaponStats &weaponStats, std::minstd_rand &rng
	, float hitCorectness, float critChanceBonus, bool unaware);
