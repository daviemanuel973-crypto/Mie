#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct PathFindingNode
{
	glm::ivec3 returnPos = {};
	int level = 0;
};

using PathFindingField = std::unordered_map<glm::ivec3, PathFindingNode>;
using PathFindingFieldView = std::unordered_map<std::uint64_t, const PathFindingField *>;

namespace mie::navigation
{
	constexpr std::size_t maximumNavigationNodes = 4096;

	// Reuse one bounded contiguous queue per tick thread. Once the field is full,
	// queued nodes cannot add a result, so do not continue querying world blocks.
	// Neighbour order, parent links and the 40-step horizon match v0.10.0.
	template <class BlockGetter>
	std::size_t buildField(PathFindingField &positions, glm::ivec3 origin,
		BlockGetter getBlock, std::vector<PathFindingNode> &queue)
	{
		queue.clear();
		if (queue.capacity() < maximumNavigationNodes) { queue.reserve(maximumNavigationNodes); }
		auto addNode = [&](PathFindingNode node, glm::ivec3 displacement)
		{
			PathFindingNode newEntry;
			newEntry.returnPos = node.returnPos;
			newEntry.level = node.level + 1;

			if (positions.size() >= maximumNavigationNodes) { return; }
			positions[node.returnPos + displacement] = newEntry;

			if (node.level < 40)
			{
				newEntry.returnPos = node.returnPos + displacement;
				queue.push_back(newEntry);
			}
		};

		auto checkDown = [&](PathFindingNode node, glm::ivec3 disp)
		{
			glm::ivec3 displacement = glm::ivec3(0, -1, 0) + disp;

			auto found = positions.find(node.returnPos + displacement);
			if (found == positions.end())
			{
				auto b = getBlock(node.returnPos + displacement);
				if (b && !b->isColidable())
				{
					auto b2 = getBlock(node.returnPos + displacement + glm::ivec3(0, -1, 0));
					if (b2 && b2->isColidable())
					{
						addNode(node, displacement);
					}
				}
			}

		};

		auto checkSides = [&](PathFindingNode node, glm::ivec3 displacement)
		{
			auto found = positions.find(node.returnPos + displacement);
			if (found == positions.end())
			{
				auto b = getBlock(node.returnPos + displacement);
				if (b && !b->isColidable())
				{

					auto bUp = getBlock(node.returnPos + displacement + glm::ivec3(0, 1, 0));
					if (!bUp || !bUp->isColidable())
					{
						auto bDown = getBlock(node.returnPos + displacement + glm::ivec3(0, -1, 0));
						auto bDown2 = getBlock(node.returnPos + displacement + glm::ivec3(0, -2, 0));

						if ((bDown && bDown->isColidable())
							|| (bDown2 && bDown2->isColidable())
							)
						{
							addNode(node, displacement);

							if((!bDown || !bDown->isColidable()) && bDown2 && bDown2->isColidable())
							{
								checkDown(node, displacement);
							}
						}

					}

				}
			}
		};

		auto checkUp = [&](PathFindingNode node, glm::ivec3 displacement)
		{
			auto found = positions.find(node.returnPos + displacement);
			if (found == positions.end())
			{
				auto b = getBlock(node.returnPos + displacement);
				if (b && !b->isColidable())
				{
					addNode(node, displacement);
				}
			}
		};

		positions.clear();
		if (positions.bucket_count() < maximumNavigationNodes) { positions.reserve(maximumNavigationNodes); }
		PathFindingNode root;
		root.returnPos = origin;
		root.level = 0;
		queue.push_back(root);
		positions[origin] = root;

		std::size_t cursor = 0;
		while (cursor < queue.size() && positions.size() < maximumNavigationNodes)
		{
			PathFindingNode node = queue[cursor];
			++cursor;

			checkSides(node, {1,0,0});
			checkSides(node, {-1,0,0});
			checkSides(node, {0,0,1});
			checkSides(node, {0,0,-1});


			auto bDown = getBlock(node.returnPos + glm::ivec3(0,-1,0));
			if (bDown && bDown->isColidable())
			{
				checkUp(node, {0,1,0});
				checkUp(node, {0,2,0});
				checkUp(node, {0,3,0});
				checkUp(node, {0,4,0});
			}

		}
		return cursor;
	}
}
