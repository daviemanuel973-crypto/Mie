#include <multyPlayer/entityIdAllocator.h>

#include <safeSave.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
	constexpr std::array<unsigned char, 8> StateMagic = {
		'M', 'I', 'E', 'E', 'I', 'D', '2', 0
	};
	constexpr std::uint32_t StateVersion = 1;
	constexpr std::size_t StateHeaderSize = StateMagic.size() + sizeof(StateVersion) + sizeof(std::uint32_t);
	constexpr std::size_t StateSize = StateHeaderSize +
		PersistentEntityIdAllocator::TypeSlots * sizeof(std::uint64_t);

	template <class T>
	void appendValue(std::vector<char> &data, const T &value)
	{
		const auto oldSize = data.size();
		data.resize(oldSize + sizeof(T));
		std::memcpy(data.data() + oldSize, &value, sizeof(T));
	}

	template <class T>
	bool readValue(const std::vector<char> &data, std::size_t &offset, T &value)
	{
		if (offset > data.size() || sizeof(T) > data.size() - offset) { return false; }
		std::memcpy(&value, data.data() + offset, sizeof(T));
		offset += sizeof(T);
		return true;
	}
}

std::uint64_t PersistentEntityIdAllocator::makeSessionFloor() const
{
	const auto milliseconds = static_cast<std::uint64_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	const std::uint64_t timestampFloor = (milliseconds << 10) & RawIdMask;
	return std::max(LegacyMigrationFloor, timestampFloor);
}

bool PersistentEntityIdAllocator::loadState(
	std::array<std::uint64_t, TypeSlots> &highWater) const
{
	highWater.fill(0);
	std::vector<char> data;
	const std::string path = currentWorldSavePath + "/entityIdHighWater";
	if (sfs::safeLoad(data, path.c_str(), false) != sfs::noError || data.size() != StateSize)
	{
		return false;
	}

	if (!std::equal(StateMagic.begin(), StateMagic.end(),
		reinterpret_cast<const unsigned char *>(data.data())))
	{
		return false;
	}

	std::size_t offset = StateMagic.size();
	std::uint32_t version = 0;
	std::uint32_t slots = 0;
	if (!readValue(data, offset, version) || !readValue(data, offset, slots) ||
		version != StateVersion || slots != TypeSlots)
	{
		return false;
	}

	for (auto &value : highWater)
	{
		if (!readValue(data, offset, value) || value > RawIdMask) { return false; }
	}
	return offset == data.size();
}

bool PersistentEntityIdAllocator::saveState(
	const std::array<std::uint64_t, TypeSlots> &highWater) const
{
	std::vector<char> data;
	data.reserve(StateSize);
	data.insert(data.end(), StateMagic.begin(), StateMagic.end());
	appendValue(data, StateVersion);
	const std::uint32_t slots = static_cast<std::uint32_t>(TypeSlots);
	appendValue(data, slots);
	for (const auto value : highWater) { appendValue(data, value); }

	const std::string path = currentWorldSavePath + "/entityIdHighWater";
	return sfs::safeSave(data.data(), data.size(), path.c_str(), true) == sfs::noError;
}

void PersistentEntityIdAllocator::initializeForPath(const std::string &worldSavePath)
{
	currentWorldSavePath = worldSavePath;
	initialized = true;

	const std::uint64_t sessionFloor = makeSessionFloor();
	std::array<std::uint64_t, TypeSlots> persisted = {};
	const bool restored = loadState(persisted);

	for (std::size_t i = 0; i < TypeSlots; ++i)
	{
		const std::uint64_t recovered = restored && persisted[i] >= LegacyMigrationFloor
			? persisted[i] : sessionFloor;
		nextIds[i] = std::max(recovered, observedFloor[i]);
		reservedUntil[i] = nextIds[i];
	}
}

bool PersistentEntityIdAllocator::reserveRange(unsigned int entityType)
{
	if (entityType >= TypeSlots) { return false; }
	if (nextIds[entityType] >= RawIdMask) { return false; }

	auto proposed = reservedUntil;
	proposed[entityType] = std::min(RawIdMask, nextIds[entityType] + ReservationBlock);
	if (!saveState(proposed))
	{
		std::cerr << "Warning: could not persist the entity ID reservation.\n";
		return false;
	}

	reservedUntil = proposed;
	return true;
}

std::uint64_t PersistentEntityIdAllocator::allocate(
	const std::string &worldSavePath, unsigned int entityType)
{
	std::lock_guard<std::mutex> lock(stateMutex);
	if (entityType >= TypeSlots) { return 0; }
	if (!initialized || currentWorldSavePath != worldSavePath)
	{
		initializeForPath(worldSavePath);
	}
	if (nextIds[entityType] >= RawIdMask) { return 0; }

	if (nextIds[entityType] >= reservedUntil[entityType] && !reserveRange(entityType))
	{
		// Keep the running session alive even if the disk is temporarily unavailable.
		// A time-derived migration floor on the next start makes accidental reuse
		// extremely unlikely, while a later successful reservation restores durability.
		reservedUntil[entityType] = std::min(RawIdMask, nextIds[entityType] + ReservationBlock);
	}

	return nextIds[entityType]++;
}

void PersistentEntityIdAllocator::observe(std::uint64_t entityId)
{
	std::lock_guard<std::mutex> lock(stateMutex);
	const unsigned int entityType = static_cast<unsigned int>((entityId >> 56) & 0xFFu);
	if (entityType >= TypeSlots) { return; }

	const std::uint64_t rawId = entityId & RawIdMask;
	if (rawId >= RawIdMask) { return; }
	observedFloor[entityType] = std::max(observedFloor[entityType], rawId + 1);

	if (initialized)
	{
		nextIds[entityType] = std::max(nextIds[entityType], observedFloor[entityType]);
		if (nextIds[entityType] >= reservedUntil[entityType]) { reserveRange(entityType); }
	}
}

std::uint64_t PersistentEntityIdAllocator::peek(unsigned int entityType) const
{
	std::lock_guard<std::mutex> lock(stateMutex);
	if (entityType >= TypeSlots) { return 0; }
	const std::uint64_t current = initialized ? nextIds[entityType] : LegacyMigrationFloor;
	return std::max(current, observedFloor[entityType]);
}
