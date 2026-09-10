#include <gameplay/farming.h>
#include <gameplay/items.h>
#include <safeSave.h>

#include <cmath>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
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
	REQUIRE(farmCropForItem(ItemTypes::carrot, crop) && crop == FarmCrop::Carrot);
	REQUIRE(farmCropForItem(ItemTypes::potato, crop) && crop == FarmCrop::Potato);
	REQUIRE(!farmCropForItem(ItemTypes::apple, crop));
	REQUIRE(farmBlockForCrop(FarmCrop::Wheat) == BlockTypes::wheatCrop);
	REQUIRE(farmBlockForCrop(FarmCrop::Potato) == BlockTypes::potatoCrop);
	REQUIRE(farmCropForBlock(BlockTypes::carrotCrop, crop) && crop == FarmCrop::Carrot);
	REQUIRE(!farmCropForBlock(BlockTypes::dirt, crop));
	REQUIRE(canPlantFarmCrop(BlockTypes::dirt, BlockTypes::air, ItemTypes::wheat));
	REQUIRE(canPlantFarmCrop(BlockTypes::grassBlock, BlockTypes::air, ItemTypes::potato));
	REQUIRE(!canPlantFarmCrop(BlockTypes::stone, BlockTypes::air, ItemTypes::potato));
	REQUIRE(!canPlantFarmCrop(BlockTypes::dirt, BlockTypes::grass, ItemTypes::potato));
	REQUIRE(!canPlantFarmCrop(BlockTypes::dirt, BlockTypes::air, ItemTypes::apple));
	FarmPlotState missingPlot;

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
	REQUIRE(farmHarvestItem(FarmCrop::Carrot) == ItemTypes::carrot);
	REQUIRE(farmHarvestItem(FarmCrop::Potato) == ItemTypes::potato);
	REQUIRE(farmGrowthSeconds(FarmCrop::Carrot) == 360.0);
	REQUIRE(farmGrowthSeconds(FarmCrop::Potato) == 390.0);

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
	REQUIRE(queryFarmPlotStatus(tempRoot.string(), {19, 70, 20}, missingPlot) ==
		FarmPlotQueryStatus::Missing);
	resetFarmRuntimeCache();
	{
		std::ofstream(tempRoot / "farmPlots1.bin", std::ios::binary) << "corrupt";
		std::ofstream(tempRoot / "farmPlots2.bin", std::ios::binary) << "corrupt";
	}
	REQUIRE(queryFarmPlotStatus(tempRoot.string(), {19, 70, 20}, missingPlot) ==
		FarmPlotQueryStatus::StorageError);
	std::filesystem::remove_all(tempRoot, error);
	std::filesystem::create_directories(tempRoot, error);
	REQUIRE(!error);
	resetFarmRuntimeCache();
	REQUIRE(plantFarmPlot(tempRoot.string(), {20, 70, 20}, ItemTypes::wheat, 1000.0));
	REQUIRE(!plantFarmPlot(tempRoot.string(), {20, 70, 20}, ItemTypes::wheat, 1000.0));

	// A valid checksum cannot make an invalid schema authoritative over a good backup.
	auto badSchema = encoded;
	badSchema[8] = 99;
	REQUIRE(sfs::writeEntireFileWithCheckSum(badSchema.data(), badSchema.size(),
		(tempRoot / "farmPlots1.bin").string().c_str()) == sfs::noError);
	resetFarmRuntimeCache();
	REQUIRE(queryFarmPlot(tempRoot.string(), {20, 70, 20}, missingPlot));
	std::filesystem::copy_file(tempRoot / "farmPlots2.bin", tempRoot / "farmPlots1.bin",
		std::filesystem::copy_options::overwrite_existing);


	FarmHarvest harvest;
	REQUIRE(!harvestFarmPlot(tempRoot.string(), {20, 70, 20}, 1200.0, harvest));
	resetFarmRuntimeCache();
	FarmPlotState reloaded;
	REQUIRE(queryFarmPlotStatus(tempRoot.string(), {20, 70, 20}, reloaded) ==
		FarmPlotQueryStatus::Found);
	REQUIRE(reloaded.crop == FarmCrop::Wheat);
	REQUIRE(harvestFarmPlot(tempRoot.string(), {20, 70, 20}, 1360.0, harvest));
	REQUIRE(harvest.itemType == ItemTypes::wheat && harvest.count == 3);
	REQUIRE(!queryFarmPlot(tempRoot.string(), {20, 70, 20}, reloaded));

	resetFarmRuntimeCache();
	REQUIRE(plantFarmPlot(tempRoot.string(), {21, 70, 20}, ItemTypes::potato, 2000.0));
	REQUIRE(harvestFarmPlot(tempRoot.string(), {21, 70, 20}, 2390.0, harvest));
	REQUIRE(harvest.itemType == ItemTypes::potato && harvest.count == 3);

	resetFarmRuntimeCache();
	REQUIRE(plantFarmPlot(tempRoot.string(), {22, 70, 20}, ItemTypes::carrot, 3000.0));
	REQUIRE(uprootFarmPlot(tempRoot.string(), {22, 70, 20}, 3010.0, harvest));
	REQUIRE(harvest.itemType == ItemTypes::carrot && harvest.count == 1);
	REQUIRE(!queryFarmPlot(tempRoot.string(), {22, 70, 20}, reloaded));

	// Region workers share one farm cache. Every acknowledged concurrent write
	// must survive a fresh load, and competing placements consume only once.
	std::array<std::thread, 4> workers;
	std::array<int, 4> planted{};
	for (int worker = 0; worker < 4; ++worker)
	{
		workers[worker] = std::thread([&, worker]()
		{
			for (int plot = 0; plot < 4; ++plot)
			{
				if (plantFarmPlot(tempRoot.string(), {100 + worker, 70, plot}, ItemTypes::wheat, 4000.0))
				{
					++planted[worker];
				}
			}
		});
	}
	for (auto &worker : workers) { worker.join(); }
	resetFarmRuntimeCache();
	for (int worker = 0; worker < 4; ++worker)
	{
		REQUIRE(planted[worker] == 4);
		for (int plot = 0; plot < 4; ++plot)
		{
			REQUIRE(queryFarmPlot(tempRoot.string(), {100 + worker, 70, plot}, reloaded));
		}
	}
	for (int worker = 0; worker < 4; ++worker)
	{
		workers[worker] = std::thread([&, worker]()
		{
			planted[worker] = plantFarmPlot(tempRoot.string(), {200, 70, 0}, ItemTypes::wheat, 4000.0) ? 1 : 0;
		});
	}
	for (auto &worker : workers) { worker.join(); }
	REQUIRE(planted[0] + planted[1] + planted[2] + planted[3] == 1);

	// Failure before the primary commit must leave both cache and persisted state unchanged.
	std::filesystem::rename(tempRoot / "farmPlots1.bin", tempRoot / "saved-primary.bin");
	std::filesystem::create_directory(tempRoot / "farmPlots1.bin");
	REQUIRE(!plantFarmPlot(tempRoot.string(), {201, 70, 0}, ItemTypes::wheat, 4000.0));
	REQUIRE(!queryFarmPlot(tempRoot.string(), {201, 70, 0}, reloaded));
	std::filesystem::remove(tempRoot / "farmPlots1.bin");
	std::filesystem::rename(tempRoot / "saved-primary.bin", tempRoot / "farmPlots1.bin");
	resetFarmRuntimeCache();
	REQUIRE(!queryFarmPlot(tempRoot.string(), {201, 70, 0}, reloaded));

	// Failure of the backup after the primary commit cannot undo an acknowledged action.
	std::filesystem::remove(tempRoot / "farmPlots2.bin");
	std::filesystem::create_directory(tempRoot / "farmPlots2.bin");
	REQUIRE(plantFarmPlot(tempRoot.string(), {202, 70, 0}, ItemTypes::wheat, 4000.0));
	resetFarmRuntimeCache();
	REQUIRE(queryFarmPlot(tempRoot.string(), {202, 70, 0}, reloaded));
	REQUIRE(!std::filesystem::exists(tempRoot / "farmPlots1.bin.tmp"));
	REQUIRE(!std::filesystem::exists(tempRoot / "farmPlots2.bin.tmp"));

	std::filesystem::remove_all(tempRoot, error);
	std::cout << "Farming persistence tests passed.\n";
	return 0;
}
