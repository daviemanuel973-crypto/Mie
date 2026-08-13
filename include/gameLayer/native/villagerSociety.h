#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <native/gameplayScheduler.h>

namespace mie::native
{
	constexpr std::size_t MAX_VILLAGERS = 2048;
	constexpr std::size_t MAX_VILLAGER_REPUTATIONS = 32;
	constexpr std::size_t MAX_VILLAGER_NAME_LENGTH = 32;
	constexpr std::uint32_t VILLAGER_SOCIETY_SNAPSHOT_VERSION = 1;

	// The shipped v0.7 runtime persisted only three profession values. Keep the
	// enum compact/stable so future jobs can extend it without renumbering these.
	enum class VillagerProfession : std::uint8_t
	{
		Farmer = 0,
		Miner = 1,
		Smith = 2,
	};

	// v0.7 persisted a 1..5 life-stage byte. The society simulation is abstract
	// for now, but preserving the range keeps snapshots forward-compatible.
	enum class VillagerLifeStage : std::uint8_t
	{
		Child = 1,
		Teen = 2,
		YoungAdult = 3,
		Adult = 4,
		Elder = 5,
	};

	enum class VillagerActivity : std::uint8_t
	{
		Idle = 0,
		Working = 1,
		Socializing = 2,
		Sleeping = 3,
	};

	struct VillagerReputation
	{
		std::uint64_t playerId = 0;
		std::int16_t value = 0;
	};

	struct VillagerProfile
	{
		std::uint64_t id = 0;
		std::string name;
		std::uint32_t settlementId = 0;
		VillagerProfession profession = VillagerProfession::Farmer;
		VillagerLifeStage lifeStage = VillagerLifeStage::Adult;
		std::array<int, 3> homePosition{};
		std::array<int, 3> workPosition{};
		VillagerActivity activity = VillagerActivity::Idle;
		std::uint8_t mood = 100;
		std::uint8_t energy = 100;
		std::uint8_t hunger = 100;
		SimulationLevel simulationLevel = SimulationLevel::Unloaded;
		std::vector<VillagerReputation> reputations;
		std::uint64_t nextUpdateTick = 0;

		bool valid() const
		{
			const auto professionValue = static_cast<unsigned int>(profession);
			const auto stageValue = static_cast<unsigned int>(lifeStage);
			const auto activityValue = static_cast<unsigned int>(activity);
			const auto simulationValue = static_cast<unsigned int>(simulationLevel);
			if (id == 0 || name.empty() || name.size() > MAX_VILLAGER_NAME_LENGTH ||
				professionValue > 2u || stageValue < 1u || stageValue > 5u ||
				activityValue > 3u || simulationValue > 3u ||
				reputations.size() > MAX_VILLAGER_REPUTATIONS ||
				mood > 100u || energy > 100u || hunger > 100u)
			{
				return false;
			}
			for (const VillagerReputation &entry : reputations)
			{
				if (entry.playerId == 0 || entry.value < -100 || entry.value > 100)
				{
					return false;
				}
			}
			return true;
		}
	};

	struct VillagerSocietyMetrics
	{
		std::uint64_t villagers = 0;
		std::uint64_t updatesExecuted = 0;
		std::uint64_t updatesDeferred = 0;
	};

	class VillagerSocietyRuntime
	{
	public:
		bool createVillager(std::uint64_t id, std::uint32_t settlementId,
			VillagerProfession profession, std::array<int, 3> homePosition,
			std::array<int, 3> workPosition, std::string name)
		{
			if (id == 0 || villagers.size() >= MAX_VILLAGERS || villagers.find(id) != villagers.end())
			{
				return false;
			}
			if (name.empty())
			{
				static constexpr std::array<const char *, 12> generatedNames = {
					"Ari", "Bela", "Caio", "Dara", "Enzo", "Fara",
					"Gabi", "Hugo", "Iara", "Joca", "Lina", "Miro",
				};
				name = generatedNames[static_cast<std::size_t>(settlementId % generatedNames.size())];
			}

			VillagerProfile profile;
			profile.id = id;
			profile.name = std::move(name);
			profile.settlementId = settlementId;
			profile.profession = profession;
			profile.lifeStage = VillagerLifeStage::Adult;
			profile.homePosition = homePosition;
			profile.workPosition = workPosition;
			profile.activity = VillagerActivity::Idle;
			profile.mood = 100;
			profile.energy = 100;
			profile.hunger = 100;
			profile.simulationLevel = SimulationLevel::Unloaded;
			if (!profile.valid()) { return false; }
			villagers.emplace(id, std::move(profile));
			persistenceDirty = true;
			return true;
		}

		VillagerProfile *find(std::uint64_t id)
		{
			auto found = villagers.find(id);
			return found == villagers.end() ? nullptr : &found->second;
		}

		const VillagerProfile *find(std::uint64_t id) const
		{
			auto found = villagers.find(id);
			return found == villagers.end() ? nullptr : &found->second;
		}

