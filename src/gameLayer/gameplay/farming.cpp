#include <gameplay/farming.h>

#include <gameplay/items.h>
#include <safeSave.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <unordered_map>

namespace
{
	constexpr std::array<unsigned char, 8> farmMagic = {'M','I','E','F','A','R','M',0};
	constexpr std::uint32_t farmFormatVersion = 1;
	constexpr std::uint32_t maxFarmPlots = 250'000;

	struct Ivec3Hash
	{
		std::size_t operator()(const glm::ivec3 &value) const noexcept
		{
			std::size_t seed = static_cast<std::size_t>(value.x) * 73856093u;
			seed ^= static_cast<std::size_t>(value.y) * 19349663u;
			seed ^= static_cast<std::size_t>(value.z) * 83492791u;
			return seed;
		}
	};

	std::string cachedWorldPath;
	bool cacheLoaded = false;
	std::unordered_map<glm::ivec3, FarmPlotState, Ivec3Hash> cachedPlots;

	template <class T>
	void appendValue(std::vector<unsigned char> &data, const T &value)
	{
		const auto start = data.size();
		data.resize(start + sizeof(T));
		std::memcpy(data.data() + start, &value, sizeof(T));
	}

	class Reader
	{
	public:
		Reader(const void *data, std::size_t size)
			: data(static_cast<const unsigned char *>(data)), size(size) {}

		template <class T>
		bool read(T &value)
		{
			if (!data || position > size || sizeof(T) > size - position) { return false; }
			std::memcpy(&value, data + position, sizeof(T));
			position += sizeof(T);
			return true;
		}

		bool readBytes(void *target, std::size_t count)
		{
			if (!data || position > size || count > size - position) { return false; }
			if (count) { std::memcpy(target, data + position, count); }
			position += count;
			return true;
		}

		std::size_t remaining() const { return position <= size ? size - position : 0; }

	private:
		const unsigned char *data = nullptr;
		std::size_t size = 0;
		std::size_t position = 0;
	};

	bool validPosition(const glm::ivec3 &position)
	{
		return std::abs(static_cast<long long>(position.x)) <= 30'000'000LL &&
			position.y >= 0 && position.y < 256 &&
			std::abs(static_cast<long long>(position.z)) <= 30'000'000LL;
	}

	bool validPlot(const FarmPlotState &plot)
	{
		return validPosition(plot.position) &&
			static_cast<unsigned int>(plot.crop) < static_cast<unsigned int>(FarmCrop::Count) &&
			std::isfinite(plot.plantedWorldSeconds) && plot.plantedWorldSeconds >= 0.0 &&
			plot.plantedWorldSeconds < 1.0e15;
	}

	std::string farmSavePath(const std::string &worldSavePath)
	{
		return (std::filesystem::path(worldSavePath) / "farmPlots").string();
	}

	bool ensureLoaded(const std::string &worldSavePath)
	{
		if (cacheLoaded && cachedWorldPath == worldSavePath) { return true; }
		cachedWorldPath = worldSavePath;
		cacheLoaded = false;
		cachedPlots.clear();

		const std::string path = farmSavePath(worldSavePath);
		// safeSave treats this as a base name and persists two checksum-protected
		// copies as <base>1.bin and <base>2.bin. Checking only the base path makes
		// every fresh process incorrectly look like it has no farming data.
		if (!std::filesystem::exists(path + "1.bin") &&
			!std::filesystem::exists(path + "2.bin"))
		{
			cacheLoaded = true;
			return true;
		}

		std::vector<char> data;
		if (sfs::safeLoad(data, path.c_str(), false) != sfs::noError) { return false; }
		std::vector<FarmPlotState> parsed;
		if (!parseFarmPlots(data.data(), data.size(), parsed)) { return false; }
		for (const auto &plot : parsed) { cachedPlots.emplace(plot.position, plot); }
		cacheLoaded = true;
		return true;
	}

	bool saveCache()
	{
		if (!cacheLoaded || cachedWorldPath.empty()) { return false; }
		std::vector<FarmPlotState> plots;
		plots.reserve(cachedPlots.size());
		for (const auto &entry : cachedPlots) { plots.push_back(entry.second); }
		std::sort(plots.begin(), plots.end(), [](const FarmPlotState &a, const FarmPlotState &b)
		{
			if (a.position.x != b.position.x) { return a.position.x < b.position.x; }
			if (a.position.z != b.position.z) { return a.position.z < b.position.z; }
			return a.position.y < b.position.y;
		});
		const auto data = formatFarmPlots(plots);
		if (data.empty() && !plots.empty()) { return false; }
		std::error_code error;
		std::filesystem::create_directories(cachedWorldPath, error);
		if (error) { return false; }
		return sfs::safeSave(data.data(), data.size(), farmSavePath(cachedWorldPath).c_str(), true)
			== sfs::noError;
	}
}

bool farmCropForItem(std::uint16_t itemType, FarmCrop &crop)
{
	switch (itemType)
	{
		case ItemTypes::wheat: crop = FarmCrop::Wheat; return true;
		case ItemTypes::strawberry: crop = FarmCrop::Strawberry; return true;
		case ItemTypes::chilliPepper: crop = FarmCrop::Chilli; return true;
		default: return false;
	}
}

std::uint16_t farmHarvestItem(FarmCrop crop)
{
	switch (crop)
	{
		case FarmCrop::Wheat: return ItemTypes::wheat;
		case FarmCrop::Strawberry: return ItemTypes::strawberry;
		case FarmCrop::Chilli: return ItemTypes::chilliPepper;
		default: return 0;
	}
}

