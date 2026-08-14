#include <gameplay/environmentMotion.h>

#include <algorithm>
#include <cmath>

void applyCobwebMotion(glm::vec3 &velocity, glm::vec3 &acceleration, float deltaTime)
{
	const float elapsed = std::clamp(deltaTime, 0.f, 0.25f);
	const float horizontalDamping = std::exp(-9.f * elapsed);
	const float verticalDamping = std::exp(-6.f * elapsed);
	velocity.x *= horizontalDamping;
	velocity.z *= horizontalDamping;
	velocity.y *= verticalDamping;
	velocity.y = std::clamp(velocity.y, -2.5f, 3.2f);
	acceleration *= 0.15f;
}
