#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct WorldCatalogEntry
{
	std::string folderName;
	std::int64_t seed = 0;
	bool hasSeed = false;
	bool hasGeneratedWorld = false;
	std::filesystem::file_time_type lastModified = {};
};

// Returns only direct child directories of worldsRoot. Invalid/inaccessible
// entries are skipped and never escape the configured saves directory.
std::vector<WorldCatalogEntry> loadWorldCatalog(
	const std::filesystem::path &worldsRoot, std::string *errorMessage = nullptr);

// World folder names are deliberately conservative because they are also
// sent to the local server and used to construct save paths.
bool normalizeWorldName(const std::string &input, std::string &normalized,
	std::string *errorMessage = nullptr);

bool markWorldPlayed(const std::filesystem::path &worldsRoot,
	const std::string &folderName, std::string *errorMessage = nullptr);

int worldSeedFromText(const std::string &seedText, int fallbackSeed);
