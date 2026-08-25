#include <multyPlayer/interactionInvalidation.h>

#include <cstdint>
#include <iostream>
#include <limits>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed: " #condition " at line " << __LINE__ << "\n"; \
		return 1; \
	} } while (false)
}

int main()
{
	using mie::network::planInteractionInvalidation;

	// Zero is the per-frame sentinel and must never mutate client state.
	auto plan = planInteractionInvalidation(0, 7);
	REQUIRE(!plan.apply);
	REQUIRE(plan.nextRevision == 7);

	// Delayed/duplicate invalidation from an already handled revision is stale.
	plan = planInteractionInvalidation(6, 7);
	REQUIRE(!plan.apply);
	REQUIRE(plan.nextRevision == 7);

	// Current-revision invalidation rolls back prediction and advances once.
	plan = planInteractionInvalidation(7, 7);
	REQUIRE(plan.apply);
	REQUIRE(plan.nextRevision == 8);

	// If the server is ahead, rebase beyond the authoritative revision instead of
	// asserting/crashing or continuing to emit events under a stale revision.
	plan = planInteractionInvalidation(11, 7);
	REQUIRE(plan.apply);
	REQUIRE(plan.nextRevision == 12);

	// Avoid integer wrap at the theoretical protocol limit.
	const auto maxRevision = std::numeric_limits<std::uint32_t>::max();
	plan = planInteractionInvalidation(maxRevision, maxRevision);
	REQUIRE(plan.apply);
	REQUIRE(plan.nextRevision == maxRevision);

	std::cout << "Interaction invalidation tests passed.\n";
	return 0;
}