		bool setSimulationLevel(std::uint64_t id, SimulationLevel level)
		{
			VillagerProfile *profile = find(id);
			if (!profile || static_cast<unsigned int>(level) > 3u) { return false; }
			if (profile->simulationLevel == level) { return true; }
			profile->simulationLevel = level;
			persistenceDirty = true;
			return true;
		}

		bool changeReputation(std::uint64_t villagerId, std::uint64_t playerId, int delta)
		{
			VillagerProfile *profile = find(villagerId);
			if (!profile || playerId == 0 || delta == 0) { return false; }
			for (VillagerReputation &entry : profile->reputations)
			{
				if (entry.playerId == playerId)
				{
					entry.value = static_cast<std::int16_t>(std::clamp<int>(entry.value + delta, -100, 100));
					persistenceDirty = true;
					return true;
				}
			}
			if (profile->reputations.size() >= MAX_VILLAGER_REPUTATIONS) { return false; }
			profile->reputations.push_back({playerId,
				static_cast<std::int16_t>(std::clamp(delta, -100, 100))});
			persistenceDirty = true;
			return true;
		}

		int reputationFor(std::uint64_t villagerId, std::uint64_t playerId) const
		{
			const VillagerProfile *profile = find(villagerId);
			if (!profile || playerId == 0) { return 0; }
			for (const VillagerReputation &entry : profile->reputations)
			{
				if (entry.playerId == playerId) { return entry.value; }
			}
			return 0;
		}

		void update(std::uint64_t currentTick, std::uint64_t budget)
		{
			std::uint64_t executed = 0;
			const std::uint64_t timeOfDay = currentTick % 24'000u;
			const bool workHours = timeOfDay >= 6'000u && timeOfDay < 18'000u;
			for (auto &entry : villagers)
			{
				VillagerProfile &profile = entry.second;
				if (profile.simulationLevel == SimulationLevel::Unloaded ||
					profile.nextUpdateTick > currentTick)
				{
					continue;
				}
				if (executed >= budget)
				{
					++runtimeMetrics.updatesDeferred;
					continue;
				}

				if (workHours)
				{
					profile.activity = VillagerActivity::Working;
					profile.energy = profile.energy > 0 ? static_cast<std::uint8_t>(profile.energy - 1) : 0;
				}
				else
				{
					profile.activity = VillagerActivity::Idle;
					profile.energy = static_cast<std::uint8_t>(std::min<int>(100, profile.energy + 2));
				}
				profile.nextUpdateTick = currentTick +
					20u * simulationIntervalMultiplier(profile.simulationLevel);
				++executed;
				++runtimeMetrics.updatesExecuted;
				persistenceDirty = true;
			}
		}

		std::vector<unsigned char> formatSnapshot() const
		{
			if (villagers.size() > MAX_VILLAGERS) { return {}; }
			std::vector<std::uint64_t> ids;
			ids.reserve(villagers.size());
			for (const auto &entry : villagers)
			{
				if (!entry.second.valid()) { return {}; }
				ids.push_back(entry.first);
			}
			std::sort(ids.begin(), ids.end());

			std::vector<unsigned char> data;
			static constexpr std::array<unsigned char, 8> magic = {
				'M', 'I', 'E', 'V', 'I', 'L', 'L', 0,
			};
			data.insert(data.end(), magic.begin(), magic.end());
			append(data, VILLAGER_SOCIETY_SNAPSHOT_VERSION);
			append(data, static_cast<std::uint32_t>(ids.size()));
			for (std::uint64_t id : ids)
			{
				const VillagerProfile &profile = villagers.at(id);
				append(data, profile.id);
				append(data, profile.settlementId);
				append(data, static_cast<std::uint8_t>(profile.profession));
				append(data, static_cast<std::uint8_t>(profile.lifeStage));
				for (int value : profile.homePosition) { append(data, static_cast<std::int32_t>(value)); }
				for (int value : profile.workPosition) { append(data, static_cast<std::int32_t>(value)); }
				append(data, static_cast<std::uint8_t>(profile.activity));
				append(data, profile.mood);
				append(data, profile.energy);
				append(data, profile.hunger);
				append(data, static_cast<std::uint8_t>(profile.simulationLevel));
				append(data, profile.nextUpdateTick);
				append(data, static_cast<std::uint8_t>(profile.name.size()));
				data.insert(data.end(), profile.name.begin(), profile.name.end());
				append(data, static_cast<std::uint8_t>(profile.reputations.size()));
				for (const VillagerReputation &reputation : profile.reputations)
				{
					append(data, reputation.playerId);
					append(data, reputation.value);
				}
			}
			return data;
		}

