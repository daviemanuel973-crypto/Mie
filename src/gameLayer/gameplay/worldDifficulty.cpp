#include <gameplay/worldDifficulty.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>

namespace
{
	constexpr std::array<unsigned char, 8> difficultyMagic = {
		'M', 'I', 'E', 'D', 'I', 'F', 'F', '1'
	};
	constexpr const char *difficultyFileName = "worldDifficulty";

	WorldDifficultySettings serverSettings;
	WorldDifficultySettings clientSettings;

	template <typename T>
	void appendValue(std::vector<unsigned char> &data, const T &value)
	{
		const auto previousSize = data.size();
		data.resize(previousSize + sizeof(value));
		std::memcpy(data.data() + previousSize, &value, sizeof(value));
	}

	template <typename T>
	bool readValue(const char *data, std::size_t size, std::size_t &offset, T &value)
	{
		if (!data || offset > size || sizeof(value) > size - offset) { return false; }
		std::memcpy(&value, data + offset, sizeof(value));
		offset += sizeof(value);
		return true;
	}
}

void WorldDifficultySettings::sanitize()
{
	const auto rawDifficulty = static_cast<std::uint8_t>(difficulty);
	if (rawDifficulty > static_cast<std::uint8_t>(WorldDifficulty::Hard))
	{
		difficulty = WorldDifficulty::Normal;
	}
	if (hardcore) { difficulty = WorldDifficulty::Hard; }
}

const char *getWorldDifficultyName(WorldDifficulty difficulty)
{
	switch (difficulty)
	{
	case WorldDifficulty::Peaceful: return "Peaceful";
	case WorldDifficulty::Easy: return "Easy";
	case WorldDifficulty::Normal: return "Normal";
	case WorldDifficulty::Hard: return "Hard";
	}
	return "Normal";
}

float getIncomingDamageMultiplier(const WorldDifficultySettings &settings)
{
	switch (settings.difficulty)
	{
	case WorldDifficulty::Peaceful: return 0.5f;
	case WorldDifficulty::Easy: return 0.75f;
	case WorldDifficulty::Hard: return 1.5f;
	case WorldDifficulty::Normal: return 1.f;
	}
	return 1.f;
}

float getSiegeEnemyMultiplier(const WorldDifficultySettings &settings)
{
	switch (settings.difficulty)
	{
	case WorldDifficulty::Peaceful: return 0.65f;
	case WorldDifficulty::Easy: return 0.75f;
	case WorldDifficulty::Hard: return 1.35f;
	case WorldDifficulty::Normal: return 1.f;
	}
	return 1.f;
}

int getStarvationHealthFloor(const WorldDifficultySettings &settings)
{
	switch (settings.difficulty)
	{
	case WorldDifficulty::Peaceful: return std::numeric_limits<int>::max();
	case WorldDifficulty::Easy: return 20;
	case WorldDifficulty::Normal: return 10;
	case WorldDifficulty::Hard: return 0;
	}
	return 10;
}

bool areNaturalSiegesEnabled(const WorldDifficultySettings &settings)
{
	return settings.difficulty != WorldDifficulty::Peaceful;
}

short scaleIncomingDamageForDifficulty(short difference,
	const WorldDifficultySettings &settings)
{
	if (difference >= 0) { return difference; }
	const int magnitude = -static_cast<int>(difference);
	const int scaled = std::clamp(static_cast<int>(std::lround(
		magnitude * getIncomingDamageMultiplier(settings))), 1,
		static_cast<int>(std::numeric_limits<short>::max()));
	return static_cast<short>(-scaled);
}

std::vector<unsigned char> formatWorldDifficultySettings(
	const WorldDifficultySettings &input)
{
	WorldDifficultySettings settings = input;
	settings.sanitize();
	std::vector<unsigned char> result(difficultyMagic.begin(), difficultyMagic.end());
	appendValue(result, WORLD_DIFFICULTY_FORMAT_VERSION);
	appendValue(result, static_cast<std::uint8_t>(settings.difficulty));
	appendValue(result, static_cast<std::uint8_t>(settings.hardcore ? 1u : 0u));
	return result;
}

bool parseWorldDifficultySettings(const char *data, std::size_t size,
	WorldDifficultySettings &settings)
{
	const std::size_t expectedSize = difficultyMagic.size() +
		sizeof(WORLD_DIFFICULTY_FORMAT_VERSION) + sizeof(std::uint8_t) * 2;
	if (!data || size != expectedSize || !std::equal(difficultyMagic.begin(),
		difficultyMagic.end(), reinterpret_cast<const unsigned char *>(data)))
	{
		return false;
	}

	std::size_t offset = difficultyMagic.size();
	std::uint32_t version = 0;
	std::uint8_t rawDifficulty = 0;
	std::uint8_t rawHardcore = 0;
	if (!readValue(data, size, offset, version) ||
		version != WORLD_DIFFICULTY_FORMAT_VERSION ||
		!readValue(data, size, offset, rawDifficulty) ||
		!readValue(data, size, offset, rawHardcore) || offset != size ||
		rawDifficulty > static_cast<std::uint8_t>(WorldDifficulty::Hard) ||
		rawHardcore > 1)
	{
		return false;
	}

	WorldDifficultySettings parsed;
	parsed.difficulty = static_cast<WorldDifficulty>(rawDifficulty);
	parsed.hardcore = rawHardcore != 0;
	parsed.sanitize();
	settings = parsed;
	return true;
}

bool saveWorldDifficultySettings(const std::filesystem::path &worldRoot,
	const WorldDifficultySettings &settings)
{
	const auto data = formatWorldDifficultySettings(settings);
	std::ofstream output(worldRoot / difficultyFileName,
		std::ios::binary | std::ios::trunc);
	if (!output) { return false; }
	output.write(reinterpret_cast<const char *>(data.data()),
		static_cast<std::streamsize>(data.size()));
	return static_cast<bool>(output);
}

bool loadWorldDifficultySettings(const std::filesystem::path &worldRoot,
	WorldDifficultySettings &settings)
{
	settings = {};
	std::ifstream input(worldRoot / difficultyFileName, std::ios::binary);
	if (!input) { return false; }
	std::vector<char> data((std::istreambuf_iterator<char>(input)),
		std::istreambuf_iterator<char>());
	return parseWorldDifficultySettings(data.data(), data.size(), settings);
}

void setServerWorldDifficultySettings(WorldDifficultySettings settings)
{
	settings.sanitize();
	serverSettings = settings;
}

const WorldDifficultySettings &getServerWorldDifficultySettings()
{
	return serverSettings;
}

void resetServerWorldDifficultySettings()
{
	serverSettings = {};
}

void setClientWorldDifficultySettings(WorldDifficultySettings settings)
{
	settings.sanitize();
	clientSettings = settings;
}

const WorldDifficultySettings &getClientWorldDifficultySettings()
{
	return clientSettings;
}

void resetClientWorldDifficultySettings()
{
	clientSettings = {};
}
