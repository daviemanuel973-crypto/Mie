#include <gameplay/entityStats.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	short saturatingAddShort(short left, short right)
	{
		const int sum = static_cast<int>(left) + static_cast<int>(right);
		return static_cast<short>(std::clamp(sum,
			static_cast<int>(std::numeric_limits<short>::min()),
			static_cast<int>(std::numeric_limits<short>::max())));
	}
}

std::string EntityStats::formatDataToString()
{
	std::string rez = "";

	// Defence
	if (armour)
		rez += "\nArmour: " + std::to_string(armour);

	if (knockBackResistance)
		rez += "\nKnockback Resistance: " + std::to_string(knockBackResistance) + "%";

	if (thorns)
		rez += "\nThorns: " + std::to_string(thorns) + "%";

	// Attack
	if (meleDamage)
		rez += "\nMelee Damage: " + std::to_string(meleDamage) + "%";

	if (meleAttackSpeed)
		rez += "\nMelee Attack Speed: " + std::to_string(meleAttackSpeed) + "%";

	if (critChance)
		rez += "\nCritical Strike Chance: " + std::to_string(critChance) + "%";

	// Player
	if (runningSpeed)
		rez += "\nRunning Speed: " + std::to_string(runningSpeed) + "%";

	// Special
	if (stealthSound)
		rez += "\nStealth Sound: " + std::to_string(stealthSound) + "%";

	if (stealthVisibility)
		rez += "\nStealth Visibility: " + std::to_string(stealthVisibility) + "%";

	// Other
	if (luck)
		rez += "\nLuck: " + std::to_string(luck) + "%";

	if (improvedMiningPower)
		rez += "\nMining Power: " + std::to_string(improvedMiningPower) + "%";

	return rez;

}

void EntityStats::add(EntityStats &other)
{
	runningSpeed += other.runningSpeed;
	armour = saturatingAddShort(armour, other.armour);
	knockBackResistance = saturatingAddShort(knockBackResistance, other.knockBackResistance);
	thorns = saturatingAddShort(thorns, other.thorns);
	meleDamage = saturatingAddShort(meleDamage, other.meleDamage);
	meleAttackSpeed = saturatingAddShort(meleAttackSpeed, other.meleAttackSpeed);
	critChance = saturatingAddShort(critChance, other.critChance);
	stealthSound = saturatingAddShort(stealthSound, other.stealthSound);
	stealthVisibility = saturatingAddShort(stealthVisibility, other.stealthVisibility);
	luck = saturatingAddShort(luck, other.luck);
	improvedMiningPower = saturatingAddShort(improvedMiningPower, other.improvedMiningPower);
}

void EntityStats::normalize()
{
	// runningSpeed is a percentage modifier just like the other offensive/player
	// stats. The previous 0..0 clamp silently erased every movement-speed bonus.
	// Keep the same bounded modifier contract used by the neighboring stats.
	if (!std::isfinite(runningSpeed)) { runningSpeed = 0.f; }
	runningSpeed = std::clamp(runningSpeed, -300.f, 300.f);
	armour = std::clamp(armour, (short)0, (short)300);
	knockBackResistance = std::clamp(knockBackResistance, (short)-300, (short)100);
	thorns = std::clamp(thorns, (short)0, (short)300);
	meleDamage = std::clamp(meleDamage, (short)-300, (short)300);
	meleAttackSpeed = std::clamp(meleAttackSpeed, (short)-300, (short)300);
	critChance = std::clamp(critChance, (short)-300, (short)300);
	stealthSound = std::clamp(stealthSound, (short)-300, (short)300);
	stealthVisibility = std::clamp(stealthVisibility, (short)-300, (short)300);
	luck = std::clamp(luck, (short)-100, (short)100);
	improvedMiningPower = std::clamp(improvedMiningPower, (short)-300, (short)300);
}
