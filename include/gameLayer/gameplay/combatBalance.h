#pragma once

#include <algorithm>

constexpr float ZOMBIE_WALKER_ATTACK_COOLDOWN = 1.5f;
constexpr float ZOMBIE_RUNNER_ATTACK_COOLDOWN = 4.f;
constexpr float ZOMBIE_BRUTE_ATTACK_COOLDOWN = 2.35f;

inline float getKnockBackResistanceMultiplier(float resistancePercent)
{
	// EntityStats supports resistance in [-300, 100]. Keep the helper defensive
	// so malformed/external values cannot invert an impulse or amplify it without bound.
	return std::clamp(1.f - resistancePercent / 100.f, 0.f, 4.f);
}
