#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mie::native
{
	constexpr std::uint32_t WORLD_SCHEMA_MANIFEST_VERSION = 1;

	struct SchemaEntry
	{
		std::string subsystem;
		std::uint32_t version = 0;

		bool operator==(const SchemaEntry &other) const
		{
			return subsystem == other.subsystem && version == other.version;
		}
	};

	struct WorldSchemaManifest
	{
		std::uint32_t manifestVersion = WORLD_SCHEMA_MANIFEST_VERSION;
		std::vector<SchemaEntry> schemas;

		std::uint32_t versionOf(const std::string &subsystem) const;
		bool setVersion(const std::string &subsystem, std::uint32_t version);
	};

	WorldSchemaManifest makeV06WorldSchemaManifest();
	std::vector<unsigned char> formatWorldSchemaManifest(const WorldSchemaManifest &manifest);
	bool parseWorldSchemaManifest(const char *data, std::size_t size,
		WorldSchemaManifest &manifest);

	class MigrationRegistry
	{
	public:
		using Migration = std::function<bool(std::vector<unsigned char> &payload)>;

		bool registerMigration(const std::string &subsystem, std::uint32_t fromVersion,
			Migration migration);
		bool migrate(const std::string &subsystem, std::uint32_t &currentVersion,
			std::uint32_t targetVersion, std::vector<unsigned char> &payload) const;

	private:
		std::unordered_map<std::string, std::unordered_map<std::uint32_t, Migration>> migrations;
	};
}
