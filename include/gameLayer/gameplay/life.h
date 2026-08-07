#pragma once
#include <gameplay/weaponStats.h>
#include <random>
#include <algorithm>

struct Life
{
	Life() {};
	Life(int l) { life = l; maxLife = l; }

	short life = 0;
	short maxLife = 0;

	void sanitize() { if (life > maxLife) { life = maxLife; } if (life < 0) { life = 0; } }
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
		int value = hunger + amount;
		hunger = static_cast<short>(std::clamp(value, 0, static_cast<int>(maxHunger)));
	}
};


struct Armour
{
	int armour = 0;

	void normalize() { armour = std::max(armour, 0); }
};


int calculateDamage(Armour armour, const WeaponStats &weaponStats, std::minstd_rand &rng
	, float hitCorectness, float critChanceBonus, bool unaware);

