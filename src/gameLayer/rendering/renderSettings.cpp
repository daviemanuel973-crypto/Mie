#include <rendering/renderSettings.h>
#include <gamePlayLogic.h>
#include <filesystem>
#include <iostream>
#include <platform/platformInput.h>
#include "multyPlayer/createConnection.h"
#include <audioEngine.h>
#include <safeSave.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <worldCatalog.h>
#include <gameplay/playerControlSettings.h>
#include <gameplay/worldDifficulty.h>
#include <gameplay/worldGameMode.h>
#include <cstring>

namespace
{
	int pendingWorldGameModeIndex = static_cast<int>(WorldGameMode::Survival);

	void maybeRenderWorldGameModeSelector(ProgramData &programData, const char *label);
	bool saveDifficultyAndWorldGameMode(const std::filesystem::path &worldRoot,
		WorldDifficultySettings settings);
}

// Keep the large, proven settings UI intact. These two narrow hooks add the
// v0.9.3 world-mode selector after the Seed field and persist its choice at
// the same transaction boundary as world difficulty.
#define InputText(label, ...) InputText(label, __VA_ARGS__); maybeRenderWorldGameModeSelector(programData, label)
#define saveWorldDifficultySettings(worldRoot, settings) saveDifficultyAndWorldGameMode(worldRoot, settings)
#include "renderSettingsLegacy.inc"
#undef saveWorldDifficultySettings
#undef InputText

namespace
{
	void maybeRenderWorldGameModeSelector(ProgramData &programData, const char *label)
	{
		if (!label || std::strcmp(label, "Seed:") != 0) { return; }

		pendingWorldGameModeIndex = glm::clamp(pendingWorldGameModeIndex,
			static_cast<int>(WorldGameMode::Survival),
			static_cast<int>(WorldGameMode::Creative));
		programData.ui.menuRenderer.toggleOptions("Game mode: ",
			"Survival|Creative", &pendingWorldGameModeIndex, true, Colors_White,
			nullptr, programData.ui.buttonTexture, Colors_Gray,
			"Survival uses health, hunger, crafting and normal item limits. "
			"Creative starts new players with the creative inventory and unrestricted building.");

		if (pendingWorldGameModeIndex == static_cast<int>(WorldGameMode::Creative))
		{
			programData.ui.menuRenderer.Text(
				"Creative disables Hardcore when the world is created.", Colors_White);
		}
	}

	bool saveDifficultyAndWorldGameMode(const std::filesystem::path &worldRoot,
		WorldDifficultySettings settings)
	{
		WorldGameModeSettings gameModeSettings;
		gameModeSettings.mode = pendingWorldGameModeIndex == static_cast<int>(WorldGameMode::Creative)
			? WorldGameMode::Creative : WorldGameMode::Survival;
		gameModeSettings.sanitize();

		if (gameModeSettings.mode == WorldGameMode::Creative)
		{
			settings.hardcore = false;
		}
		settings.sanitize();

		if (!saveWorldDifficultySettings(worldRoot, settings)) { return false; }
		if (!saveWorldGameModeSettings(worldRoot, gameModeSettings)) { return false; }

		// A freshly opened creation dialog defaults back to Survival after a
		// successful create/create-and-play transaction.
		pendingWorldGameModeIndex = static_cast<int>(WorldGameMode::Survival);
		return true;
	}
}
