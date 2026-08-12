#include <multyPlayer/playerPersistence.h>

#include <gameplay/player.h>
#include <platformTools.h>
#include <safeSave.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <random>
#include <unordered_map>
#include <utility>

namespace
{
	constexpr std::array<unsigned char, 8> identityMagic = {
		'M', 'I', 'E', 'I', 'D', 'E', 'N', 0
	};

	// The recovered v0.6 source does not yet expose the v0.7 Field Guide runtime
	// on PlayerServer. Retain its persisted extension by stable identity so a
	// recovered/stabilized build never silently erases objective/reward progress.
	std::mutex retainedGuideStateMutex;
	std::unordered_map<std::string, PlayerGuideSaveState> retainedGuideState;

	std::string getLocalIdentitySavePath()
	{
		return std::string(USER_SETTINGS_PATH) + "playerIdentity";
	}

	std::string getPlayerSavePath(const std::string &worldSavePath, const PlayerIdentity &identity)
	{
		return (std::filesystem::path(worldSavePath) / "players" /
			("player_" + identity.toString())).string();
	}

	PlayerIdentity generatePlayerIdentity()
	{
		PlayerIdentity result;
		std::random_device randomDevice;
		std::seed_seq seed{
			randomDevice(), randomDevice(), randomDevice(), randomDevice(),
			static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
		};
		std::mt19937 generator(seed);
		std::uniform_int_distribution<unsigned int> distribution(0, 255);
		for (auto &value : result.bytes)
		{
			value = static_cast<unsigned char>(distribution(generator));
		}

		// Mark it as an RFC 4122 version-4/variant-1 UUID while keeping the wire
		// representation as the compact raw 128-bit value.
		result.bytes[6] = static_cast<unsigned char>((result.bytes[6] & 0x0F) | 0x40);
		result.bytes[8] = static_cast<unsigned char>((result.bytes[8] & 0x3F) | 0x80);
		return result;
	}

	std::vector<unsigned char> formatIdentity(const PlayerIdentity &identity)
	{
		std::vector<unsigned char> result;
		result.insert(result.end(), identityMagic.begin(), identityMagic.end());
		const auto version = PLAYER_IDENTITY_PROTOCOL_VERSION;
		const auto oldSize = result.size();
		result.resize(oldSize + sizeof(version));
		std::memcpy(result.data() + oldSize, &version, sizeof(version));
		result.insert(result.end(), identity.bytes.begin(), identity.bytes.end());
		return result;
	}

	bool parseIdentity(const std::vector<char> &data, PlayerIdentity &identity)
	{
		const auto expectedSize = identityMagic.size() + sizeof(std::uint32_t) + identity.bytes.size();
		if (data.size() != expectedSize ||
			!std::equal(identityMagic.begin(), identityMagic.end(), data.begin()))
		{
			return false;
		}

		std::uint32_t version = 0;
		std::memcpy(&version, data.data() + identityMagic.size(), sizeof(version));
		if (version != PLAYER_IDENTITY_PROTOCOL_VERSION) { return false; }

		std::copy_n(reinterpret_cast<const unsigned char *>(data.data()) +
			identityMagic.size() + sizeof(version), identity.bytes.size(), identity.bytes.begin());
		return identity.isValid();
	}

	void retainGuideState(const PlayerIdentity &identity, PlayerGuideSaveState state)
	{
		state.sanitize();
		std::lock_guard<std::mutex> lock(retainedGuideStateMutex);
		retainedGuideState[identity.toString()] = state;
	}

	PlayerGuideSaveState getRetainedGuideState(const PlayerIdentity &identity)
	{
		std::lock_guard<std::mutex> lock(retainedGuideStateMutex);
		const auto found = retainedGuideState.find(identity.toString());
		return found == retainedGuideState.end() ? PlayerGuideSaveState{} : found->second;
	}
}

