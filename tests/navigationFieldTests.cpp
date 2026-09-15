#include <gameplay/navigationField.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <tuple>

#ifdef MIE_NAV_BASELINE
#include "navigationBaseline.h"
#endif

#define REQUIRE(condition) do { if (!(condition)) { std::cerr << "Failure at line " << __LINE__ << ": " #condition "\n"; std::exit(1); } } while (false)

struct TestBlock
{
	bool solid = false;
	bool isColidable() const { return solid; }
};

std::uint64_t fingerprint(const PathFindingField &field)
{
	std::vector<std::array<int, 7>> nodes;
	for (const auto &entry : field)
	{
		const auto &p = entry.first, &parent = entry.second.returnPos;
		nodes.push_back({p.x, p.y, p.z, parent.x, parent.y, parent.z, entry.second.level});
	}
	std::sort(nodes.begin(), nodes.end());
	std::uint64_t hash = 14695981039346656037ULL;
	for (const auto &node : nodes)
	{
		for (int component : node)
		{
			for (unsigned int byte = 0; byte < 4; ++byte)
			{
				hash ^= (static_cast<std::uint32_t>(component) >> (byte * 8)) & 255u;
				hash *= 1099511628211ULL;
			}
		}
	}
	return hash;
}

int main()
{
	// Generated from the shipped v0.10.0 BFS; includes every position, parent and depth.
	constexpr std::array<std::uint64_t, 7> expectedHashes = {14751944927619301897ULL, 13005340470797770346ULL, 13774287054671962195ULL, 16147742844024335913ULL, 16606350410595487781ULL, 16606350410595487781ULL, 14720358784578329855ULL};
	PathFindingField field;
	std::vector<PathFindingNode> queue;
	for (int fixture = 0; fixture < 7; ++fixture)
	{
		const glm::ivec3 origin = fixture == 6 ? glm::ivec3(-31, 2, -17) : glm::ivec3(0, 1, 0);
		TestBlock air, solid{true};
		std::size_t queries = 0;
		auto getter = [&](glm::ivec3 p) -> TestBlock *
		{
			++queries;
			const auto q = p - origin;
			if (std::abs(q.x) > 52 || std::abs(q.z) > 52 || q.y < -5 || q.y > 8) { return nullptr; }
			if (fixture == 3 && q.x > 3 && q.z < 0) { return nullptr; }
			if (fixture == 4) { return &solid; }
			if (fixture == 5) { return &air; }
			int ground = -1;
			if (fixture == 1) { ground += std::clamp(q.x / 6, -3, 3); }
			bool blocked = q.y <= ground;
			if (fixture == 2 && q.x % 7 == 3 && q.z % 9 != 0 && q.y < 3) { blocked = true; }
			return blocked ? &solid : &air;
		};
		const auto start = std::chrono::steady_clock::now();
#ifdef MIE_NAV_BASELINE
		const auto expanded = buildLegacyField(field, origin, getter, queue);
#else
		const auto expanded = mie::navigation::buildField(field, origin, getter, queue);
#endif
		const auto micros = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
		const auto hash = fingerprint(field);
		std::cout << fixture << " " << field.size() << " " << hash << " " << expanded << " " << queries << " " << micros << "\n";
		REQUIRE(field.size() <= 4096 && queue.size() <= 4096 && expanded <= queue.size());
#ifndef MIE_NAV_BASELINE
		REQUIRE(hash == expectedHashes[fixture]);
		const auto storage = queue.data();
		mie::navigation::buildField(field, origin, getter, queue);
		REQUIRE(fingerprint(field) == hash && queue.data() == storage);
#endif
	}
}