double farmGrowthSeconds(FarmCrop crop)
{
	switch (crop)
	{
		case FarmCrop::Wheat: return 360.0;
		case FarmCrop::Strawberry: return 480.0;
		case FarmCrop::Chilli: return 420.0;
		default: return std::numeric_limits<double>::infinity();
	}
}

float farmGrowthFraction(const FarmPlotState &plot, double currentWorldSeconds)
{
	if (!validPlot(plot) || !std::isfinite(currentWorldSeconds)) { return 0.f; }
	const double duration = farmGrowthSeconds(plot.crop);
	if (!std::isfinite(duration) || duration <= 0.0) { return 0.f; }
	const double elapsed = std::max(0.0, currentWorldSeconds - plot.plantedWorldSeconds);
	return static_cast<float>(std::clamp(elapsed / duration, 0.0, 1.0));
}

bool farmPlotMature(const FarmPlotState &plot, double currentWorldSeconds)
{
	return farmGrowthFraction(plot, currentWorldSeconds) >= 1.f;
}

std::vector<unsigned char> formatFarmPlots(const std::vector<FarmPlotState> &plots)
{
	if (plots.size() > maxFarmPlots) { return {}; }
	for (const auto &plot : plots) { if (!validPlot(plot)) { return {}; } }

	std::vector<unsigned char> data;
	data.insert(data.end(), farmMagic.begin(), farmMagic.end());
	appendValue(data, farmFormatVersion);
	const std::uint32_t count = static_cast<std::uint32_t>(plots.size());
	appendValue(data, count);
	for (const auto &plot : plots)
	{
		const std::int32_t x = plot.position.x;
		const std::int32_t y = plot.position.y;
		const std::int32_t z = plot.position.z;
		const std::uint8_t crop = static_cast<std::uint8_t>(plot.crop);
		appendValue(data, x);
		appendValue(data, y);
		appendValue(data, z);
		appendValue(data, crop);
		appendValue(data, plot.plantedWorldSeconds);
	}
	return data;
}

bool parseFarmPlots(const void *data, std::size_t size, std::vector<FarmPlotState> &plots)
{
	plots.clear();
	Reader reader(data, size);
	std::array<unsigned char, farmMagic.size()> magic = {};
	std::uint32_t version = 0;
	std::uint32_t count = 0;
	if (!reader.readBytes(magic.data(), magic.size()) || magic != farmMagic ||
		!reader.read(version) || version != farmFormatVersion ||
		!reader.read(count) || count > maxFarmPlots)
	{
		return false;
	}

	std::unordered_map<glm::ivec3, bool, Ivec3Hash> seen;
	plots.reserve(count);
	for (std::uint32_t i = 0; i < count; ++i)
	{
		std::int32_t x = 0, y = 0, z = 0;
		std::uint8_t crop = 0;
		double planted = 0.0;
		if (!reader.read(x) || !reader.read(y) || !reader.read(z) ||
			!reader.read(crop) || !reader.read(planted))
		{
			plots.clear();
			return false;
		}
		FarmPlotState plot;
		plot.position = {x, y, z};
		plot.crop = static_cast<FarmCrop>(crop);
		plot.plantedWorldSeconds = planted;
		if (!validPlot(plot) || seen.find(plot.position) != seen.end())
		{
			plots.clear();
			return false;
		}
		seen.emplace(plot.position, true);
		plots.push_back(plot);
	}
	if (reader.remaining() != 0)
	{
		plots.clear();
		return false;
	}
	return true;
}

bool plantFarmPlot(const std::string &worldSavePath, glm::ivec3 position,
	std::uint16_t itemType, double currentWorldSeconds)
{
	FarmCrop crop;
	if (!farmCropForItem(itemType, crop) || !validPosition(position) ||
		!std::isfinite(currentWorldSeconds) || currentWorldSeconds < 0.0 ||
		!ensureLoaded(worldSavePath) || cachedPlots.find(position) != cachedPlots.end())
	{
		return false;
	}

	FarmPlotState plot;
	plot.position = position;
	plot.crop = crop;
	plot.plantedWorldSeconds = currentWorldSeconds;
	cachedPlots.emplace(position, plot);
	if (saveCache()) { return true; }
	cachedPlots.erase(position);
	return false;
}

bool harvestFarmPlot(const std::string &worldSavePath, glm::ivec3 position,
	double currentWorldSeconds, FarmHarvest &harvest)
{
	harvest = {};
	if (!ensureLoaded(worldSavePath)) { return false; }
	auto found = cachedPlots.find(position);
	if (found == cachedPlots.end() || !farmPlotMature(found->second, currentWorldSeconds))
	{
		return false;
	}

	harvest.itemType = farmHarvestItem(found->second.crop);
	harvest.count = found->second.crop == FarmCrop::Wheat ? 3 : 2;
	const FarmPlotState removed = found->second;
	cachedPlots.erase(found);
	if (saveCache()) { return true; }
	cachedPlots.emplace(removed.position, removed);
	harvest = {};
	return false;
}

bool queryFarmPlot(const std::string &worldSavePath, glm::ivec3 position,
	FarmPlotState &plot)
{
	if (!ensureLoaded(worldSavePath)) { return false; }
	auto found = cachedPlots.find(position);
	if (found == cachedPlots.end()) { return false; }
	plot = found->second;
	return true;
}

void resetFarmRuntimeCache()
{
	cachedWorldPath.clear();
	cacheLoaded = false;
	cachedPlots.clear();
}
