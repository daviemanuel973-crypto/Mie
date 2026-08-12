#include <native/gameplayFoundation.h>

#include <algorithm>
#include <utility>

namespace mie::native
{
	DirtyFlag operator|(DirtyFlag left, DirtyFlag right)
	{
		return static_cast<DirtyFlag>(static_cast<std::uint16_t>(left) |
			static_cast<std::uint16_t>(right));
	}

	DirtyFlag operator&(DirtyFlag left, DirtyFlag right)
	{
		return static_cast<DirtyFlag>(static_cast<std::uint16_t>(left) &
			static_cast<std::uint16_t>(right));
	}

	DirtyFlag &operator|=(DirtyFlag &left, DirtyFlag right)
	{
		left = left | right;
		return left;
	}

	DirtyFlag without(DirtyFlag value, DirtyFlag removed)
	{
		return static_cast<DirtyFlag>(static_cast<std::uint16_t>(value) &
			~static_cast<std::uint16_t>(removed));
	}

	bool hasFlag(DirtyFlag value, DirtyFlag flag)
	{
		return (value & flag) != DirtyFlag::None;
	}

	GameplayEventStream::GameplayEventStream(std::size_t capacity):
		maxCapacity(std::max<std::size_t>(capacity, 1))
	{
	}

	std::uint64_t GameplayEventStream::publish(std::string type,
		std::uint64_t subject, std::uint64_t tick)
	{
		if (storedEvents.size() >= maxCapacity)
		{
			storedEvents.pop_front();
			++dropped;
		}
		const std::uint64_t sequence = nextSequence++;
		storedEvents.push_back({sequence, std::move(type), subject, tick});
		return sequence;
	}

	void GameplayEventStream::clear()
	{
		storedEvents.clear();
		nextSequence = 1;
		dropped = 0;
	}

	bool isRelevantToObserver(const InterestDescriptor &interest,
		const ObserverContext &observer)
	{
		switch (interest.scope)
		{
			case InterestScope::Chunk:
				return std::abs(interest.chunkX - observer.chunkX) <= observer.chunkRadius &&
					std::abs(interest.chunkZ - observer.chunkZ) <= observer.chunkRadius;
			case InterestScope::Player:
				return interest.ownerId != 0 && interest.ownerId == observer.playerId;
			case InterestScope::Interaction:
				return interest.interactionId != 0 &&
					interest.interactionId == observer.interactionId;
			case InterestScope::PartyOrSettlement:
				return interest.groupId != 0 && interest.groupId == observer.groupId;
			case InterestScope::Global:
				return true;
		}
		return false;
	}
}
