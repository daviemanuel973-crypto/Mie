#pragma once

#include "glui/glui.h"
#include <filesystem>

struct ProgramData;


void displayWorldSelectorMenuButton(ProgramData &programData);

void displayWorldSelectorMenu(ProgramData &programData);

void displayRenderSettingsMenuButton(ProgramData &programData);
void displayRenderSettingsMenu(ProgramData &programData);

void displaySettingsMenuButton(ProgramData &programData);
void displaySettingsMenu(ProgramData &programData);

void displayControlsSettingsMenuButton(ProgramData &programData);
void displayControlsSettingsMenu(ProgramData &programData);

bool shouldReloadTexturePacks();

std::vector<std::filesystem::path> getUsedTexturePacksAndResetFlag();

void displayTexturePacksSettingsMenuButton(ProgramData &programData);

void displayTexturePacksSettingsMenu(ProgramData &programData);

void displaySkinSelectorMenu(ProgramData &programData);

void displaySkinSelectorMenuButton(ProgramData &programData);

void displayVolumeMenu(ProgramData &programData);

void displayVolumeMenuButton(ProgramData &programData);


std::string getSkinName();

struct ShadingSettings
{

#if defined(OURCRAFT_LOW_END_BUILD)
	int viewDistance = 5;
	int tonemapper = 0;
	int shadows = 0;
	int waterType = 0;
	int workerThreadsForBaking = 1;
	int lodStrength = 5;
	int PBR = 0;
	int maxLights = 8;
	int useLights = 1;
	float lightsStrength = 1.f;
	bool FXAA = 1;
#else
	int viewDistance = 15;
	int tonemapper = 0;
	int shadows = 0;
	int waterType = 1;
	int workerThreadsForBaking = 2; //MOVE TODO
	int lodStrength = 1; //MOVE TODO
	int PBR = 1;
	int maxLights = 40;
	int useLights = 1;
	float lightsStrength = 1.f;
	bool FXAA = 1;
#endif

	glm::vec3 waterColor = (glm::vec3(6, 42, 52) / 255.f);
	glm::vec3 underWaterColor = glm::vec3(0, 17, 25) / 255.f;

	float underwaterDarkenStrength = 0.94;
	float underwaterDarkenDistance = 29;
	float fogGradientUnderWater = 1.9;
	
	float bloomTresshold = 0.5;
	float bloomMultiplier = 0.5;

	float exposure = 0;
	float fogGradient = 16.f;
#if defined(OURCRAFT_LOW_END_BUILD)
	int bloom = 0;

	int SSR = 0;
#else
	int bloom = 1;

	int SSR = 1;
#endif

	float toneMapSaturation = 1;
	float toneMapVibrance = 1;
	float toneMapGamma = 1;
	float toneMapShadowBoost = 0;
	float toneMapHighlightBoost = 0;
	float vignette = 0.15;
	glm::vec3 toneMapLift = glm::vec3(0.5);
	glm::vec3 toneMapGain = glm::vec3(0.5);

	void normalize();

	// Equality operator
	bool operator==(const ShadingSettings &other) const
	{
		return viewDistance == other.viewDistance && tonemapper == other.tonemapper &&
			shadows == other.shadows && waterType == other.waterType &&
			workerThreadsForBaking == other.workerThreadsForBaking &&
			lodStrength == other.lodStrength && PBR == other.PBR &&
			maxLights == other.maxLights && useLights == other.useLights &&
			lightsStrength == other.lightsStrength && FXAA == other.FXAA &&
			glm::all(glm::equal(waterColor, other.waterColor)) &&
			glm::all(glm::equal(underWaterColor, other.underWaterColor)) &&
			underwaterDarkenStrength == other.underwaterDarkenStrength &&
			underwaterDarkenDistance == other.underwaterDarkenDistance &&
			fogGradientUnderWater == other.fogGradientUnderWater &&
			bloomTresshold == other.bloomTresshold && bloomMultiplier == other.bloomMultiplier &&
			exposure == other.exposure && fogGradient == other.fogGradient &&
			bloom == other.bloom && SSR == other.SSR &&
			toneMapSaturation == other.toneMapSaturation &&
			toneMapVibrance == other.toneMapVibrance && toneMapGamma == other.toneMapGamma &&
			toneMapShadowBoost == other.toneMapShadowBoost &&
			toneMapHighlightBoost == other.toneMapHighlightBoost && vignette == other.vignette &&
			glm::all(glm::equal(toneMapLift, other.toneMapLift)) &&
			glm::all(glm::equal(toneMapGain, other.toneMapGain));
	}

	// Inequality operator
	bool operator!=(const ShadingSettings &other) const
	{
		return !(*this == other);
	}

	std::string formatIntoGLSLcode();
};

ShadingSettings &getShadingSettings();

bool checkIfShadingSettingsChangedForShaderReloads();

void applyLowEndPerformancePreset(ProgramData &programData);
void applyGraphicsPreset(ProgramData &programData, int preset);
int getRecommendedGraphicsPreset();
const char *getGraphicsPresetName(int preset);

void saveShadingSettings();

void loadShadingSettings();
