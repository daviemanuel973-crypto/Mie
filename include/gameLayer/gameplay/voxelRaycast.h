#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

// Visits each voxel intersected by a ray. Mie blocks are centred on integer
// coordinates, so shifting the ray by 0.5 maps them to conventional unit cells.
template <class Visitor>
bool visitVoxelsOnRay(glm::dvec3 origin, glm::dvec3 direction, double maxDistance,
	Visitor &&visitor)
{
	if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(origin.z) ||
		!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
		!std::isfinite(direction.z) || !std::isfinite(maxDistance) || maxDistance < 0.0 ||
		maxDistance > 10'000.0)
	{
		return false;
	}

	const double directionLength = glm::length(direction);
	if (!std::isfinite(directionLength) || directionLength <= 1e-12) { return false; }
	direction /= directionLength;

	const glm::dvec3 shiftedOrigin = origin + glm::dvec3(0.5);
	const double coordinateMargin = maxDistance + 2.0;
	for (int axis = 0; axis < 3; ++axis)
	{
		if (shiftedOrigin[axis] <= static_cast<double>(std::numeric_limits<int>::min()) + coordinateMargin ||
			shiftedOrigin[axis] >= static_cast<double>(std::numeric_limits<int>::max()) - coordinateMargin)
		{
			return false;
		}
	}
	glm::ivec3 voxel = glm::ivec3(glm::floor(shiftedOrigin));
	const glm::ivec3 step(
		direction.x > 0.0 ? 1 : (direction.x < 0.0 ? -1 : 0),
		direction.y > 0.0 ? 1 : (direction.y < 0.0 ? -1 : 0),
		direction.z > 0.0 ? 1 : (direction.z < 0.0 ? -1 : 0));

	const double infinity = std::numeric_limits<double>::infinity();
	glm::dvec3 delta(
		step.x == 0 ? infinity : std::abs(1.0 / direction.x),
		step.y == 0 ? infinity : std::abs(1.0 / direction.y),
		step.z == 0 ? infinity : std::abs(1.0 / direction.z));
	glm::dvec3 next(
		step.x > 0 ? static_cast<double>(voxel.x + 1) : static_cast<double>(voxel.x),
		step.y > 0 ? static_cast<double>(voxel.y + 1) : static_cast<double>(voxel.y),
		step.z > 0 ? static_cast<double>(voxel.z + 1) : static_cast<double>(voxel.z));
	glm::dvec3 maximum(
		step.x == 0 ? infinity : (next.x - shiftedOrigin.x) / direction.x,
		step.y == 0 ? infinity : (next.y - shiftedOrigin.y) / direction.y,
		step.z == 0 ? infinity : (next.z - shiftedOrigin.z) / direction.z);
	maximum = glm::max(maximum, glm::dvec3(0.0));

	double entryDistance = 0.0;
	const int hardLimit = std::max(8, static_cast<int>(std::ceil(maxDistance * 3.0)) + 8);
	for (int visited = 0; visited < hardLimit && entryDistance <= maxDistance; ++visited)
	{
		if (visitor(voxel, entryDistance)) { return true; }

		if (maximum.x <= maximum.y && maximum.x <= maximum.z)
		{
			entryDistance = maximum.x;
			maximum.x += delta.x;
			voxel.x += step.x;
		}
		else if (maximum.y <= maximum.z)
		{
			entryDistance = maximum.y;
			maximum.y += delta.y;
			voxel.y += step.y;
		}
		else
		{
			entryDistance = maximum.z;
			maximum.z += delta.z;
			voxel.z += step.z;
		}
	}
	return false;
}
