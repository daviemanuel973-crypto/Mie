#include <gameplay/itemDurability.h>
#include <gameplay/items.h>

#include <algorithm>

namespace
{
	constexpr unsigned char durabilityMagic[] = {'M', 'I', 'E', 'D'};
	constexpr unsigned char durabilityFormatVersion = 1;
	constexpr std::size_t durabilityMetadataSize = 7;

	bool decodeDurability(const std::vector<unsigned char> &metaData,
		std::uint16_t &remaining)
	{
		if (metaData.size() != durabilityMetadataSize ||
			metaData[0] != durabilityMagic[0] ||
			metaData[1] != durabilityMagic[1] ||
			metaData[2] != durabilityMagic[2] ||
			metaData[3] != durabilityMagic[3] ||
			metaData[4] != durabilityFormatVersion)
		{
			return false;
		}

		remaining = static_cast<std::uint16_t>(metaData[5]) |
			(static_cast<std::uint16_t>(metaData[6]) << 8);
		return true;
	}

	void encodeDurability(std::vector<unsigned char> &metaData,
		std::uint16_t remaining)
	{
		metaData = {
			durabilityMagic[0], durabilityMagic[1],
			durabilityMagic[2], durabilityMagic[3],
			durabilityFormatVersion,
			static_cast<unsigned char>(remaining & 0xff),
			static_cast<unsigned char>((remaining >> 8) & 0xff),
		};
	}
}

std::uint16_t getMaximumItemDurability(unsigned short itemType)
{
	switch (itemType)
	{
		case ItemTypes::trainingScythe:
		case ItemTypes::trainingSword:
		case ItemTypes::trainingWarHammer:
		case ItemTypes::trainingSpear:
		case ItemTypes::trainingKnife:
		case ItemTypes::trainingBattleAxe:
			return 128;

		case ItemTypes::goldPickaxe:
		case ItemTypes::goldAxe:
		case ItemTypes::goldShovel:
		case ItemTypes::goldSword:
		case ItemTypes::goldWarHammer:
		case ItemTypes::goldSpear:
		case ItemTypes::goldKnife:
		case ItemTypes::goldBattleAxe:
			return 96;

		case ItemTypes::copperPickaxe:
		case ItemTypes::copperAxe:
		case ItemTypes::copperShovel:
		case ItemTypes::copperSword:
		case ItemTypes::copperWarHammer:
		case ItemTypes::copperSpear:
		case ItemTypes::copperKnife:
		case ItemTypes::copperBattleAxe:
			return 180;

		case ItemTypes::bronzePickaxe:
		case ItemTypes::bronzeAxe:
		case ItemTypes::bronzeShovel:
		case ItemTypes::bronzeSword:
			return 240;

		case ItemTypes::silverPickaxe:
		case ItemTypes::silverAxe:
		case ItemTypes::silverShovel:
		case ItemTypes::silverSword:
		case ItemTypes::silverWarHammer:
		case ItemTypes::silverSpear:
		case ItemTypes::silverKnife:
		case ItemTypes::silverBattleAxe:
			return 260;

		case ItemTypes::leadPickaxe:
		case ItemTypes::leadAxe:
		case ItemTypes::leadShovel:
		case ItemTypes::leadSword:
		case ItemTypes::leadWarHammer:
		case ItemTypes::leadSpear:
		case ItemTypes::leadKnife:
		case ItemTypes::leadBattleAxe:
			return 280;

		case ItemTypes::ironPickaxe:
		case ItemTypes::ironAxe:
		case ItemTypes::ironShovel:
		case ItemTypes::ironSword:
		case ItemTypes::ironWarHammer:
		case ItemTypes::ironSpear:
		case ItemTypes::ironKnife:
		case ItemTypes::ironBattleAxe:
			return 360;
	}

	return 0;
}

bool itemUsesDurability(unsigned short itemType)
{
	return getMaximumItemDurability(itemType) != 0;
}

std::uint16_t getRemainingItemDurability(unsigned short itemType,
	const std::vector<unsigned char> &metaData)
{
	const std::uint16_t maximum = getMaximumItemDurability(itemType);
	if (maximum == 0) { return 0; }

	std::uint16_t remaining = maximum;
	if (!decodeDurability(metaData, remaining)) { return maximum; }
	return std::min(remaining, maximum);
}

float getItemDurabilityFraction(unsigned short itemType,
	const std::vector<unsigned char> &metaData)
{
	const std::uint16_t maximum = getMaximumItemDurability(itemType);
	if (maximum == 0) { return 0.f; }
	return static_cast<float>(getRemainingItemDurability(itemType, metaData)) /
		static_cast<float>(maximum);
}

bool sanitizeItemDurabilityMetadata(unsigned short itemType,
	std::vector<unsigned char> &metaData)
{
	const std::uint16_t maximum = getMaximumItemDurability(itemType);
	if (maximum == 0 || metaData.empty()) { return true; }

	std::uint16_t remaining = 0;
	if (!decodeDurability(metaData, remaining))
	{
		metaData.clear();
		return true;
	}

	if (remaining == 0) { return false; }
	if (remaining > maximum) { encodeDurability(metaData, maximum); }
	return true;
}

ItemDurabilityUseResult consumeItemDurability(unsigned short itemType,
	std::vector<unsigned char> &metaData, std::uint16_t amount)
{
	const std::uint16_t maximum = getMaximumItemDurability(itemType);
	if (maximum == 0 || amount == 0)
	{
		return ItemDurabilityUseResult::notApplicable;
	}

	const std::uint16_t remaining = getRemainingItemDurability(itemType, metaData);
	if (amount >= remaining)
	{
		metaData.clear();
		return ItemDurabilityUseResult::broken;
	}

	encodeDurability(metaData, static_cast<std::uint16_t>(remaining - amount));
	return ItemDurabilityUseResult::damaged;
}
