#include <native/worldSchema.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <unordered_set>

namespace mie::native
{
	namespace
	{
		constexpr std::array<unsigned char, 8> manifestMagic = {
			'M', 'I', 'E', 'S', 'C', 'H', 'M', 'A'
		};
		constexpr std::size_t maxSchemas = 64;
		constexpr std::size_t maxSubsystemLength = 64;

		template<typename T>
		void append(std::vector<unsigned char> &data, const T &value)
		{
			const std::size_t oldSize = data.size();
			data.resize(oldSize + sizeof(value));
			std::memcpy(data.data() + oldSize, &value, sizeof(value));
		}

		template<typename T>
		bool read(const char *data, std::size_t size, std::size_t &offset, T &value)
		{
			if (!data || offset > size || sizeof(value) > size - offset) { return false; }
			std::memcpy(&value, data + offset, sizeof(value));
			offset += sizeof(value);
			return true;
		}

		bool validSubsystem(const std::string &subsystem)
		{
			return !subsystem.empty() && subsystem.size() <= maxSubsystemLength &&
				subsystem.find(':') != std::string::npos;
		}
	}

	std::uint32_t WorldSchemaManifest::versionOf(const std::string &subsystem) const
	{
		const auto found = std::find_if(schemas.begin(), schemas.end(),
			[&subsystem](const SchemaEntry &entry) { return entry.subsystem == subsystem; });
		return found == schemas.end() ? 0u : found->version;
	}

	bool WorldSchemaManifest::setVersion(const std::string &subsystem, std::uint32_t version)
	{
		if (!validSubsystem(subsystem) || version == 0) { return false; }
		for (SchemaEntry &entry : schemas)
		{
			if (entry.subsystem == subsystem)
			{
				entry.version = version;
				return true;
			}
		}
		if (schemas.size() >= maxSchemas) { return false; }
		schemas.push_back({subsystem, version});
		return true;
	}

	WorldSchemaManifest makeV06WorldSchemaManifest()
	{
		WorldSchemaManifest manifest;
		manifest.schemas = {
			{"mie:content_registry", 1},
			{"mie:player_snapshot", 1},
			{"mie:world_progress", 1},
			{"mie:prototype_machines", 1},
		};
		return manifest;
	}

	std::vector<unsigned char> formatWorldSchemaManifest(const WorldSchemaManifest &manifest)
	{
		if (manifest.manifestVersion != WORLD_SCHEMA_MANIFEST_VERSION ||
			manifest.schemas.size() > maxSchemas)
		{
			return {};
		}

		std::unordered_set<std::string> unique;
		std::vector<unsigned char> result(manifestMagic.begin(), manifestMagic.end());
		append(result, manifest.manifestVersion);
		const std::uint32_t count = static_cast<std::uint32_t>(manifest.schemas.size());
		append(result, count);
		for (const SchemaEntry &entry : manifest.schemas)
		{
			if (!validSubsystem(entry.subsystem) || entry.version == 0 ||
				!unique.insert(entry.subsystem).second)
			{
				return {};
			}
			const std::uint16_t length = static_cast<std::uint16_t>(entry.subsystem.size());
			append(result, length);
			result.insert(result.end(), entry.subsystem.begin(), entry.subsystem.end());
			append(result, entry.version);
		}
		return result;
	}

	bool parseWorldSchemaManifest(const char *data, std::size_t size,
		WorldSchemaManifest &manifest)
	{
		if (!data || size < manifestMagic.size() + sizeof(std::uint32_t) * 2 ||
			!std::equal(manifestMagic.begin(), manifestMagic.end(),
				reinterpret_cast<const unsigned char *>(data)))
		{
			return false;
		}

		std::size_t offset = manifestMagic.size();
		WorldSchemaManifest parsed;
		std::uint32_t count = 0;
		if (!read(data, size, offset, parsed.manifestVersion) ||
			parsed.manifestVersion != WORLD_SCHEMA_MANIFEST_VERSION ||
			!read(data, size, offset, count) || count > maxSchemas)
		{
			return false;
		}

		std::unordered_set<std::string> unique;
		parsed.schemas.reserve(count);
		for (std::uint32_t index = 0; index < count; ++index)
		{
			std::uint16_t length = 0;
			if (!read(data, size, offset, length) || length == 0 ||
				length > maxSubsystemLength || length > size - offset)
			{
				return false;
			}
			SchemaEntry entry;
			entry.subsystem.assign(data + offset, data + offset + length);
			offset += length;
			if (!read(data, size, offset, entry.version) || entry.version == 0 ||
				!validSubsystem(entry.subsystem) || !unique.insert(entry.subsystem).second)
			{
				return false;
			}
			parsed.schemas.push_back(std::move(entry));
		}
		if (offset != size) { return false; }
		manifest = std::move(parsed);
		return true;
	}

	bool MigrationRegistry::registerMigration(const std::string &subsystem,
		std::uint32_t fromVersion, Migration migration)
	{
		if (!validSubsystem(subsystem) || fromVersion == 0 || !migration) { return false; }
		return migrations[subsystem].emplace(fromVersion, std::move(migration)).second;
	}

	bool MigrationRegistry::migrate(const std::string &subsystem,
		std::uint32_t &currentVersion, std::uint32_t targetVersion,
		std::vector<unsigned char> &payload) const
	{
		if (currentVersion == 0 || targetVersion < currentVersion) { return false; }
		const auto subsystemMigrations = migrations.find(subsystem);
		std::uint32_t candidateVersion = currentVersion;
		std::vector<unsigned char> candidate = payload;
		while (candidateVersion < targetVersion)
		{
			if (subsystemMigrations == migrations.end()) { return false; }
			const auto migration = subsystemMigrations->second.find(candidateVersion);
			if (migration == subsystemMigrations->second.end()) { return false; }
			if (!migration->second(candidate)) { return false; }
			++candidateVersion;
		}
		payload.swap(candidate);
		currentVersion = candidateVersion;
		return true;
	}
}
