#pragma once

#include <cstdint>

inline unsigned char getEntityTypeFromEID(std::uint64_t entityId)
{
	return static_cast<unsigned char>(entityId >> 56);
}

inline std::uint64_t getOnlyIdFromEID(std::uint64_t entityId)
{
	return entityId & 0x00FFFFFFFFFFFFFFULL;
}
