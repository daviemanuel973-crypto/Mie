#include <gameplay/farming.h>
#include <gameplay/items.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
	#define REQUIRE(condition) do { if (!(condition)) { \
		std::cerr << "Requirement failed at line " << __LINE__ << ": " #condition "\n"; \
		return 1; } } while (false)
}

int main()
{
	FarmCrop crop;
	REQUIRE(farmCropForItem(ItemTypes::wheat, crop) && crop == FarmCrop::Wheat);
	REQUIRE(farmCropForItem(ItemTypes::strawberry, crop) && crop == FarmCrop::Strawberry);
	REQUIRE(farmCropForItem(ItemTypes::chilliPepper, crop) && crop == FarmCrop::Chilli);
	REQUIRE(!farmCropForItem(ItemTypes::apple, crop));

	FarmPlotState wheat{{10, 64, -2}, FarmCrop::Wheat, 100.0};
	FarmPlotState strawberry{{11, 64, -2}, FarmCrop::Strawberry, 200.0};
	const std::vector<FarmPlotState> original{wheat, strawberry};
	const auto encoded = formatFarmPlots(original);
	REQUIRE(!encoded.empty());
	std::vector<FarmPlotState> decoded;
	REQUIRE(parseFarmPlots(encoded.data(), encoded.size(), decoded));
	REQUIRE(decoded == original);

	REQUIRE(std::fabs(farmGrowthFraction(wheat, 280.0) - 0.5f) < 0.001f);
	REQUIRE(!farmPlotMature(wheat, 459.0));
	REQUIRE(farmPlotMature(wheat, 460.0));
	REQUIRE(farmHarvestItem(FarmCrop::Wheat) == ItemTypes::wheat);

	for (std::size_t size = 0; size < encoded.size(); ++size)
	{
		std::vector<FarmPlotState> truncated;
		REQUIRE(!parseFarmPlots(encoded.data(), size, truncated));
	}

	auto corrupted = encoded;
	corrupted[0] ^= 0xff;
	REQUIRE(!parseFarmPlots(corrupted.data(), corrupted.size(), decoded));

	FarmPlotState invalid = wheat;
	invalid.plantedWorldSeconds = std::numeric_limits<double>::infinity();
	REQUIRE(formatFarmPlots({invalid}).empty());

	// Runtime persistence is transactional: planting is stored immediately,
	// premature harvest does nothing, and a fresh cache can reload the plot.
	const auto tempRoot = std::filesystem::temp_directory_path() / "mie-v09-farming-tests";
	std::error_code error;
	std::filesystem::remove_all(tempRoot, error);
	std::filesystem::create_directories(tempRoot, error);
	REQUIRE(!error);
	resetFarmRuntimeCache();
	REQUIRE(plantFarmPlot(tempRoot.string(), {20, 70, 20}, ItemTypes::wheat, 1000.0));
	REQUIRE(!plantFarmPlot(tempRoot.string(), {20, 70, 20}, ItemTypes::wheat, 1000.0));

	FarmHarvest harvest;
	REQUIRE(!harvestFarmPlot(tempRoot.string(), {20, 70, 20}, 1200.0, harvest));
	resetFarmRuntimeCache();
	FarmPlotState reloaded;
	REQUIRE(queryFarmPlot(tempRoot.string(), {20, 70, 20}, reloaded));
	REQUIRE(reloaded.crop == FarmCrop::Wheat);
	REQUIRE(harvestFarmPlot(tempRoot.string(), {20, 70, 20}, 1360.0, harvest));
	REQUIRE(harvest.itemType == ItemTypes::wheat && harvest.count == 3);
	REQUIRE(!queryFarmPlot(tempRoot.string(), {20, 70, 20}, reloaded));

	std::filesystem::remove_all(tempRoot, error);
	std::cout << "Farming persistence tests passed.\n";
	return 0;
}