		bool restoreSnapshot(const char *bytes, std::uint64_t size)
		{
			static constexpr std::array<unsigned char, 8> magic = {
				'M', 'I', 'E', 'V', 'I', 'L', 'L', 0,
			};
			if (!bytes || size < magic.size() + sizeof(std::uint32_t) * 2 ||
				std::memcmp(bytes, magic.data(), magic.size()) != 0)
			{
				return false;
			}
			std::size_t offset = magic.size();
			std::uint32_t version = 0;
			std::uint32_t count = 0;
			if (!read(bytes, size, offset, version) || version != VILLAGER_SOCIETY_SNAPSHOT_VERSION ||
				!read(bytes, size, offset, count) || count > MAX_VILLAGERS)
			{
				return false;
			}

			std::unordered_map<std::uint64_t, VillagerProfile> candidate;
			candidate.reserve(count);
			for (std::uint32_t index = 0; index < count; ++index)
			{
				VillagerProfile profile;
				std::uint8_t profession = 0;
				std::uint8_t lifeStage = 0;
				std::uint8_t activity = 0;
				std::uint8_t simulation = 0;
				std::uint8_t nameLength = 0;
				std::uint8_t reputationCount = 0;
				if (!read(bytes, size, offset, profile.id) ||
					!read(bytes, size, offset, profile.settlementId) ||
					!read(bytes, size, offset, profession) ||
					!read(bytes, size, offset, lifeStage))
				{
					return false;
				}
				for (int &value : profile.homePosition)
				{
					std::int32_t parsed = 0;
					if (!read(bytes, size, offset, parsed)) { return false; }
					value = parsed;
				}
				for (int &value : profile.workPosition)
				{
					std::int32_t parsed = 0;
					if (!read(bytes, size, offset, parsed)) { return false; }
					value = parsed;
				}
				if (!read(bytes, size, offset, activity) ||
					!read(bytes, size, offset, profile.mood) ||
					!read(bytes, size, offset, profile.energy) ||
					!read(bytes, size, offset, profile.hunger) ||
					!read(bytes, size, offset, simulation) ||
					!read(bytes, size, offset, profile.nextUpdateTick) ||
					!read(bytes, size, offset, nameLength) || nameLength == 0 ||
					nameLength > MAX_VILLAGER_NAME_LENGTH || nameLength > size - offset)
				{
					return false;
				}
				profile.profession = static_cast<VillagerProfession>(profession);
				profile.lifeStage = static_cast<VillagerLifeStage>(lifeStage);
				profile.activity = static_cast<VillagerActivity>(activity);
				profile.simulationLevel = static_cast<SimulationLevel>(simulation);
				profile.name.assign(bytes + offset, bytes + offset + nameLength);
				offset += nameLength;
				if (!read(bytes, size, offset, reputationCount) ||
					reputationCount > MAX_VILLAGER_REPUTATIONS)
				{
					return false;
				}
				profile.reputations.reserve(reputationCount);
				for (std::uint8_t reputationIndex = 0; reputationIndex < reputationCount; ++reputationIndex)
				{
					VillagerReputation reputation;
					if (!read(bytes, size, offset, reputation.playerId) ||
						!read(bytes, size, offset, reputation.value))
					{
						return false;
					}
					profile.reputations.push_back(reputation);
				}
				if (!profile.valid() || !candidate.emplace(profile.id, std::move(profile)).second)
				{
					return false;
				}
			}
			if (offset != size) { return false; }
			villagers.swap(candidate);
			persistenceDirty = false;
			return true;
		}

		VillagerSocietyMetrics metrics() const
		{
			VillagerSocietyMetrics result = runtimeMetrics;
			result.villagers = static_cast<std::uint64_t>(villagers.size());
			return result;
		}

		const std::unordered_map<std::uint64_t, VillagerProfile> &allVillagers() const
		{
			return villagers;
		}

		bool hasPersistenceChanges() const { return persistenceDirty; }
		void acknowledgePersisted() { persistenceDirty = false; }
		void clear()
		{
			villagers.clear();
			runtimeMetrics = {};
			persistenceDirty = true;
		}

	private:
		template <class T>
		static void append(std::vector<unsigned char> &data, const T &value)
		{
			static_assert(std::is_trivially_copyable<T>::value,
				"Villager snapshot primitives must be trivially copyable");
			const std::size_t oldSize = data.size();
			data.resize(oldSize + sizeof(T));
			std::memcpy(data.data() + oldSize, &value, sizeof(T));
		}

		template <class T>
		static bool read(const char *data, std::uint64_t size, std::size_t &offset, T &value)
		{
			static_assert(std::is_trivially_copyable<T>::value,
				"Villager snapshot primitives must be trivially copyable");
			if (!data || offset > size || sizeof(T) > size - offset) { return false; }
			std::memcpy(&value, data + offset, sizeof(T));
			offset += sizeof(T);
			return true;
		}

		std::unordered_map<std::uint64_t, VillagerProfile> villagers;
		bool persistenceDirty = false;
		VillagerSocietyMetrics runtimeMetrics;
	};
}
