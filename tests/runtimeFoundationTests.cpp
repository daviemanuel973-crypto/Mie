#include <gameplay/physics.h>
#include <gameplay/playerControlSettings.h>
#include <gameplay/voxelRaycast.h>
#include <metrics.h>

#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		std::exit(1); } } while (false)
}

int main()
{
	REQUIRE(divideChunk(0) == 0);
	REQUIRE(divideChunk(15) == 0);
	REQUIRE(divideChunk(16) == 1);
	REQUIRE(divideChunk(-1) == -1);
	REQUIRE(divideChunk(-16) == -1);
	REQUIRE(divideChunk(-17) == -2);
	REQUIRE(divideChunk(-0.5) == 0);
	REQUIRE(divideChunk(-0.5001) == -1);
	REQUIRE(divideChunk(-16.499) == -1);
	REQUIRE(divideChunk(-16.501) == -2);
	REQUIRE(divideChunk(15.499) == 0);
	REQUIRE(divideChunk(15.5) == 1);
	REQUIRE(divideChunk(16.999) == 1);
	REQUIRE(divideChunk(std::numeric_limits<double>::quiet_NaN()) == 0);
	REQUIRE(divideMetaChunk(-33) == -2);
	REQUIRE(divideChunk(INT_MAX) == INT_MAX / CHUNK_SIZE);
	REQUIRE(divideChunk(INT_MIN) == static_cast<int>(std::floor(
		static_cast<double>(INT_MIN) / CHUNK_SIZE)));
	REQUIRE(determineChunkThatIsEntityIn({-16.501, 64.0, -0.501}) == glm::ivec2(-2, -1));
	REQUIRE(determineChunkThatIsEntityIn({-16.001, 64.0, -0.001}) == glm::ivec2(-1, 0));

	MotionState collision;
	collision.setColidesBottom(true);
	collision.setColidesBottom(true);
	REQUIRE(collision.colidesBottom());
	collision.setColidesBottom(false);
	collision.setColidesBottom(false);
	REQUIRE(!collision.colidesBottom());
	collision.setColidesFront(true);
	collision.setColidesBack(true);
	REQUIRE(collision.colidesFront() && collision.colidesBack());
	collision.setColidesFront(false);
	REQUIRE(!collision.colidesFront() && collision.colidesBack());

	BufferedJumpState jump;
	REQUIRE(!updateBufferedJump(jump, 0.016f, true, false));
	REQUIRE(updateBufferedJump(jump, 0.050f, false, true));
	REQUIRE(!updateBufferedJump(jump, 0.016f, false, false));
	jump = {};
	REQUIRE(!updateBufferedJump(jump, 0.016f, false, true));
	REQUIRE(updateBufferedJump(jump, 0.050f, true, false));
	jump = {};
	REQUIRE(!updateBufferedJump(jump, 0.200f, true, false));
	REQUIRE(!updateBufferedJump(jump, 0.200f, false, true));

	std::vector<glm::ivec3> visited;
	REQUIRE(!visitVoxelsOnRay(glm::dvec3(-0.49, 0.0, 0.0), glm::dvec3(1, 0, 0), 2.2,
		[&](glm::ivec3 voxel, double)
		{
			visited.push_back(voxel);
			return false;
		}));
	REQUIRE(visited.size() >= 3);
	REQUIRE(visited[0] == glm::ivec3(0, 0, 0));
	REQUIRE(visited[1] == glm::ivec3(1, 0, 0));
	REQUIRE(visited[2] == glm::ivec3(2, 0, 0));
	visited.clear();
	visitVoxelsOnRay(glm::dvec3(-0.51, 0.0, 0.0), glm::dvec3(-1, 0, 0), 1.2,
		[&](glm::ivec3 voxel, double)
		{
			visited.push_back(voxel);
			return false;
		});
	REQUIRE(visited.size() >= 2 && visited[0].x == -1 && visited[1].x == -2);
	REQUIRE(!visitVoxelsOnRay(glm::dvec3(0), glm::dvec3(0), 5.0,
		[](glm::ivec3, double) { return true; }));

	std::cout << "Runtime foundation tests passed.\n";
	return 0;
}