PlayerIdentity loadOrCreateLocalPlayerIdentity()
{
	std::error_code directoryError;
	std::filesystem::create_directories(USER_SETTINGS_PATH, directoryError);

	PlayerIdentity identity;
	std::vector<char> savedData;
	if (!directoryError &&
		sfs::safeLoad(savedData, getLocalIdentitySavePath().c_str(), false) == sfs::noError &&
		parseIdentity(savedData, identity))
	{
		return identity;
	}

	identity = generatePlayerIdentity();
	const auto identityData = formatIdentity(identity);
	if (directoryError || sfs::safeSave(identityData.data(), identityData.size(),
		getLocalIdentitySavePath().c_str(), true) != sfs::noError)
	{
		std::cerr << "Warning: could not persist the local player identity.\n";
	}
	return identity;
}

bool loadPlayerFromDisk(const std::string &worldSavePath,
	const PlayerIdentity &identity, PlayerServer &player)
{
	if (!identity.isValid()) { return false; }

	std::vector<char> savedData;
	if (sfs::safeLoad(savedData, getPlayerSavePath(worldSavePath, identity).c_str(), false) != sfs::noError)
	{
		return false;
	}

	PlayerSaveSnapshot snapshot;
	if (!parsePlayerSaveSnapshot(savedData.data(), savedData.size(), snapshot) || snapshot.identity != identity)
	{
		std::cerr << "Warning: ignored an invalid player save for " << identity.toString() << ".\n";
		return false;
	}

	PlayerInventory inventory;
	std::size_t inventoryBytesRead = 0;
	if (!inventory.readFromData(snapshot.inventory.data(), snapshot.inventory.size(), &inventoryBytesRead) ||
		inventoryBytesRead != snapshot.inventory.size())
	{
		std::cerr << "Warning: ignored a player save with invalid inventory data.\n";
		return false;
	}
	inventory.sanitize();
	retainGuideState(identity, snapshot.guideState);

	player.entity.position = {snapshot.position[0], snapshot.position[1], snapshot.position[2]};
	player.entity.lastPosition = player.entity.position;
	player.entity.forces = {};
	player.inventory = std::move(inventory);
	const auto maxLife = std::max<std::int16_t>(snapshot.maxLife, 1);
	player.newLife = {maxLife};
	player.newLife.life = std::clamp<std::int16_t>(snapshot.life, 0, maxLife);
	player.newLife.sanitize();
	player.lifeLastFrame = player.newLife;
	player.survivalStats.hunger = snapshot.hunger;
	player.survivalStats.maxHunger = snapshot.maxHunger;
	player.survivalStats.sanitize();
	player.otherPlayerSettings.gameMode = snapshot.gameMode == OtherPlayerSettings::CREATIVE
		? OtherPlayerSettings::CREATIVE : OtherPlayerSettings::SURVIVAL;
	player.lastHungerPosition = player.entity.position;
	player.hungerPositionInitialized = true;
	return true;
}

bool savePlayerToDisk(const std::string &worldSavePath,
	const PlayerIdentity &identity, const PlayerServer &player)
{
	if (!identity.isValid()) { return false; }

	PlayerSaveSnapshot snapshot;
	snapshot.identity = identity;
	snapshot.position = {
		player.entity.position.x, player.entity.position.y, player.entity.position.z
	};
	snapshot.life = player.newLife.life;
	snapshot.maxLife = player.newLife.maxLife;
	snapshot.hunger = player.survivalStats.hunger;
	snapshot.maxHunger = player.survivalStats.maxHunger;
	snapshot.gameMode = player.otherPlayerSettings.gameMode;
	snapshot.guideState = getRetainedGuideState(identity);
	auto inventory = player.inventory;
	inventory.formatIntoData(snapshot.inventory);

	const auto data = formatPlayerSaveSnapshot(snapshot);
	if (data.empty()) { return false; }

	std::error_code directoryError;
	std::filesystem::create_directories(std::filesystem::path(worldSavePath) / "players", directoryError);
	if (directoryError) { return false; }

	return sfs::safeSave(data.data(), data.size(), getPlayerSavePath(worldSavePath, identity).c_str(), true)
		== sfs::noError;
}