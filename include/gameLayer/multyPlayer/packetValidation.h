#pragma once

#include <cstddef>
#include <cstdint>

struct PacketPayloadRule
{
	std::size_t minimumSize = 0;
	std::size_t maximumSize = 0;
	std::size_t sizeMultiple = 1;
};

bool getServerPacketPayloadRule(std::uint32_t header, PacketPayloadRule &rule);
bool validateServerPacketPayload(std::uint32_t header, const char *data, std::size_t size);
std::size_t maximumDecompressedServerPayload(std::uint32_t header);

bool getClientPacketPayloadRule(std::uint32_t header, bool compressed,
	PacketPayloadRule &rule);
bool validateClientPacketPayload(std::uint32_t header, const char *data,
	std::size_t size, bool compressed = false);
