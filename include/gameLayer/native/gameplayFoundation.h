#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace mie::native
{
	enum class DirtyFlag : std::uint16_t
	{
		None = 0,
		Persistence = 1u << 0u,
		Network = 1u << 1u,
		Topology = 1u << 2u,
		Inventory = 1u << 3u,
		Process = 1u << 4u,
		Presentation = 1u << 5u,
	};

	DirtyFlag operator|(DirtyFlag left, DirtyFlag right);
	DirtyFlag operator&(DirtyFlag left, DirtyFlag right);
	DirtyFlag &operator|=(DirtyFlag &left, DirtyFlag right);
	DirtyFlag without(DirtyFlag value, DirtyFlag removed);
	bool hasFlag(DirtyFlag value, DirtyFlag flag);

	struct GameplayEvent
	{
		std::uint64_t sequence = 0;
		std::string type;
		std::uint64_t subject = 0;
		std::uint64_t tick = 0;
	};

	class GameplayEventStream
	{
	public:
		explicit GameplayEventStream(std::size_t capacity = 1024);
		std::uint64_t publish(std::string type, std::uint64_t subject, std::uint64_t tick);
		const std::deque<GameplayEvent> &events() const { return storedEvents; }
		std::uint64_t droppedEvents() const { return dropped; }
		void clear();

	private:
		std::size_t maxCapacity = 1024;
		std::uint64_t nextSequence = 1;
		std::uint64_t dropped = 0;
		std::deque<GameplayEvent> storedEvents;
	};

	enum class InterestScope : std::uint8_t
	{
		Chunk,
		Player,
		Interaction,
		PartyOrSettlement,
		Global,
	};

	struct InterestDescriptor
	{
		InterestScope scope = InterestScope::Chunk;
		int chunkX = 0;
		int chunkZ = 0;
		std::uint64_t ownerId = 0;
		std::uint64_t interactionId = 0;
		std::uint64_t groupId = 0;
	};

	struct ObserverContext
	{
		int chunkX = 0;
		int chunkZ = 0;
		int chunkRadius = 0;
		std::uint64_t playerId = 0;
		std::uint64_t interactionId = 0;
		std::uint64_t groupId = 0;
	};

	bool isRelevantToObserver(const InterestDescriptor &interest,
		const ObserverContext &observer);
}
