#include <multyPlayer/playerPersistence.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>

namespace
{
	constexpr std::array<unsigned char, 8> playerSaveMagic = {
		'M', 'I', 'E', 'P', 'L', 'Y', 'R', 0
	};

	template <class T>
	void appendValue(std::vector<unsigned char> &data, const T &value)
	{
		static_assert(std::is_trivially_copyable<T>::value, "Save values must be trivial");
		const auto oldSize = data.size();
		data.resize(oldSize + sizeof(T));
		std::memcpy(data.data() + oldSize, &value, sizeof(T));
	}

	void appendBytes(std::vector<unsigned char> &data, const void *value, std::size_t size)
	{
		if (!size) { return; }
		const auto oldSize = data.size();
		data.resize(oldSize + size);
		std::memcpy(data.data() + oldSize, value, size);
	}

	class SaveReader
	{
	public:
		SaveReader(const void *data, std::size_t size)
			: data(static_cast<const unsigned char *>(data)), size(size) {}

		template <class T>
		bool read(T &value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "Save values must be trivial");
			return readBytes(&value, sizeof(T));
		}

		bool readBytes(void *value, std::size_t count)
		{
			if ((!data && count) || count > size - std::min(position, size)) { return false; }
			if (count) { std::memcpy(value, data + position, count); }
			position += count;
			return true;
		}

		std::size_t remaining() const
		{
			return position <= size ? size - position : 0;
		}

	private:
		const unsigned char *data = nullptr;
		std::size_t size = 0;
		std::size_t position = 0;
	};
}

bool PlayerIdentity::isValid() const
{
	return std::any_of(bytes.begin(), bytes.end(), [](unsigned char value) { return value != 0; });
}

std::string PlayerIdentity::toString() const
{
	std::ostringstream result;
	result << std::hex << std::setfill('0');
	for (const auto value : bytes)
	{
		result << std::setw(2) << static_cast<unsigned int>(value);
	}
	return result.str();
}

std::vector<unsigned char> formatPlayerSaveSnapshot(const PlayerSaveSnapshot &snapshot)
{
	if (!snapshot.identity.isValid() ||
		snapshot.inventory.size() > MAX_PLAYER_INVENTORY_SAVE_SIZE ||
		snapshot.inventory.size() > std::numeric_limits<std::uint32_t>::max())
	{
		return {};
	}

	std::vector<unsigned char> data;
	data.reserve(64 + snapshot.inventory.size());
	appendBytes(data, playerSaveMagic.data(), playerSaveMagic.size());
	appendValue(data, PLAYER_SAVE_FORMAT_VERSION);
	appendBytes(data, snapshot.identity.bytes.data(), snapshot.identity.bytes.size());
	for (const auto coordinate : snapshot.position) { appendValue(data, coordinate); }
	appendValue(data, snapshot.life);
	appendValue(data, snapshot.maxLife);
	appendValue(data, snapshot.hunger);
	appendValue(data, snapshot.maxHunger);
	appendValue(data, snapshot.gameMode);
	const auto inventorySize = static_cast<std::uint32_t>(snapshot.inventory.size());
	appendValue(data, inventorySize);
	appendBytes(data, snapshot.inventory.data(), snapshot.inventory.size());
	return data;
}

bool parsePlayerSaveSnapshot(const void *data, std::size_t size, PlayerSaveSnapshot &snapshot)
{
	snapshot = {};
	SaveReader reader(data, size);
	std::array<unsigned char, playerSaveMagic.size()> magic = {};
	std::uint32_t version = 0;
	std::uint32_t inventorySize = 0;

	if (!reader.readBytes(magic.data(), magic.size()) || magic != playerSaveMagic ||
		!reader.read(version) || version != PLAYER_SAVE_FORMAT_VERSION ||
		!reader.readBytes(snapshot.identity.bytes.data(), snapshot.identity.bytes.size()) ||
		!snapshot.identity.isValid())
	{
		return false;
	}

	for (auto &coordinate : snapshot.position)
	{
		if (!reader.read(coordinate) || !std::isfinite(coordinate) ||
			std::abs(coordinate) > 30'000'000.0)
		{
			return false;
		}
	}

	if (!reader.read(snapshot.life) || !reader.read(snapshot.maxLife) ||
		!reader.read(snapshot.hunger) || !reader.read(snapshot.maxHunger) ||
		!reader.read(snapshot.gameMode) || !reader.read(inventorySize) ||
		inventorySize > MAX_PLAYER_INVENTORY_SAVE_SIZE || inventorySize != reader.remaining())
	{
		return false;
	}

	snapshot.inventory.resize(inventorySize);
	return reader.readBytes(snapshot.inventory.data(), snapshot.inventory.size()) && reader.remaining() == 0;
}
