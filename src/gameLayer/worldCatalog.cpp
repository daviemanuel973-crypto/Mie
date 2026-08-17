#include <worldCatalog.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <climits>
#include <fstream>
#include <limits>
#include <sstream>

namespace
{
	bool readSeedValue(const std::filesystem::path &worldPath, std::int64_t &seed)
	{
		std::ifstream legacySeed(worldPath / "seed.txt");
		if (legacySeed && (legacySeed >> seed)) { return true; }

		std::ifstream settings(worldPath / "worldGenSettings.wgenerator");
		if (!settings) { return false; }

		std::string line;
		while (std::getline(settings, line))
		{
			auto key = line.find("seed:");
			if (key == std::string::npos) { continue; }

			std::istringstream value(line.substr(key + 5));
			if (value >> seed) { return true; }
		}

		return false;
	}

}

std::vector<WorldCatalogEntry> loadWorldCatalog(
	const std::filesystem::path &worldsRoot, std::string *errorMessage)
{
	std::vector<WorldCatalogEntry> result;
	if (errorMessage) { errorMessage->clear(); }

	std::error_code error;
	std::filesystem::create_directories(worldsRoot, error);
	if (error)
	{
		if (errorMessage) { *errorMessage = "Could not open the worlds folder: " + error.message(); }
		return result;
	}

	for (std::filesystem::directory_iterator it(
		worldsRoot, std::filesystem::directory_options::skip_permission_denied, error), end;
		it != end && !error; it.increment(error))
	{
		std::error_code entryError;
		if (!it->is_directory(entryError) || entryError) { continue; }

		WorldCatalogEntry entry;
		entry.folderName = it->path().filename().string();
		if (entry.folderName.empty() || entry.folderName == "." || entry.folderName == "..") { continue; }

		entry.hasSeed = readSeedValue(it->path(), entry.seed);
		entry.hasExplicitDifficulty = loadWorldDifficultySettings(
			it->path(), entry.difficultySettings);
		entry.hasGeneratedWorld = std::filesystem::is_directory(it->path() / "world", entryError);
		entryError.clear();
		const auto lastPlayed = it->path() / ".last_played";
		entry.lastModified = std::filesystem::last_write_time(
			std::filesystem::exists(lastPlayed, entryError) ? lastPlayed : it->path(), entryError);
		result.push_back(std::move(entry));
	}

	if (error && errorMessage)
	{
		*errorMessage = "Some worlds could not be read: " + error.message();
	}

	std::sort(result.begin(), result.end(), [](const auto &a, const auto &b)
	{
		if (a.lastModified != b.lastModified) { return a.lastModified > b.lastModified; }
		return a.folderName < b.folderName;
	});

	return result;
}

bool normalizeWorldName(const std::string &input, std::string &normalized,
	std::string *errorMessage)
{
	if (errorMessage) { errorMessage->clear(); }
	normalized = input;

	auto notSpace = [](unsigned char c) { return !std::isspace(c); };
	auto first = std::find_if(normalized.begin(), normalized.end(), notSpace);
	auto last = std::find_if(normalized.rbegin(), normalized.rend(), notSpace).base();
	if (first >= last)
	{
		normalized.clear();
		if (errorMessage) { *errorMessage = "Please enter a world name."; }
		return false;
	}
	normalized = std::string(first, last);

	if (normalized.size() > 48)
	{
		if (errorMessage) { *errorMessage = "World names can contain at most 48 characters."; }
		return false;
	}

	if (normalized == "." || normalized == ".." ||
		normalized.back() == '.' || normalized.find_first_of("/\\:*?\"<>|") != std::string::npos)
	{
		if (errorMessage) { *errorMessage = "The world name contains an invalid character."; }
		return false;
	}

	std::string windowsStem = normalized.substr(0, normalized.find('.'));
	std::transform(windowsStem.begin(), windowsStem.end(), windowsStem.begin(),
		[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	const bool reservedWindowsName = windowsStem == "CON" || windowsStem == "PRN" ||
		windowsStem == "AUX" || windowsStem == "NUL" ||
		(windowsStem.size() == 4 &&
			(windowsStem.rfind("COM", 0) == 0 || windowsStem.rfind("LPT", 0) == 0) &&
			windowsStem[3] >= '1' && windowsStem[3] <= '9');
	if (reservedWindowsName)
	{
		if (errorMessage) { *errorMessage = "That name is reserved by Windows."; }
		return false;
	}

	for (unsigned char c : normalized)
	{
		if (c < 32)
		{
			if (errorMessage) { *errorMessage = "The world name contains a control character."; }
			return false;
		}
	}

	return true;
}

bool markWorldPlayed(const std::filesystem::path &worldsRoot,
	const std::string &folderName, std::string *errorMessage)
{
	std::string normalized;
	if (!normalizeWorldName(folderName, normalized, errorMessage) || normalized != folderName)
	{
		if (errorMessage && errorMessage->empty()) { *errorMessage = "The selected world name is invalid."; }
		return false;
	}

	const auto worldPath = worldsRoot / normalized;
	std::error_code error;
	if (!std::filesystem::is_directory(worldPath, error) || error)
	{
		if (errorMessage) { *errorMessage = "The selected world folder no longer exists."; }
		return false;
	}

	std::ofstream marker(worldPath / ".last_played", std::ios::trunc);
	if (!marker)
	{
		if (errorMessage) { *errorMessage = "Could not update the world's last-played time."; }
		return false;
	}
	marker << "Mie world last played marker\n";
	return static_cast<bool>(marker);
}

int worldSeedFromText(const std::string &seedText, int fallbackSeed)
{
	if (seedText.empty())
	{
		if (fallbackSeed == std::numeric_limits<int>::min()) { return 1; }
		fallbackSeed = std::abs(fallbackSeed);
		return fallbackSeed == 0 ? 1 : fallbackSeed;
	}

	long long numericSeed = 0;
	const auto numeric = std::from_chars(seedText.data(), seedText.data() + seedText.size(), numericSeed);
	if (numeric.ec == std::errc{} && numeric.ptr == seedText.data() + seedText.size())
	{
		const auto positive = numericSeed < 0 ?
			0ull - static_cast<unsigned long long>(numericSeed) :
			static_cast<unsigned long long>(numericSeed);
		const auto reduced = positive % INT_MAX;
		return reduced == 0 ? 1 : static_cast<int>(reduced);
	}

	// Stable FNV-1a hash allows readable text seeds without depending on the
	// implementation-defined result of std::hash.
	std::uint32_t hash = 2166136261u;
	for (unsigned char c : seedText)
	{
		hash ^= c;
		hash *= 16777619u;
	}
	return static_cast<int>((hash % static_cast<std::uint32_t>(INT_MAX)) + 1u);
}
