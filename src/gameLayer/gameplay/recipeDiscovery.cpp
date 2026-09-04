#include <gameplay/recipeDiscovery.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace
{
	constexpr std::array<unsigned char, 4> discoveryMagic = {'M', 'I', 'E', 'R'};
	constexpr std::uint8_t legacyDiscoveryVersion = 1;
	constexpr std::uint8_t discoveryVersion = 2;
}

bool RecipeDiscovery::typeToIndex(std::uint16_t type, std::size_t &index)
{
	if (type > 0 && type < BlockTypeCount)
	{
		index = type;
		return true;
	}
	if (type >= FirstItemType && type < LastItemTypeExclusive)
	{
		index = BlockTypeCount + static_cast<std::size_t>(type - FirstItemType);
		return true;
	}
	return false;
}

bool RecipeDiscovery::learnType(std::uint16_t type)
{
	std::size_t index = 0;
	if (!typeToIndex(type, index)) { return false; }
	const unsigned char mask = static_cast<unsigned char>(1u << (index % 8));
	unsigned char &byte = knownTypes[index / 8];
	if ((byte & mask) != 0) { return false; }
	byte = static_cast<unsigned char>(byte | mask);
	return true;
}

bool RecipeDiscovery::knowsType(std::uint16_t type) const
{
	std::size_t index = 0;
	if (!typeToIndex(type, index)) { return false; }
	const unsigned char mask = static_cast<unsigned char>(1u << (index % 8));
	return (knownTypes[index / 8] & mask) != 0;
}

std::size_t RecipeDiscovery::learnedTypeCount() const
{
	std::size_t result = 0;
	for (unsigned char value : knownTypes)
	{
		while (value)
		{
			result += value & 1u;
			value = static_cast<unsigned char>(value >> 1u);
		}
	}
	return result;
}

void RecipeDiscovery::sanitize()
{
	constexpr std::size_t usedBitsInLastByte = KnownTypeCount % 8;
	if constexpr (usedBitsInLastByte != 0)
	{
		const unsigned char mask = static_cast<unsigned char>((1u << usedBitsInLastByte) - 1u);
		knownTypes.back() = static_cast<unsigned char>(knownTypes.back() & mask);
	}
	// Block ID zero is air/empty and never represents a discovered material.
	knownTypes[0] = static_cast<unsigned char>(knownTypes[0] & ~1u);
}

std::size_t RecipeDiscovery::formatIntoData(std::vector<unsigned char> &data) const
{
	RecipeDiscovery sanitized = *this;
	sanitized.sanitize();
	const std::uint16_t payloadSize = static_cast<std::uint16_t>(StorageBytes);
	data.insert(data.end(), discoveryMagic.begin(), discoveryMagic.end());
	data.push_back(discoveryVersion);
	const std::size_t sizeOffset = data.size();
	data.resize(data.size() + sizeof(payloadSize));
	std::memcpy(data.data() + sizeOffset, &payloadSize, sizeof(payloadSize));
	data.insert(data.end(), sanitized.knownTypes.begin(), sanitized.knownTypes.end());
	return SerializedBytes;
}

int RecipeDiscovery::readFromData(const void *data, std::size_t size)
{
	*this = {};
	if (!data || size < HeaderBytes) { return -1; }
	const auto *bytes = static_cast<const unsigned char *>(data);
	if (!std::equal(discoveryMagic.begin(), discoveryMagic.end(), bytes))
	{
		return -1;
	}

	const std::uint8_t version = bytes[discoveryMagic.size()];
	std::uint16_t payloadSize = 0;
	std::memcpy(&payloadSize, bytes + discoveryMagic.size() + sizeof(version),
		sizeof(payloadSize));
	const bool legacyPayload = version == legacyDiscoveryVersion &&
		payloadSize == LegacyV09StorageBytes;
	const bool currentPayload = version == discoveryVersion && payloadSize == StorageBytes;
	if ((!legacyPayload && !currentPayload) || size < HeaderBytes + payloadSize) { return -1; }

	std::copy_n(bytes + HeaderBytes, payloadSize, knownTypes.begin());
	sanitize();
	return static_cast<int>(HeaderBytes + payloadSize);
}
