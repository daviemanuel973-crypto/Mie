#include <gameplay/weaponStats.h>
#include <splines.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iomanip>
#include <sstream>

void WeaponStats::normalize()
{
	if (!std::isfinite(critChance)) { critChance = 0.1f; }
	if (!std::isfinite(critDamage)) { critDamage = 15.f; }
	if (!std::isfinite(surprizeDamage)) { surprizeDamage = 20.f; }
	if (!std::isfinite(damage)) { damage = 10.f; }
	if (!std::isfinite(accuracy)) { accuracy = 5.f; }
	if (!std::isfinite(range)) { range = 1.8f; }
	if (!std::isfinite(knockBack)) { knockBack = 4.f; }
	if (!std::isfinite(speed)) { speed = 6.f; }
	if (!std::isfinite(drawSpeed)) { drawSpeed = 2.f; }
	if (!std::isfinite(armourPenetration)) { armourPenetration = 1.f; }

	critChance = glm::clamp(critChance, 0.f, 0.7f);
	critDamage = glm::clamp(critDamage, 0.f, 999.f);
	surprizeDamage = glm::clamp(surprizeDamage, 0.f, 999.f);
	damage = glm::clamp(damage, 1.f, 999.f);
	accuracy = glm::clamp(accuracy, -10.f, 20.f);
	range = glm::clamp(range, 1.f, 6.f);
	knockBack = glm::clamp(knockBack, 0.f, 30.f);
	speed = glm::clamp(speed, -10.f, 20.f);
	drawSpeed = glm::clamp(drawSpeed, -10.f, 20.f);
	armourPenetration = glm::clamp(armourPenetration, 0.f, 999.f);
}

float WeaponStats::getKnockBackNormalized()
{
	return glm::clamp(knockBack, 0.f, 30.f) / 30.f;
}

glm::vec2 WeaponStats::getTimerCulldownRangeForAttacks()
{
	glm::vec2 ret = {0.8, 2.0};

	float normalizedSpeed = (glm::clamp(speed, -10.f, 20.f) + 10.f) / 30.f;
	normalizedSpeed = std::pow(normalizedSpeed, 2.f);
	normalizedSpeed = 1.2f + normalizedSpeed * 3.f;

	ret.x /= normalizedSpeed;
	ret.y /= normalizedSpeed;
	return ret;
}

float WeaponStats::getUIMoveSpeed()
{
	// Speed is intentionally allowed down to -10 for heavy weapons. The old
	// implementation normalised into `s` and then accidentally evaluated
	// sqrt(speed), producing NaN for war hammers and other negative-speed items.
	const float normalizedSpeed = (glm::clamp(speed, -10.f, 20.f) + 10.f) / 30.f;
	return std::sqrt(normalizedSpeed) * 0.7f + 0.6f;
}

float WeaponStats::getSpeedNormalizedInSecconds() const
{
	float normalizedSpeed = (glm::clamp(speed, -10.f, 20.f) + 10.f) / 30.f;
	normalizedSpeed = 1.f - normalizedSpeed;
	normalizedSpeed = std::pow(normalizedSpeed, 2.f);
	normalizedSpeed = 0.1f + normalizedSpeed * 2.0f;
	return normalizedSpeed;
}

float WeaponStats::getDrawSpeedNormalizedInSecconds() const
{
	float normalizedSpeed = (glm::clamp(drawSpeed, -10.f, 20.f) + 10.f) / 30.f;
	normalizedSpeed = 1.f - normalizedSpeed;
	normalizedSpeed = std::pow(normalizedSpeed, 2.f);
	normalizedSpeed = 0.1f + normalizedSpeed * 2.0f;
	return normalizedSpeed;
}

float WeaponStats::getAccuracyAdjusted()
{
	float accuracyNormalized = (glm::clamp(accuracy, -10.f, 20.f) + 10.f) / 30.f;
	accuracyNormalized -= 0.5f;
	accuracyNormalized *= 0.5f;
	return accuracyNormalized;
}

float WeaponStats::getAccuracyNormalized() const
{
	return (glm::clamp(accuracy, -10.f, 20.f) + 10.f) / 30.f;
}

float WeaponStats::getAccuracyNormalizedNegative() const
{
	return glm::clamp(accuracy, -10.f, 20.f) / 20.f;
}

std::string WeaponStats::formatDataToString() const
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(1);

	oss << "\nMele damage: " << (int)damage;
	oss << "\nCritical damage: " << (int)critDamage;
	oss << "\nCritical chance: " << ((int)(critChance * 100)) << "%";
	oss << "\nSurprise damage: " << (int)surprizeDamage;
	oss << "\nSpeed: " << getSpeedNormalizedInSecconds() << " secconds.";
	oss << "\nDraw speed: " << getDrawSpeedNormalizedInSecconds() << " secconds.";
	oss << "\nArmour penetration: " << (int)armourPenetration;
	oss << "\nKnockback: " << knockBack;
	oss << "\nRange: " << range;
	oss << "\nAccuracy: " << ((int)(getAccuracyNormalizedNegative() * 100)) << "%";

	return oss.str();
}

