#include <gameplay/entityStats.h>
#include <glm/glm.hpp>


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
	armour += other.armour;
	knockBackResistance += other.knockBackResistance;
	thorns += other.thorns;
	meleDamage += other.meleDamage;
	meleAttackSpeed += other.meleAttackSpeed;
	critChance += other.critChance;
	stealthSound += other.stealthSound;
	stealthVisibility += other.stealthVisibility;
	luck += other.luck;
	improvedMiningPower += other.improvedMiningPower;


}

void EntityStats::normalize()
{
	// runningSpeed is a percentage modifier just like the other offensive/player
	// stats. The previous 0..0 clamp silently erased every movement-speed bonus.
	// Keep the same bounded modifier contract used by the neighboring stats.
	runningSpeed = glm::clamp(runningSpeed, -300.f, 300.f);
	armour = glm::clamp(armour, (short)0, (short)300);
	knockBackResistance = glm::clamp(knockBackResistance, (short)-300, (short)100);
	thorns = glm::clamp(thorns, (short)0, (short)300);
	meleDamage = glm::clamp(meleDamage, (short)-300, (short)300);
	meleAttackSpeed = glm::clamp(meleAttackSpeed, (short)-300, (short)300);
	critChance = glm::clamp(critChance, (short)-300, (short)300);
	stealthSound = glm::clamp(stealthSound, (short)-300, (short)300);
	stealthVisibility = glm::clamp(stealthVisibility, (short)-300, (short)300);
	luck = glm::clamp(luck, (short)-100, (short)100);
	improvedMiningPower = glm::clamp(improvedMiningPower, (short)-300, (short)300);


}