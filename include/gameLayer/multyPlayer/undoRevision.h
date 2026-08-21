#pragma once

#include <algorithm>
#include <limits>

enum class UndoInvalidationDisposition
{
	IgnoreStale,
	ApplyAndResync,
};

template <typename Revision>
constexpr UndoInvalidationDisposition classifyUndoInvalidation(Revision localRevision,
	Revision incomingRevision)
{
	return incomingRevision < localRevision ? UndoInvalidationDisposition::IgnoreStale :
		UndoInvalidationDisposition::ApplyAndResync;
}

template <typename Revision>
constexpr Revision revisionAfterUndoInvalidation(Revision localRevision,
	Revision incomingRevision)
{
	const Revision newestRevision = std::max(localRevision, incomingRevision);
	return newestRevision == std::numeric_limits<Revision>::max() ? newestRevision :
		static_cast<Revision>(newestRevision + 1);
}
