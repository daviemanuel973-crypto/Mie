#pragma once

#include <cstdint>
#include <limits>

namespace mie::network
{
	struct InteractionInvalidationPlan
	{
		bool apply = false;
		std::uint32_t nextRevision = 0;
	};

	inline InteractionInvalidationPlan planInteractionInvalidation(
		std::uint32_t serverRevision, std::uint32_t localRevision)
	{
		// Revision zero is the protocol sentinel for "no invalidation this frame".
		if (serverRevision == 0)
		{
			return {false, localRevision};
		}

		// Once the client has already advanced past a revision, delayed or duplicate
		// invalidations for that old revision are stale and must not roll back newer work.
		if (serverRevision < localRevision)
		{
			return {false, localRevision};
		}

		// The server is authoritative. Equal revisions invalidate the current local
		// prediction; a server revision ahead of ours also means the client must rebase.
		const auto maxRevision = std::numeric_limits<std::uint32_t>::max();
		const std::uint32_t nextRevision = serverRevision == maxRevision
			? maxRevision : serverRevision + 1u;
		return {true, nextRevision};
	}
}