#include <gameplay/items.h>

WeaponStats Item::getWeaponStats()
{
	WeaponStats stats{};

	auto basicSword = [&](int damage)
	{
		stats = {};
		stats.damage = damage;
		stats.critDamage = damage * 1.5f;
		stats.surprizeDamage = damage * 2.f;
		stats.speed = 7;
		stats.drawSpeed = 3;
	};

	auto basicKnife = [&](int damage)
	{
		stats = {};
		stats.damage = damage;
		stats.critDamage = damage * 2.f;
		stats.surprizeDamage = std::max(damage * 4, 20);
		stats.range = 1.45f;
		stats.knockBack = 1;
		stats.speed = 15;
		stats.drawSpeed = 15;
		stats.critChance = 0.2;
		stats.accuracy = 10;
	};

	auto basicScythe = [&](int damage)
	{
		stats = {};
		stats.damage = damage;
		stats.critDamage = damage * 2.f;
		stats.surprizeDamage = damage * 2.2f;
		stats.critChance = 0.15;
		stats.accuracy = 1;
		stats.armourPenetration = 0;
		stats.speed = 3;
		stats.drawSpeed = 1;
		stats.knockBack = 6;
		stats.range = 2.3;
	};

	auto basicSpear = [&](int damage)
	{
		stats = {};
		stats.damage = damage;
		stats.critDamage = damage * 1.5f;
		stats.surprizeDamage = damage * 2.f;
		stats.range = 3.3;
		stats.speed = 1;
		stats.drawSpeed = 0;
		stats.accuracy = 0;
		stats.knockBack = 6;
	};

	auto basicHammer = [&](int damage)
	{
		stats = {};
		stats.damage = damage;
		stats.critDamage = damage * 1.8f;
		stats.surprizeDamage = damage * 2.f;
		stats.range = 2;
		stats.speed = -4;
		stats.drawSpeed = -6;
		stats.accuracy = 1;
		stats.knockBack = 12;
		stats.range = 1.4;
		stats.armourPenetration = std::max(damage / 3, 5);
	};

	auto basicAxe = [&](int damage)
	{
		stats = {};
		stats.damage = damage;
		stats.critDamage = damage * 1.8f;
		stats.critChance = 0.12;
		stats.surprizeDamage = damage * 2.f;
		stats.range = 1.6;
		stats.speed = 0;
		stats.drawSpeed = -2;
		stats.accuracy = 3;
		stats.knockBack = 8;
		stats.armourPenetration = std::max(damage / 5, 2);
	};

	switch (type)
	{
	case trainingScythe: { basicScythe(7); } break;

	case trainingKnife: { basicKnife(3); } break;
	case copperKnife: { basicKnife(8); } break;
	case leadKnife: { basicKnife(11); } break;
	case ironKnife: { basicKnife(14); } break;
	case silverKnife: { basicKnife(17); } break;
	case goldKnife: { basicKnife(20); } break;

	case trainingSword: { basicSword(5); } break;
	case copperSword: { basicSword(7); } break;
	case bronzeSword: { basicSword(10); } break;
	case leadSword: { basicSword(12); } break;
	case ironSword: { basicSword(20); } break;
	case silverSword: { basicSword(25); } break;
	case goldSword: { basicSword(30); } break;

	case trainingWarHammer: { basicHammer(8); } break;
	case copperWarHammer: { basicHammer(10); } break;
	case leadWarHammer: { basicHammer(16); } break;
	case ironWarHammer: { basicHammer(22); } break;
	case silverWarHammer: { basicHammer(30); } break;
	case goldWarHammer: { basicHammer(40); } break;

	case trainingSpear: { basicSpear(4); } break;
	case copperSpear: { basicSpear(6); } break;
	case leadSpear: { basicSpear(10); } break;
	case ironSpear: { basicSpear(15); } break;
	case silverSpear: { basicSpear(21); } break;
	case goldSpear: { basicSpear(27); } break;

	case trainingBattleAxe: { basicAxe(6); } break;
	case copperBattleAxe: { basicAxe(8); } break;
	case leadBattleAxe: { basicAxe(12); } break;
	case ironBattleAxe: { basicAxe(20); } break;
	case silverBattleAxe: { basicAxe(28); } break;
	case goldBattleAxe: { basicAxe(34); } break;
	}

	stats.normalize();
	return stats;
}
