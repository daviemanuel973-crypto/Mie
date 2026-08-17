#pragma once

#include <cstdint>
#include <vector>

enum class ItemDurabilityUseResult
{
	notApplicable,
	damaged,
	broken,
};

// Durability is stored in Item::metaData so old inventory and dropped-item
// payloads remain readable. Items without durability metadata are treated as
// unused at their material's full durability.
std::uint16_t getMaximumItemDurability(unsigned short itemType);
bool itemUsesDurability(unsigned short itemType);
std::uint16_t getRemainingItemDurability(unsigned short itemType,
	const std::vector<unsigned char> &metaData);
float getItemDurabilityFraction(unsigned short itemType,
	const std::vector<unsigned char> &metaData);

// Returns false only when a valid durable item was persisted at zero and must
// be removed by Item::sanitize(). Unknown legacy metadata is cleared and
// therefore safely becomes full durability.
bool sanitizeItemDurabilityMetadata(unsigned short itemType,
	std::vector<unsigned char> &metaData);

ItemDurabilityUseResult consumeItemDurability(unsigned short itemType,
	std::vector<unsigned char> &metaData, std::uint16_t amount = 1);

// v0.9 repair primitive. Restores at most `amount` durability and returns the
// number of points actually restored. Fresh/full items are left untouched;
// damaged legacy items keep using the same MIED metadata payload, so repair
// does not require an inventory/save migration.
std::uint16_t repairItemDurability(unsigned short itemType,
	std::vector<unsigned char> &metaData, std::uint16_t amount);
