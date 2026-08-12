#include <gameplay/worldTime.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace
{
	constexpr std::array<unsigned char, 8> worldProgressMagic = {
		'M', 'I', 'E', 'W', 'T', 'I', 'M', 'E'
	};

	struct ClientWorldTime
	{
		float dayPhase = 0.25f;
		std::uint64_t completedCycles = 0;
		bool advancing = false;
	};

	ClientWorldTime clientWorldTime;

	template <typename T>
	void appendValue(std::vector<unsigned char> &data, const T &value)
	{
		const auto oldSize = data.size();
		data.resize(oldSize + sizeof(value));
		std::memcpy(data.data() + oldSize, &value, sizeof(value));
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

WorldCycleClock::WorldCycleClock(double cycleDurationSeconds):
	cycleDurationSeconds(std::max(cycleDurationSeconds, 1.0))
{
	reset();
}

void WorldCycleClock::reset()
{
	cycleProgressSeconds = 0.0;
	completedCycles = 0;
}

void WorldCycleClock::restore(double newCycleProgressSeconds,
	std::uint64_t newCompletedCycles)
{
	if (!std::isfinite(newCycleProgressSeconds) || newCycleProgressSeconds < 0.0)
	{
		newCycleProgressSeconds = 0.0;
	}

	const double wrappedProgress = std::fmod(newCycleProgressSeconds, cycleDurationSeconds);
	cycleProgressSeconds = wrappedProgress < 0.0 ? wrappedProgress + cycleDurationSeconds : wrappedProgress;
	completedCycles = newCompletedCycles;
}

std::uint64_t WorldCycleClock::update(double deltaTime, bool advancing)
{
	if (!advancing || !std::isfinite(deltaTime) || deltaTime <= 0.0) { return 0; }

	const double total = cycleProgressSeconds + deltaTime;
	const double completed = std::floor(total / cycleDurationSeconds);
	const std::uint64_t advancedCycles = completed >= static_cast<double>(
		std::numeric_limits<std::uint64_t>::max()) ? std::numeric_limits<std::uint64_t>::max() :
		static_cast<std::uint64_t>(completed);
	cycleProgressSeconds = std::fmod(total, cycleDurationSeconds);
	if (cycleProgressSeconds < 0.0) { cycleProgressSeconds += cycleDurationSeconds; }

	if (advancedCycles > std::numeric_limits<std::uint64_t>::max() - completedCycles)
	{
		completedCycles = std::numeric_limits<std::uint64_t>::max();
	}
	else
	{
		completedCycles += advancedCycles;
	}
	return advancedCycles;
}

float WorldCycleClock::getDayPhase() const
{
	const double phase = cycleProgressSeconds / cycleDurationSeconds + 0.25;
	return static_cast<float>(phase - std::floor(phase));
}

std::vector<unsigned char> formatWorldProgressSnapshot(const WorldProgressSnapshot &snapshot)
{
	std::vector<unsigned char> result;
	result.reserve(worldProgressMagic.size() + sizeof(WORLD_PROGRESS_FORMAT_VERSION) +
		sizeof(snapshot.cycleProgressSeconds) + sizeof(snapshot.completedCycles) +
		sizeof(snapshot.nextSiegeCycle) + sizeof(snapshot.completedSieges));
	result.insert(result.end(), worldProgressMagic.begin(), worldProgressMagic.end());
	appendValue(result, WORLD_PROGRESS_FORMAT_VERSION);
	appendValue(result, snapshot.cycleProgressSeconds);
	appendValue(result, snapshot.completedCycles);
	appendValue(result, snapshot.nextSiegeCycle);
	appendValue(result, snapshot.completedSieges);
	return result;
}

bool parseWorldProgressSnapshot(const char *data, std::size_t size,
	WorldProgressSnapshot &snapshot)
{
	const std::size_t expectedSize = worldProgressMagic.size() +
		sizeof(WORLD_PROGRESS_FORMAT_VERSION) + sizeof(snapshot.cycleProgressSeconds) +
		sizeof(snapshot.completedCycles) + sizeof(snapshot.nextSiegeCycle) +
		sizeof(snapshot.completedSieges);
	if (!data || size != expectedSize || !std::equal(worldProgressMagic.begin(),
		worldProgressMagic.end(), reinterpret_cast<const unsigned char *>(data)))
	{
		return false;
	}

	std::size_t offset = worldProgressMagic.size();
	std::uint32_t version = 0;
	WorldProgressSnapshot parsed;
	if (!readValue(data, size, offset, version) || version != WORLD_PROGRESS_FORMAT_VERSION ||
		!readValue(data, size, offset, parsed.cycleProgressSeconds) ||
		!readValue(data, size, offset, parsed.completedCycles) ||
		!readValue(data, size, offset, parsed.nextSiegeCycle) ||
		!readValue(data, size, offset, parsed.completedSieges) || offset != size ||
		!std::isfinite(parsed.cycleProgressSeconds) || parsed.cycleProgressSeconds < 0.0 ||
		parsed.cycleProgressSeconds >= DEFAULT_WORLD_CYCLE_SECONDS ||
		parsed.nextSiegeCycle == 0)
	{
		return false;
	}

	snapshot = parsed;
	return true;
}

void setClientWorldTime(float dayPhase, std::uint64_t completedCycles, bool advancing)
{
	if (!std::isfinite(dayPhase)) { dayPhase = 0.25f; }
	clientWorldTime.dayPhase = dayPhase - std::floor(dayPhase);
	clientWorldTime.completedCycles = completedCycles;
	clientWorldTime.advancing = advancing;
}

void updateClientWorldTime(float deltaTime)
{
	if (!clientWorldTime.advancing || !std::isfinite(deltaTime) || deltaTime <= 0.f) { return; }
	clientWorldTime.dayPhase += static_cast<float>(deltaTime / DEFAULT_WORLD_CYCLE_SECONDS);
	if (clientWorldTime.dayPhase >= 1.f)
	{
		const float completed = std::floor(clientWorldTime.dayPhase);
		clientWorldTime.dayPhase -= completed;
		clientWorldTime.completedCycles += static_cast<std::uint64_t>(completed);
	}
}

float getClientWorldDayPhase()
{
	return clientWorldTime.dayPhase;
}

std::uint64_t getClientCompletedWorldCycles()
{
	return clientWorldTime.completedCycles;
}

void resetClientWorldTime()
{
	clientWorldTime = {};
}
