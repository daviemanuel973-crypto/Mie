#include <worldCatalog.h>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

int main()
{
	std::string normalized;
	std::string error;
	assert(normalizeWorldName("  My World  ", normalized, &error));
	assert(normalized == "My World");
	assert(!normalizeWorldName("../escape", normalized, &error));
	assert(!normalizeWorldName("bad/world", normalized, &error));
	assert(!normalizeWorldName("CON", normalized, &error));
	assert(!normalizeWorldName("con.txt", normalized, &error));
	assert(!normalizeWorldName("world.", normalized, &error));
	assert(!normalizeWorldName("   ", normalized, &error));

	const auto root = std::filesystem::temp_directory_path() / "mie-world-catalog-test";
	std::error_code filesystemError;
	std::filesystem::remove_all(root, filesystemError);
	std::filesystem::create_directories(root / "Older World" / "world", filesystemError);
	assert(!filesystemError);
	{
		std::ofstream settings(root / "Older World" / "worldGenSettings.wgenerator");
		settings << "seed: 12345;\n";
	}
	std::filesystem::create_directories(root / "New World", filesystemError);
	{
		std::ofstream seed(root / "New World" / "seed.txt");
		seed << "9876";
	}
	const auto now = std::filesystem::file_time_type::clock::now();
	std::filesystem::last_write_time(root / "Older World", now - std::chrono::hours(2), filesystemError);
	std::filesystem::last_write_time(root / "New World", now - std::chrono::hours(1), filesystemError);
	assert(!filesystemError);

	auto worlds = loadWorldCatalog(root, &error);
	assert(error.empty());
	assert(worlds.size() == 2);
	assert(worlds.front().folderName == "New World");

	auto findWorld = [&](const std::string &name) -> const WorldCatalogEntry*
	{
		for (const auto &world : worlds)
		{
			if (world.folderName == name) { return &world; }
		}
		return nullptr;
	};

	const auto *older = findWorld("Older World");
	const auto *newer = findWorld("New World");
	assert(older && newer);
	assert(older->hasSeed && older->seed == 12345);
	assert(older->hasGeneratedWorld);
	assert(newer->hasSeed && newer->seed == 9876);
	assert(markWorldPlayed(root, "Older World", &error));
	assert(std::filesystem::exists(root / "Older World" / ".last_played"));
	worlds = loadWorldCatalog(root, &error);
	assert(error.empty());
	assert(worlds.front().folderName == "Older World");
	assert(!markWorldPlayed(root, "../escape", &error));
	assert(worldSeedFromText("12345", 7) == 12345);
	assert(worldSeedFromText("", 42) == 42);
	assert(worldSeedFromText("Mie", 42) == worldSeedFromText("Mie", 99));
	assert(worldSeedFromText("Mie", 42) > 0);

	std::filesystem::remove_all(root, filesystemError);
	return 0;
}
