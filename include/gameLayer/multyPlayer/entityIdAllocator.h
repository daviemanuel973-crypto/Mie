#pragma once

#include <array>
#include <cstdint>
#include <string>

class PersistentEntityIdAllocator
{
public:
	static constexpr std::size_t TypeSlots = 256;
	static constexpr std::uint64_t RawIdMask = 0x00FFFFFFFFFFFFFFULL;
	static constexpr std::uint64_t LegacyMigrationFloor = (1ULL << 40);
	static constexpr std::uint64_t ReservationBlock = 1024;

	std::uint64_t allocate(const std::string &worldSavePath, unsigned int entityType);
	void observe(std::uint64_t entityId);
	std::uint64_t peek(unsigned int entityType) const;

private:
	void initializeForPath(const std::string &worldSavePath);
	bool reserveRange(unsigned int entityType);
	bool loadState(std::array<std::uint64_t, TypeSlots> &highWater) const;
	bool saveState(const std::array<std::uint64_t, TypeSlots> &highWater) const;
	std::uint64_t makeSessionFloor() const;

	std::array<std::uint64_t, TypeSlots> nextIds = {};
	std::array<std::uint64_t, TypeSlots> reservedUntil = {};
	std::array<std::uint64_t, TypeSlots> observedFloor = {};
	std::string currentWorldSavePath;
	bool initialized = false;
};
