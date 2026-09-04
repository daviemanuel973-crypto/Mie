#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

// v0.9 farming deliberately reuses existing produce item IDs and ordinary dirt
// blocks. Farm plots are world records keyed by position, so old chunks and the
// frozen v0.7 block-ID range need no migration.
enum class FarmCrop : std::uint8_t
{
	Wheat = 0,
	Strawberry,
	Chilli,
	Carrot,
	Potato,
	Count,
};

struct FarmPlotState
{
	glm::ivec3 position = {};
	FarmCrop crop = FarmCrop::Wheat;
	double plantedWorldSeconds = 0.0;

	bool operator==(const FarmPlotState &other) const
	{
		return position == other.position && crop == other.crop &&
			plantedWorldSeconds == other.plantedWorldSeconds;
	}
};

struct FarmHarvest
{
	std::uint16_t itemType = 0;
	std::uint16_t count = 0;
};

bool farmCropForItem(std::uint16_t itemType, FarmCrop &crop);
std::uint16_t farmHarvestItem(FarmCrop crop);
double farmGrowthSeconds(FarmCrop crop);
float farmGrowthFraction(const FarmPlotState &plot, double currentWorldSeconds);
bool farmPlotMature(const FarmPlotState &plot, double currentWorldSeconds);

std::vector<unsigned char> formatFarmPlots(const std::vector<FarmPlotState> &plots);
bool parseFarmPlots(const void *data, std::size_t size, std::vector<FarmPlotState> &plots);

// Runtime registry. The caller passes monotonically advancing world time (in
// seconds) so growth remains deterministic across chunk unloads and restarts.
bool plantFarmPlot(const std::string &worldSavePath, glm::ivec3 position,
	std::uint16_t itemType, double currentWorldSeconds);
bool harvestFarmPlot(const std::string &worldSavePath, glm::ivec3 position,
	double currentWorldSeconds, FarmHarvest &harvest);
bool queryFarmPlot(const std::string &worldSavePath, glm::ivec3 position,
	FarmPlotState &plot);
void resetFarmRuntimeCache();
