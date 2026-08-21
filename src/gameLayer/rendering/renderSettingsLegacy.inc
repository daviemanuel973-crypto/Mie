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
#include <cmath>
#include <worldCatalog.h>
#include <gameplay/playerControlSettings.h>
#include <gameplay/worldDifficulty.h>

void displayRenderSettingsMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Rendering", Colors_Gray, programData.ui.buttonTexture);

	displayRenderSettingsMenu(programData);

	programData.ui.menuRenderer.EndMenu();
}

#define DEFAULT_SLIDER Colors_White, programData.ui.buttonTexture, Colors_Gray, programData.ui.buttonTexture, Colors_White
#define DEFAULT_SLIDER_TRANSPARENT {1,1,1,0.65}, programData.ui.buttonTexture, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}, programData.ui.buttonTexture, {1,1,1,0.65}
#define DEFAULT_COLOR_PICKER programData.ui.buttonTexture, programData.ui.buttonTexture, Colors_Gray, Colors_Gray
#define DEFAULT_COLOR_PICKER_TRANSPARENT programData.ui.buttonTexture, programData.ui.buttonTexture, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}

const char *getGraphicsPresetName(int preset)
{
    switch (preset)
    {
        case 0: return "Low";
        case 1: return "Medium";
        case 2: return "High";
        case 3: return "Ultra";
        default: return "Custom";
    }
}

int getRecommendedGraphicsPreset()
{
    const char *rendererNameRaw = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    std::string rendererName = rendererNameRaw ? rendererNameRaw : "";
    std::transform(rendererName.begin(), rendererName.end(), rendererName.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    if (rendererName.find("llvmpipe") != std::string::npos ||
        rendererName.find("softpipe") != std::string::npos ||
        rendererName.find("software") != std::string::npos)
        return 0;

    if (rendererName.find("intel") != std::string::npos)
    {
        if (rendererName.find("hd graphics") != std::string::npos ||
            rendererName.find("uhd graphics") != std::string::npos)
            return 0;
        return 1;
    }

    if (maxTextureSize > 0 && maxTextureSize < 8192) return 0;
    if (rendererName.find("nvidia") != std::string::npos ||
        rendererName.find("geforce") != std::string::npos ||
        rendererName.find("radeon") != std::string::npos ||
        rendererName.find("amd") != std::string::npos)
        return 2;
    return 1;
}

void applyGraphicsPreset(ProgramData &programData, int preset)
{
    preset = glm::clamp(preset, 0, 3);
    auto &settings = getShadingSettings();

    programData.renderer.frustumCulling = true;
    programData.renderer.sortChunks = true;
    settings.useLights = 1;
    settings.FXAA = 1;

    if (preset == 0)
    {
        settings.viewDistance = 5;
        settings.lodStrength = 5;
        settings.workerThreadsForBaking = 1;
        settings.shadows = 0;
        settings.waterType = 0;
        settings.PBR = 0;
        settings.SSR = 0;
        settings.bloom = 0;
        settings.bloomMultiplier = 0.f;
        settings.maxLights = 8;
        programData.renderer.ssao = false;
        programData.renderer.fxaaData.ITERATIONS = 6;
        programData.renderer.fxaaData.quaityMultiplier = 0.65f;
        programData.renderer.fxaaData.SUBPIXEL_QUALITY = 0.60f;
    }
    else if (preset == 1)
    {
        settings.viewDistance = 8;
        settings.lodStrength = 3;
        settings.workerThreadsForBaking = 2;
        settings.shadows = 1;
        settings.waterType = 0;
        settings.PBR = 0;
        settings.SSR = 0;
        settings.bloom = 0;
        settings.bloomMultiplier = 0.f;
        settings.maxLights = 16;
        programData.renderer.ssao = false;
        programData.renderer.fxaaData.ITERATIONS = 8;
        programData.renderer.fxaaData.quaityMultiplier = 0.72f;
        programData.renderer.fxaaData.SUBPIXEL_QUALITY = 0.70f;
    }
    else if (preset == 2)
    {
        settings.viewDistance = 12;
        settings.lodStrength = 2;
        settings.workerThreadsForBaking = 3;
        settings.shadows = 1;
        settings.waterType = 1;
        settings.PBR = 1;
        settings.SSR = 0;
        settings.bloom = 1;
        settings.bloomMultiplier = 0.45f;
        settings.maxLights = 32;
        programData.renderer.ssao = true;
        programData.renderer.fxaaData.ITERATIONS = 10;
        programData.renderer.fxaaData.quaityMultiplier = 0.78f;
        programData.renderer.fxaaData.SUBPIXEL_QUALITY = 0.85f;
    }
    else
    {
        settings.viewDistance = 18;
        settings.lodStrength = 1;
        settings.workerThreadsForBaking = 4;
        settings.shadows = 2;
        settings.waterType = 1;
        settings.PBR = 1;
        settings.SSR = 1;
        settings.bloom = 1;
        settings.bloomMultiplier = 0.60f;
        settings.maxLights = 48;
        programData.renderer.ssao = true;
        programData.renderer.fxaaData.ITERATIONS = 12;
        programData.renderer.fxaaData.quaityMultiplier = 0.8f;
        programData.renderer.fxaaData.SUBPIXEL_QUALITY = 0.95f;
    }

    settings.normalize();
    saveShadingSettings();
}

void applyLowEndPerformancePreset(ProgramData &programData)
{
	applyGraphicsPreset(programData, 0);
}

void displayRenderSettingsMenu(ProgramData &programData)
{

	for (auto &s : programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId])
	{
		if (s == "Rendering" || s == "Settings")
		{
			programData.ui.renderer2d.renderText({150,50},
				("fps: " + std::to_string(programData.currentFps)).c_str(), programData.ui.font, Colors_Gray, 0.75f);
			break;
		}
	}



	programData.ui.menuRenderer.Text("Rendering Settings...", Colors_White);

	const int recommendedPreset = getRecommendedGraphicsPreset();
	std::string recommendation = std::string("Recommended for this GPU: ") + getGraphicsPresetName(recommendedPreset);
	programData.ui.menuRenderer.Text(recommendation.c_str(), Colors_White);
	if (programData.ui.menuRenderer.Button("Apply recommended graphics", Colors_Gray, programData.ui.buttonTexture))
		applyGraphicsPreset(programData, recommendedPreset);
	if (programData.ui.menuRenderer.Button("Graphics: Low", Colors_Gray, programData.ui.buttonTexture))
		applyGraphicsPreset(programData, 0);
	if (programData.ui.menuRenderer.Button("Graphics: Medium", Colors_Gray, programData.ui.buttonTexture))
		applyGraphicsPreset(programData, 1);
	if (programData.ui.menuRenderer.Button("Graphics: High", Colors_Gray, programData.ui.buttonTexture))
		applyGraphicsPreset(programData, 2);
	if (programData.ui.menuRenderer.Button("Graphics: Ultra", Colors_Gray, programData.ui.buttonTexture))
		applyGraphicsPreset(programData, 3);
	programData.ui.menuRenderer.Text("You can still tune every option below manually.", Colors_Gray);

	programData.ui.menuRenderer.sliderInt("View Distance", &getShadingSettings().viewDistance,
		1, 50, Colors_White, programData.ui.buttonTexture, Colors_Gray,
		programData.ui.buttonTexture, Colors_White);

	programData.ui.menuRenderer.sliderInt("Lod Strength", &getShadingSettings().lodStrength,
		0, 5, Colors_White, programData.ui.buttonTexture, Colors_Gray,
		programData.ui.buttonTexture, Colors_White);

#pragma region water
{
	programData.ui.menuRenderer.BeginMenu("Water", Colors_Gray, programData.ui.buttonTexture);

	programData.ui.menuRenderer.Text("Water settings...", Colors_White);


	programData.ui.menuRenderer.colorPicker("Water color",
		&programData.renderer.defaultShader.shadingSettings.waterColor[0],
		programData.ui.buttonTexture, programData.ui.buttonTexture, Colors_Gray, Colors_Gray);
	getShadingSettings().waterColor = programData.renderer.defaultShader.shadingSettings.waterColor;


	programData.ui.menuRenderer.colorPicker("Under water color",
		&programData.renderer.defaultShader.shadingSettings.underWaterColor[0],
		programData.ui.buttonTexture, programData.ui.buttonTexture, Colors_Gray, Colors_Gray);
	getShadingSettings().underWaterColor = programData.renderer.defaultShader.shadingSettings.underWaterColor;


	programData.ui.menuRenderer.sliderFloat("Underwater Fog strength",
		&programData.renderer.defaultShader.shadingSettings.underwaterDarkenStrength,
		0, 1, Colors_White, programData.ui.buttonTexture, Colors_Gray, 
		programData.ui.buttonTexture, Colors_White
		);
	getShadingSettings().underwaterDarkenStrength = programData.renderer.defaultShader.shadingSettings.underwaterDarkenStrength;


	programData.ui.menuRenderer.sliderFloat("Underwater Fog Distance",
		&programData.renderer.defaultShader.shadingSettings.underwaterDarkenDistance,
		0, 40, Colors_White, programData.ui.buttonTexture, Colors_Gray,
		programData.ui.buttonTexture, Colors_White);
	getShadingSettings().underwaterDarkenDistance = programData.renderer.defaultShader.shadingSettings.underwaterDarkenDistance;


	programData.ui.menuRenderer.sliderFloat("Underwater Fog Gradient",
		&programData.renderer.defaultShader.shadingSettings.fogGradientUnderWater,
		0, 32, Colors_White, programData.ui.buttonTexture, Colors_Gray,
		programData.ui.buttonTexture, Colors_White);
	getShadingSettings().fogGradientUnderWater = programData.renderer.defaultShader.shadingSettings.fogGradientUnderWater;


	static glm::vec4 colors[] = {{0.6,0.9,0.6,1}, Colors_Red};

	programData.ui.menuRenderer.toggleOptions("Water type: ", "cheap|fancy",
		&getShadingSettings().waterType, true, Colors_White, colors, programData.ui.buttonTexture,
		Colors_Gray,
		"How the water should be rendered\n-Cheap: \
good performance.\n-Fancy: significant performance cost but looks very nice.");

	programData.ui.menuRenderer.EndMenu();
}
#pragma endregion

#pragma region bloom
	programData.ui.menuRenderer.BeginMenu("Bloom", Colors_Gray, programData.ui.buttonTexture);
	programData.ui.menuRenderer.Text("Bloom settings...", Colors_White);

	programData.ui.menuRenderer.toggleOptions("BLoom: ", "OFF|ON", &getShadingSettings().bloom, true, Colors_White, 0, programData.ui.buttonTexture,
		Colors_Gray);

	if (getShadingSettings().bloom)
	{
		programData.ui.menuRenderer.sliderFloat("Bloom Multiplier", &getShadingSettings().bloomMultiplier, 0, 1, DEFAULT_SLIDER);
		programData.ui.menuRenderer.sliderFloat("Bloom Tresshold", &getShadingSettings().bloomTresshold, 0.1, 1, DEFAULT_SLIDER);
	};

	programData.ui.menuRenderer.EndMenu();
#pragma endregion

#pragma region lights
	programData.ui.menuRenderer.BeginMenu("Lights", Colors_Gray, programData.ui.buttonTexture);
	programData.ui.menuRenderer.Text("Lights settings...", Colors_White);

	programData.ui.menuRenderer.toggleOptions("Lights: ", "OFF|ON", &getShadingSettings().useLights, true, Colors_White, 0,
		programData.ui.buttonTexture, Colors_Gray, "If this is on, torches will contribute with specualr and diffuse lights.");

	if (getShadingSettings().useLights)
	{
		programData.ui.menuRenderer.sliderInt("Max lights", &getShadingSettings().maxLights, 1, 100, DEFAULT_SLIDER);
		programData.ui.menuRenderer.sliderFloat("Lights strength", &getShadingSettings().lightsStrength, 0.1, 2, DEFAULT_SLIDER);
	};

	programData.ui.menuRenderer.EndMenu();
#pragma endregion

#pragma region color post processing
	programData.ui.menuRenderer.BeginMenu("Color post processing", Colors_Gray, programData.ui.buttonTexture);

	programData.ui.menuRenderer.Text("Color post processing", {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65});

	//static glm::vec4 colorsTonemapper[] = {{0.6,0.9,0.6,1}, {0.6,0.9,0.6,1}, {0.7,0.8,0.6,1} , {0.4,0.8,0.4,1}};
	programData.ui.menuRenderer.toggleOptions("Tonemapper: ",
		"ACES|AgX|ZCAM|Uncharted|PBR neutral", &getShadingSettings().tonemapper,
		true, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}, nullptr, programData.ui.buttonTexture,
		{(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65},
		"The tonemapper is the thing that displays the final color\n\
-Aces: a filmic look.\n-AgX: a more dull neutral look.\n-ZCAM a verey neutral and vanila look\n   preserves colors, slightly more expensive.\n-Unchrated :))");
	programData.renderer.defaultShader.shadingSettings.tonemapper = getShadingSettings().tonemapper;

	if (programData.ui.menuRenderer.Button("Reset settings...", {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}, programData.ui.buttonTexture))
	{
		getShadingSettings().toneMapSaturation = 1;
		getShadingSettings().toneMapVibrance = 1;
		getShadingSettings().toneMapGamma = 1;
		getShadingSettings().toneMapShadowBoost = 0;
		getShadingSettings().toneMapHighlightBoost = 0;
		getShadingSettings().vignette = 0.15f;
		getShadingSettings().toneMapLift = glm::vec3(0.5);
		getShadingSettings().toneMapGain = glm::vec3(0.5);
	}

	programData.ui.menuRenderer.sliderFloat("Vignette", &getShadingSettings().vignette, 0, 1, DEFAULT_SLIDER_TRANSPARENT);
	programData.ui.menuRenderer.sliderFloat("Saturation", &getShadingSettings().toneMapSaturation, 0, 2, DEFAULT_SLIDER_TRANSPARENT);
	programData.ui.menuRenderer.sliderFloat("Vibrance", &getShadingSettings().toneMapVibrance, 0, 2, DEFAULT_SLIDER_TRANSPARENT);
	programData.ui.menuRenderer.sliderFloat("Gamma", &getShadingSettings().toneMapGamma, 0.1, 2, DEFAULT_SLIDER_TRANSPARENT);
	programData.ui.menuRenderer.sliderFloat("Shadow Boost", &getShadingSettings().toneMapShadowBoost, -1, 1, DEFAULT_SLIDER_TRANSPARENT);
	programData.ui.menuRenderer.sliderFloat("Highlight Boost", &getShadingSettings().toneMapHighlightBoost, -1, 1, DEFAULT_SLIDER_TRANSPARENT);
	

	programData.ui.menuRenderer.colorPicker("Lift", &getShadingSettings().toneMapLift[0], DEFAULT_COLOR_PICKER_TRANSPARENT);
	programData.ui.menuRenderer.colorPicker("Gain", &getShadingSettings().toneMapGain[0], DEFAULT_COLOR_PICKER_TRANSPARENT);

	//glUniform1f(applyToneMapper.u_saturation, shadingSettings.toneMapSaturation);
	//glUniform1f(applyToneMapper.u_vibrance, shadingSettings.toneMapVibrance);
	//glUniform1f(applyToneMapper.u_gamma, shadingSettings.toneMapGamma);
	//glUniform1f(applyToneMapper.u_shadowBoost, shadingSettings.toneMapShadowBoost);
	//glUniform1f(applyToneMapper.u_highlightBoost, shadingSettings.toneMapHighlightBoost);
	//glUniform3f(applyToneMapper.u_lift, shadingSettings.toneMapLift.x, shadingSettings.toneMapLift.y, shadingSettings.toneMapLift.z);
	//glUniform3f(applyToneMapper.u_gain, shadingSettings.toneMapGain.x, shadingSettings.toneMapGain.y, shadingSettings.toneMapGain.z);







	programData.ui.menuRenderer.EndMenu();

#pragma endregion






	programData.ui.menuRenderer.newColum(2);

	programData.ui.menuRenderer.Text("", {});
	
	programData.ui.menuRenderer.sliderInt("Chunk building extra threads", 
		&getShadingSettings().workerThreadsForBaking,
		0, 10, Colors_White, programData.ui.buttonTexture, Colors_Gray,
		programData.ui.buttonTexture, Colors_White);

	displayTexturePacksSettingsMenuButton(programData);

#pragma region SSR
	programData.ui.menuRenderer.BeginMenu("Screen Space Reflections", Colors_Gray, programData.ui.buttonTexture);
	programData.ui.menuRenderer.Text("Screen Space Reflections settings...", Colors_White);

	programData.ui.menuRenderer.toggleOptions("SSR: ", "OFF|ON", &getShadingSettings().SSR, true, Colors_White, 0, programData.ui.buttonTexture,
		Colors_Gray);

	if (getShadingSettings().SSR)
	{
		//... other SSR settings like quality stuff
	};

	programData.ui.menuRenderer.EndMenu();
#pragma endregion


	//programData.menuRenderer.BeginMenu("Volumetric", Colors_Gray, programData.buttonTexture);
	//programData.menuRenderer.Text("Volumetric Settings...", Colors_White);
	programData.ui.menuRenderer.sliderFloat("Fog gradient (O to disable it)",
		&getShadingSettings().fogGradient,
		0, 100, Colors_White, programData.ui.buttonTexture, Colors_Gray,
		programData.ui.buttonTexture, Colors_White);
	//programData.menuRenderer.EndMenu();


	static glm::vec4 colorsShadows[] = {{0.0,1,0.0,1}, {0.8,0.6,0.6,1}, {0.9,0.3,0.3,1}};
	programData.ui.menuRenderer.toggleOptions("Shadows: ", "Off|Hard|Soft",
		&getShadingSettings().shadows, true,
		Colors_White, colorsShadows, programData.ui.buttonTexture,
		Colors_Gray, "Shadows can affect the performance significantly."
	);


	programData.ui.menuRenderer.ToggleButton("FXAA", Colors_White, &getShadingSettings().FXAA, programData.ui.buttonTexture,
		Colors_Gray);


}

glm::ivec4 shrinkPercentage(glm::ivec4 dimensions, glm::vec2 p)
{
	glm::vec4 b = dimensions;

	b.x += (b.z * p.x) / 2.f;
	b.y += (b.w * p.y) / 2.f;

	b.z *= (1.f - p.x);
	b.w *= (1.f - p.y);

	dimensions = b;
	return dimensions;
}

bool texturePackDirty = 1;
int leftAdvance = 0;
int rightAdvance = 0;
std::vector<std::filesystem::path> loadedTexturePacks;
std::unordered_map<std::string, gl2d::Texture> logoTextures;

std::vector<std::filesystem::path> usedTexturePacks{"ourcraft"};

void displaySettingsMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Settings", Colors_Gray, programData.ui.buttonTexture);

	displaySettingsMenu(programData);

	programData.ui.menuRenderer.EndMenu();
}

void displaySettingsMenu(ProgramData &programData)
{

	programData.ui.menuRenderer.Text("Settings", Colors_White);

	displayRenderSettingsMenuButton(programData);
	
	displayVolumeMenuButton(programData);

	displayControlsSettingsMenuButton(programData);
}

void displayControlsSettingsMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Controls", Colors_Gray, programData.ui.buttonTexture);
	displayControlsSettingsMenu(programData);
	programData.ui.menuRenderer.EndMenu();
}

void displayControlsSettingsMenu(ProgramData &programData)
{
	auto &settings = getPlayerControlSettings();
	programData.ui.menuRenderer.Text("Movement assists", Colors_White);
	const bool autoJumpChanged = programData.ui.menuRenderer.ToggleButton("Auto jump on half blocks",
		Colors_White, &settings.autoJump, programData.ui.buttonTexture, Colors_Gray);
	const bool autoRunChanged = programData.ui.menuRenderer.ToggleButton("Auto run",
		Colors_White, &settings.autoRun, programData.ui.buttonTexture, Colors_Gray);
	programData.ui.menuRenderer.Text(std::string("Camera: ") +
		getPlayerCameraModeName(settings.cameraMode), Colors_White);
	if (programData.ui.menuRenderer.Button("Cycle camera (F5)", Colors_Gray,
		programData.ui.buttonTexture))
	{
		cyclePlayerCameraMode();
	}
	else if (autoJumpChanged || autoRunChanged)
	{
		savePlayerControlSettings();
	}
}

bool shouldReloadTexturePacks()
{
	return texturePackDirty;
}

std::vector<std::filesystem::path> getUsedTexturePacksAndResetFlag()
{
	texturePackDirty = false;
	return usedTexturePacks;
}

bool isTexturePackUsed(std::string t)
{
	for (auto &t2 : usedTexturePacks)
	{
		if (t == t2) { return true; }
	}
	return false;
}

void useTexturePack(std::string t)
{
	if (!isTexturePackUsed(t))
	{
		usedTexturePacks.push_back(t);
		texturePackDirty = true;
	}
}

void unuseTexturePack(std::string t)
{
	auto f = std::find(usedTexturePacks.begin(), usedTexturePacks.end(), t);

	if (f != usedTexturePacks.end())
	{
		usedTexturePacks.erase(f);
		texturePackDirty = true;
	}
}

void displayTexturePacksSettingsMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Textures Packs", Colors_Gray, programData.ui.buttonTexture);

	displayTexturePacksSettingsMenu(programData);

	programData.ui.menuRenderer.EndMenu();
}


void openFolder(const char *path)
{
	//TODO!
#if defined(_WIN32) || defined(_WIN64)
	std::string command = "explorer ";
	command += path;
	const int openResult = system(command.c_str());
	(void)openResult;
#elif defined(__APPLE__) || defined(__MACH__)
	std::string command = "open ";
	command += path;
	const int openResult = system(command.c_str());
	(void)openResult;
#elif defined(__linux__)
	std::string command = "xdg-open ";
	command += path;
	const int openResult = system(command.c_str());
	(void)openResult;
#else
	
#endif
}

inline void stubErrorFunc(const char *msg, void *userDefinedData)
{
	
}


bool renderButton(gl2d::Renderer2D &renderer,
	gl2d::Texture buttonTexture,
	glm::ivec4 box, const std::string &text = "", gl2d::Font *f = 0)
{
	bool hovered = 0;
	bool held = 0;
	bool released = 0;
	auto cursorPos = platform::getRelMousePosition();

	if (glui::aabb(box, cursorPos))
	{
		hovered = 1;

		if (platform::isLMouseHeld())
		{
			held = 1;
		}
		else if (platform::isLMouseReleased())
		{
			released = 1;
		}
	}

	if (held)
	{
		box.y += 10;
	}

	if (buttonTexture.id)
	{

		auto color = Colors_Gray;

		if (hovered)
		{
			color += glm::vec4(0.1, 0.1, 0.1, 0);
		}

		renderer.render9Patch(box,
			20, color, {}, 0, buttonTexture,
			GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});
	}

	if (text.size() && f)
	{
		glui::renderText(renderer, text, *f, box, Colors_White, 0, true, true);
	}

	return released;
};


void displayTexturePacksSettingsMenu(ProgramData &programData)
{
	std::error_code err;


	programData.ui.menuRenderer.Text("Texture packs...", Colors_White);

	//todo
	//if (programData.ui.menuRenderer.Button("Open Folder", Colors_Gray))
	//{
	//	if (!std::filesystem::exists(USER_CONTENT_PATH "texturePacks"))
	//	{
	//		std::filesystem::create_directories(USER_CONTENT_PATH "texturePacks", err);
	//	}
	//	
	//	openFolder(USER_CONTENT_PATH "texturePacks");
	//}


	glm::vec4 customWidgetTransform = {};
	programData.ui.menuRenderer.CustomWidget(169, &customWidgetTransform);



	//programData.ui.menuRenderer.cu
	auto &renderer = programData.ui.renderer2d;


	auto renderBox = [&](glm::vec4 c)
	{
		renderer.render9Patch(glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f)(),
			20, c, {}, 0, programData.ui.buttonTexture, GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});
	};

	if (programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].size()
		&& programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].back() == "Textures Packs"
		)
	{

		std::vector<std::filesystem::path> allTexturePacks;

		if (!std::filesystem::exists(USER_CONTENT_PATH "texturePacks"))
		{
			std::filesystem::create_directories(USER_CONTENT_PATH "texturePacks", err);
		}

		for (auto const &d : std::filesystem::directory_iterator{USER_CONTENT_PATH "texturePacks", err})
		{
			if (d.is_directory())
			{
				allTexturePacks.push_back(d.path().filename());
			}
		}

		//load logo textures
		gl2d::setErrorFuncCallback(stubErrorFunc);
		for (auto &pack : allTexturePacks)
		{
			auto file = pack;
			file = (USER_CONTENT_PATH "texturePacks") / file;
			file /= "logo.png";

			if (logoTextures.find(pack.string()) == logoTextures.end())
			{
				gl2d::Texture t;
				t.loadFromFile(file.string().c_str());

				if (!t.id)
				{
					file = pack;
					file = (USER_CONTENT_PATH "texturePacks") / file;
					file /= "pack.png";
					t.loadFromFile(file.string().c_str(), true);

				}

				logoTextures[pack.string()] = t;
			}
		}
		gl2d::setErrorFuncCallback(gl2d::defaultErrorFunc);
		//TODO delete unused entries


		auto listImplementation = [&](bool left)
		{
			std::vector<std::filesystem::path> *listPacks = 0;
			int *advance = 0;
			//buttons
			glm::ivec4 b;
			if (left)
			{
				b = glui::Box().xLeft().yTop().
					xDimensionPercentage(0.5).yDimensionPercentage(1).shrinkPercentage({0.05,0.05});
				listPacks = &allTexturePacks;
				advance = &leftAdvance;
			}
			else
			{
				b = glui::Box().xRight().yTop().
					xDimensionPercentage(0.5).yDimensionPercentage(1).shrinkPercentage({0.05,0.05});
				listPacks = &usedTexturePacks;
				advance = &rightAdvance;
			}

			glui::Frame f(b);

			float buttonSize = glui::Box().xLeft().yTop().xDimensionPercentage(0.1)().z;

			auto currentUpperBox = glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f)();

			//center
			{

				glui::Frame f(glui::Box().xCenter().yCenter().
					xDimensionPercentage(1).
					yDimensionPixels(currentUpperBox.w - buttonSize * 2)());

				auto currentBox = glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f)();

				//renderer.renderRectangle(,
				//	{0,1,0,0.2});

				renderBox({0.6,0.6,0.6,0.5});

				int height = currentBox.w - 10;

				int maxH = std::min(height, int(currentBox.z * 0.2));

				auto defaultB = glui::Box().xLeft().yTop().xDimensionPercentage(1).yDimensionPixels(maxH)();
				defaultB.y += 10;

				auto renderOneTeturePack = [&](glm::ivec4 box,
					std::string name)
				{
					box = shrinkPercentage(box, {0.1,0.05});

					renderer.render9Patch(box,
						20, {0.6,0.6,0.6,0.9}, {}, 0, programData.ui.buttonTexture,
						GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});

					{
						glui::Frame f(box);

						auto button = glui::Box().xRight().yTop().yDimensionPercentage(1.f).xDimensionPixels(buttonSize)();
						if (renderButton(renderer, programData.ui.buttonTexture, button))
						{
							if (left)
							{
								useTexturePack(name);
							}
							else
							{
								unuseTexturePack(name);
							}
						}

						auto icon = glui::Box().xLeft().yTop().yDimensionPercentage(1.f).xDimensionPixels(box.w)();

						if (logoTextures[name].id)
						{
							glui::renderTexture(renderer, shrinkPercentage(icon, {0.1,0.1}),
								logoTextures[name], Colors_White, GL2D_DefaultTextureCoords);
						}
						else
						{
							glui::renderTexture(renderer, shrinkPercentage(icon, {0.1,0.1}),
								programData.defaultCover, Colors_White, GL2D_DefaultTextureCoords);
						}


						auto text = icon;
						text.x += icon.z;
						text.z = button.x - text.x;

						glui::renderText(renderer, name, programData.ui.font, text, Colors_White, true);
					}


				};

				int canRenderCount = height / defaultB.w;

				int overflow = listPacks->size() - canRenderCount;
				if (overflow < 0) { overflow = 0; }
				if (*advance > overflow) { *advance = overflow; }

				for (int i = 0; i < listPacks->size(); i++)
				{
					if (i >= canRenderCount) { break; }

					renderOneTeturePack(defaultB, (*listPacks)[i + *advance].filename().string());
					defaultB.y += defaultB.w;
				}


			}

			//bottom button
			{
				auto currentDownBox = glui::Box().xLeft().yBottom().xDimensionPercentage(1).yDimensionPixels(buttonSize)();

				if (renderButton(renderer, programData.ui.buttonTexture, currentDownBox))
				{
					(*advance)++;
				}
			}

			//top button
			{
				auto currentUpperBox = glui::Box().xLeft().yTop().xDimensionPercentage(1).yDimensionPixels(buttonSize)();

				if (renderButton(renderer, programData.ui.buttonTexture, currentUpperBox))
				{
					(*advance)--;
				}
			}

			if (*advance < 0) { *advance = 0; }

		};
		

		glui::Frame f({0,0,renderer.windowW, renderer.windowH});

		{
			float ySize = renderer.windowH - customWidgetTransform.y - customWidgetTransform.w/2.f;

			glui::Frame f(glui::Box().xCenter().yTop(customWidgetTransform.y).
				yDimensionPixels(ySize).xDimensionPercentage(0.9)());

			//renderer.renderRectangle(glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f),
			//	{1,0,0,0.2});
			renderBox({0.2,0.2,0.2,0.4});

			//left
			{
				listImplementation(true);
			}

			//rioght
			{
				listImplementation(false);
			}

		}

	}
}

std::string currentSkinSelected = "";

gl2d::Texture currentTextureLoaded;
std::string currentTextureLoadedName = "";

void loadTexture()
{

	if (currentTextureLoadedName != currentSkinSelected || !currentTextureLoaded.id)
	{
		currentTextureLoaded.cleanup();
	}

	if(!currentTextureLoaded.id)
	{
		currentTextureLoadedName = currentSkinSelected;

		if (currentTextureLoadedName == "")
		{
			currentTextureLoaded
				= loadPlayerSkin(RESOURCES_PATH "assets/models/steve.png");
		}
		else
		{
			currentTextureLoaded
				= loadPlayerSkin((RESOURCES_PATH "skins/" + currentTextureLoadedName + ".png").c_str());
		}
	}

}

std::string getSkinName()
{
	return currentSkinSelected;
}

ShadingSettings shadingSettings;
ShadingSettings shadingSettingsLastFrame;

ShadingSettings &getShadingSettings()
{
	return shadingSettings;
}

bool checkIfShadingSettingsChangedForShaderReloads()
{
	shadingSettings.normalize();
	if (
		shadingSettings.shadows != shadingSettingsLastFrame.shadows ||
		shadingSettings.PBR != shadingSettingsLastFrame.PBR ||
		shadingSettings.useLights != shadingSettingsLastFrame.useLights ||
		shadingSettings.SSR != shadingSettingsLastFrame.SSR 
		)
	{
		shadingSettingsLastFrame = shadingSettings;
		return true;
	}

	return false;
}

#define SET_INT(x) data.setInt( #x, shadingSettings. x )
#define SET_VEC3(x) data.setRawData( #x, &shadingSettings. x [0], sizeof(shadingSettings. x) )
#define SET_FLOAT(x) data.setFloat( #x, shadingSettings. x )


void saveShadingSettings()
{

	shadingSettings.normalize();

	sfs::SafeSafeKeyValueData data;
	data.setInt("Version", 1);

	SET_INT(viewDistance);
	SET_INT(lodStrength);
	SET_INT(workerThreadsForBaking);
	SET_INT(tonemapper);
	SET_INT(shadows);
	SET_INT(waterType);
	SET_INT(PBR);
	SET_INT(SSR);
	SET_INT(maxLights);
	SET_INT(useLights);
	SET_INT(bloom);
	SET_FLOAT(lightsStrength);

	SET_VEC3(waterColor);
	SET_VEC3(underWaterColor);

	SET_FLOAT(underwaterDarkenStrength);
	SET_FLOAT(underwaterDarkenDistance);
	SET_FLOAT(fogGradientUnderWater);
	SET_FLOAT(bloomTresshold);
	SET_FLOAT(bloomMultiplier);
	SET_FLOAT(exposure);
	SET_FLOAT(fogGradient);


	SET_FLOAT(toneMapSaturation);
	SET_FLOAT(toneMapVibrance);
	SET_FLOAT(toneMapGamma);
	SET_FLOAT(toneMapShadowBoost);
	SET_FLOAT(toneMapHighlightBoost);
	SET_FLOAT(vignette);
	SET_VEC3(toneMapLift);
	SET_VEC3(toneMapGain);

	data.setBool("FXAA", shadingSettings.FXAA);



	sfs::safeSave(data, USER_SETTINGS_PATH "renderSettings", 0);

}

#undef SET_INT
#undef SET_VEC3
#undef SET_FLOAT



#define GET_INT(x) data.getInt( #x, shadingSettings. x )
#define GET_VEC3(x) data.setRawData( #x, &shadingSettings. x [0], sizeof(shadingSettings. x) )
#define GET_FLOAT(x) data.getFloat( #x, shadingSettings. x )
void loadShadingSettings()
{

	shadingSettings = ShadingSettings{};

	sfs::SafeSafeKeyValueData data;

	if (sfs::safeLoad(data, USER_SETTINGS_PATH "renderSettings", 0) == sfs::noError)
	{
		GET_INT(viewDistance);
		GET_INT(lodStrength);
		GET_INT(workerThreadsForBaking);
		GET_INT(tonemapper);
		GET_INT(shadows);
		GET_INT(waterType);
		GET_INT(PBR);
		GET_INT(SSR);
		GET_INT(maxLights);
		GET_INT(bloom);
		GET_INT(useLights);
		GET_FLOAT(lightsStrength);

		GET_FLOAT(toneMapSaturation);
		GET_FLOAT(toneMapVibrance);
		GET_FLOAT(toneMapGamma);
		GET_FLOAT(toneMapShadowBoost);
		GET_FLOAT(toneMapHighlightBoost);
		GET_FLOAT(vignette);


		void *rawData = 0;
		size_t dataSize = 0;
		data.getRawDataPointer("waterColor", rawData, dataSize);
		if (dataSize == sizeof(shadingSettings.waterColor))
			{ memcpy(&shadingSettings.waterColor[0], rawData, dataSize); }

		data.getRawDataPointer("underWaterColor", rawData, dataSize);
		if (dataSize == sizeof(shadingSettings.underWaterColor))
			{ memcpy(&shadingSettings.underWaterColor[0], rawData, dataSize); }

		data.getRawDataPointer("toneMapLift", rawData, dataSize);
		if (dataSize == sizeof(shadingSettings.toneMapLift))
			{ memcpy(&shadingSettings.toneMapLift[0], rawData, dataSize); }

		data.getRawDataPointer("toneMapGain", rawData, dataSize);
		if (dataSize == sizeof(shadingSettings.toneMapGain))
			{ memcpy(&shadingSettings.toneMapGain[0], rawData, dataSize); }

		data.getBool("FXAA", shadingSettings.FXAA);

		GET_FLOAT(underwaterDarkenStrength);
		GET_FLOAT(underwaterDarkenDistance);
		GET_FLOAT(fogGradientUnderWater);
		GET_FLOAT(exposure);

		GET_FLOAT(bloomTresshold);
		GET_FLOAT(bloomMultiplier);
		GET_FLOAT(fogGradient);

	}

	shadingSettings.normalize();

	//todo apply

}

#undef GET_INT
#undef GET_VEC3
#undef GET_FLOAT


void displaySkinSelectorMenu(ProgramData &programData)
{
	std::error_code err;

	programData.ui.menuRenderer.Text("Change Skin", Colors_White);

	//todo
	//if (programData.ui.menuRenderer.Button("Open Folder", Colors_Gray))
	//{
	//	if (!std::filesystem::exists(USER_CONTENT_PATH "texturePacks"))
	//	{
	//		std::filesystem::create_directories(USER_CONTENT_PATH "texturePacks", err);
	//	}
	//	
	//	openFolder(USER_CONTENT_PATH "texturePacks");
	//}

	glm::vec4 customWidgetTransform = {};
	programData.ui.menuRenderer.CustomWidget(170, &customWidgetTransform);

	//programData.ui.menuRenderer.cu
	auto &renderer = programData.ui.renderer2d;
	
	auto renderBox = [&](glm::vec4 c)
	{
		renderer.render9Patch(glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f)(),
			20, c, {}, 0, programData.ui.buttonTexture, GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});
	};

	if (programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].size()
		&& programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].back() == "Change Skin"
		)
	{

		if (!std::filesystem::exists(USER_CONTENT_PATH "skins"))
		{
			std::filesystem::create_directories(USER_CONTENT_PATH "skins", err);
		}

		std::vector<std::string> skins;

		for (auto const &d : std::filesystem::directory_iterator{USER_CONTENT_PATH "skins", err})
		{
			if (!d.is_directory())
			{
				if (d.path().filename().extension() == ".png")
				{
					skins.push_back(d.path().filename().stem().string());
				}
			}
		}

		int posInVect = -1;
		if (currentSkinSelected == "")
		{
			posInVect = -1;
		}
		else
		{
			int index = 0;
			for(auto &s : skins)
			{
				if (s == currentSkinSelected)
				{
					currentSkinSelected = s;
					posInVect = index;
					break;
				}
				index++;
			}
		}

		

		glui::Frame f({0,0,renderer.windowW, renderer.windowH});
		{
			float ySize = renderer.windowH - customWidgetTransform.y - customWidgetTransform.w / 2.f;

			glui::Frame f(glui::Box().xCenter().yTop(customWidgetTransform.y).
				yDimensionPixels(ySize).xDimensionPercentage(0.9)());

			//renderer.renderRectangle(glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f),
			//	{1,0,0,1.0});
			renderBox({0.4,0.4,0.4,0.5});


			auto textBox = glui::Box().xCenter().yBottom().xDimensionPercentage(1).yDimensionPercentage(0.2)();
			if (currentSkinSelected == "")
			{
				glui::renderText(renderer, "Default", programData.ui.font, textBox, Colors_White, true);
			}
			else
			{
				glui::renderText(renderer, currentSkinSelected, programData.ui.font, textBox, Colors_White, true);
			}

			auto center = glui::Box().xCenter().yCenter().yDimensionPercentage(0.5).xDimensionPercentage(0.5)();
			center.z = std::min(center.z, center.w);
			center = glui::Box().xCenter().yCenter().yDimensionPixels(center.z).xDimensionPixels(center.z)();

			loadTexture();
			if (currentTextureLoaded.id)
			{
				renderer.renderRectangle(center, currentTextureLoaded);
			}

			auto left = glui::Box().xLeft().yCenter().yDimensionPercentage(0.2).xAspectRatio(1.0)();
			auto right = glui::Box().xRight().yCenter().yDimensionPercentage(0.2).xAspectRatio(1.0)();

			//move cursor
			{
				if (renderButton(renderer, programData.ui.buttonTexture, left))
				{
					posInVect--;
				}

				if (renderButton(renderer, programData.ui.buttonTexture, right))
				{
					posInVect++;
				}

				if (posInVect < -1)
				{
					posInVect = skins.size() - 1;
				}

				if (posInVect >= skins.size())
				{
					posInVect = -1;
				}

				if (posInVect == -1)
				{
					currentSkinSelected = "";
				}
				else
				{
					currentSkinSelected = skins[posInVect];
				}
			}

		
		}
	}

}

void displaySkinSelectorMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Change Skin", Colors_Gray, programData.ui.buttonTexture);

	displaySkinSelectorMenu(programData);

	programData.ui.menuRenderer.EndMenu();
}

void displayVolumeMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Audio Settings", Colors_Gray, programData.ui.buttonTexture);

	displayVolumeMenu(programData);

	programData.ui.menuRenderer.EndMenu();
}

void displayVolumeMenu(ProgramData &programData)
{

	programData.ui.menuRenderer.Text("Audio Settings", Colors_White);

	programData.ui.menuRenderer.sliderFloat("Master Volume", &AudioEngine::getMasterVolume(), 0, 1, Colors_White, programData.ui.buttonTexture, Colors_Gray, programData.ui.buttonTexture, Colors_White);
	programData.ui.menuRenderer.newLine();
	programData.ui.menuRenderer.sliderFloat("Music Volume", &AudioEngine::getMusicVolume(), 0, 1, Colors_White, programData.ui.buttonTexture, Colors_Gray, programData.ui.buttonTexture, Colors_White);
	programData.ui.menuRenderer.sliderFloat("UI Volume", &AudioEngine::getUIVolume(), 0, 1, Colors_White, programData.ui.buttonTexture, Colors_Gray, programData.ui.buttonTexture, Colors_White);
	programData.ui.menuRenderer.sliderFloat("Sounds Volume", &AudioEngine::getSoundsVolume(), 0, 1, Colors_White, programData.ui.buttonTexture, Colors_Gray, programData.ui.buttonTexture, Colors_White);

}


void displayWorldSelectorMenuButton(ProgramData &programData)
{
	programData.ui.menuRenderer.BeginMenu("Play", Colors_Gray, programData.ui.buttonTexture);

	displayWorldSelectorMenu(programData);

	programData.ui.menuRenderer.EndMenu();
}


void displayWorldSelectorMenu(ProgramData &programData)
{
	auto &renderer = programData.ui.renderer2d;
	static std::string selected;
	static std::vector<WorldCatalogEntry> allWorlds;
	static std::string catalogError;
	static auto lastCatalogRefresh = std::chrono::steady_clock::time_point{};
	static bool forceCatalogRefresh = true;

	auto refreshCatalog = [&]()
	{
		allWorlds = loadWorldCatalog(USER_CONTENT_PATH "worlds", &catalogError);
		lastCatalogRefresh = std::chrono::steady_clock::now();
		forceCatalogRefresh = false;

		auto selectedEntry = std::find_if(allWorlds.begin(), allWorlds.end(),
			[&](const auto &entry) { return entry.folderName == selected; });
		if (selectedEntry == allWorlds.end())
		{
			selected = allWorlds.empty() ? std::string{} : allWorlds.front().folderName;
		}
	};



	programData.ui.menuRenderer.Text("Select world", Colors_White);

	//programData.ui.menuRenderer.Button("Create new world", Colors_Gray, programData.ui.buttonTexture);
	programData.ui.menuRenderer.Text("", Colors_White);

	glm::vec4 customWidgetTransform = {};
	programData.ui.menuRenderer.CustomWidget(171, &customWidgetTransform);

	glui::Frame f({0,0,renderer.windowW, renderer.windowH});


	auto drawButton = [&](glm::vec4 transform, glm::vec4 color,
		const std::string &s)
	{
		return glui::drawButton(renderer, transform, color, s, programData.ui.font, programData.ui.buttonTexture,
			platform::getRelMousePosition(), platform::isLMouseHeld(), platform::isLMouseReleased());
	};

	auto drawBackground = [&]()
	{
		float rezolution = 256;
		glm::vec2 size{renderer.windowW, renderer.windowH};
		size /= 256.f;

		renderer.renderRectangle({0,0, renderer.windowW, renderer.windowH},
			programData.blocksLoader.backgroundTexture, {0.6,0.6,0.6,1}, {}, 0,
			{0,size.y, size.x, 0}
		);

	};

	if (programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].size()
		&& programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].back() == "Play"
		)
	{

		drawBackground();

		// Refresh periodically so newly created/recovered worlds appear without
		// rescanning the filesystem on every rendered frame.
		const auto now = std::chrono::steady_clock::now();
		if (forceCatalogRefresh || now - lastCatalogRefresh > std::chrono::seconds(2))
		{
			refreshCatalog();
		}


		//center
		{
			float ySize = renderer.windowH - customWidgetTransform.y * 2;
			
			if (ySize > 50)
			{
				static int advance = 0;

				glui::Frame f(glui::Box().xCenter().yTop(customWidgetTransform.y).
					yDimensionPixels(ySize).xDimensionPercentage(1)());
				auto fullBox = glui::Box().xLeft().yTop().xDimensionPercentage(1.f).yDimensionPercentage(1.f)();

				float buttonH = customWidgetTransform.w;
				int maxElements = std::max(1, static_cast<int>(fullBox.w / std::max(buttonH, 1.f)));

				advance = std::max(advance, 0);
				int overflow = allWorlds.size() - maxElements;
				if (overflow < 0) { overflow = 0; }
				advance = std::min(advance, overflow);

				//background
				{
					float rezolution = 256;
					glm::vec2 size{fullBox.z, fullBox.w};
					size /= 256.f;

					float padding = advance * 0.4;

					renderer.renderRectangle(fullBox,
						programData.blocksLoader.backgroundTexture, {0.4,0.4,0.4,1}, {}, 0,
						{padding,size.y, size.x + padding, 0}
					);
				}

				//renderer.renderRectangle(glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f),
				//	{1,0,0,1.0});

				glm::vec4 worldBox = fullBox;
				worldBox.w = buttonH;
				worldBox.z -= buttonH;

				for (int i = 0; i < maxElements; i++)
				{
					if (allWorlds.size() <= i + advance) { break; }

					const auto &world = allWorlds[i + advance];
					const auto &worldDifficulty = world.difficultySettings;
					const std::string s = world.folderName + " [" +
						getWorldDifficultyName(worldDifficulty.difficulty) +
						(worldDifficulty.hardcore ? " Hardcore]" : "]");

					if (world.folderName == selected)
					{
						renderer.render9Patch(worldBox,
							20, {0.3,0.3,0.3,0.7}, {}, 0, programData.ui.buttonTexture,
							GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});
					}

					if (renderButton(renderer, {}, worldBox, s, &programData.ui.font))
					{
						selected = world.folderName;
					}

					worldBox.y += buttonH;
				}

				if (allWorlds.empty())
				{
					programData.ui.renderer2d.renderText(
						{fullBox.x + 24.f, fullBox.y + 24.f},
						"No saved worlds yet. Create one below.",
						programData.ui.font, Colors_White, 0.75f);
				}

				auto topButton = glui::Box().yTop().xRight().yDimensionPixels(buttonH).xDimensionPixels(buttonH)();
				auto bottomButton = glui::Box().yBottom().xRight().yDimensionPixels(buttonH).xDimensionPixels(buttonH)();

				if (renderButton(renderer, programData.ui.buttonTexture, topButton)) { advance--; }
				if (renderButton(renderer, programData.ui.buttonTexture, bottomButton)) { advance++; }

			}

		}

		//bottom
		{
			float ySize = customWidgetTransform.y;
			glui::Frame f(glui::Box().xCenter().yBottom().
				yDimensionPixels(ySize).xDimensionPercentage(1)());

			//renderer.renderRectangle(glui::Box().xCenter().yCenter().xDimensionPercentage(1).yDimensionPercentage(1.f),
			//	{0,1,0,1.0});


			//top
			{
				glui::Frame f(glui::Box().xCenter().yTop().
					yDimensionPercentage(0.5).xDimensionPercentage(1)());

				{
					auto leftButton = glui::Box().xLeft().yCenter().xDimensionPercentage(0.5).yDimensionPercentage(1)();
					if (drawButton(shrinkPercentage(leftButton, {0.1,0.05}), Colors_Gray, "Play Selected World"))
					{
						if (selected.size())
						{
							hostServer(selected);
						}

					}

					auto rightButton = glui::Box().xRight().yCenter().xDimensionPercentage(0.5).yDimensionPercentage(1)();
					if (drawButton(shrinkPercentage(rightButton, {0.1,0.05}), Colors_Gray, "Refresh worlds"))
					{
						forceCatalogRefresh = true;
					}


				}

			}

			//bottom
			{
				glui::Frame f(glui::Box().xCenter().yBottom().
					yDimensionPercentage(0.5).xDimensionPercentage(1)());

				{
					auto leftButton = glui::Box().xLeft().yCenter().xDimensionPercentage(0.5).yDimensionPercentage(1)();
					if (drawButton(shrinkPercentage(leftButton, {0.1,0.05}), Colors_Gray, "Create a new world!"))
					{
						programData.ui.menuRenderer.StartManualMenu("Create world");
					}

					auto rightButton = glui::Box().xRight().yCenter().xDimensionPercentage(0.5).yDimensionPercentage(1)();
					if (drawButton(shrinkPercentage(rightButton, {0.1,0.05}), Colors_Gray, "Delete world"))
					{
						if (selected.size())
						{
							programData.ui.menuRenderer.StartManualMenu("Delete world");
						}
					}
				}


			}

		}

		if (!catalogError.empty())
		{
			programData.ui.menuRenderer.Text(catalogError.c_str(), glm::vec4(1, 0.35f, 0.35f, 1));
		}

	}

	static char seed[12] = {};
	static char name[49] = {};
	static int currentIndex = 0; //0 normal, 1 super flat
	static int difficultyIndex = static_cast<int>(WorldDifficulty::Normal);
	static bool hardcore = false;
	static WorldGeneratorSettings settings;
	static gl2d::Texture worldPreviewTexture;
	static WorldGenerator wg;
	if (wg.regionsHeightNoise == 0)
	{
		wg.init();
	}



	programData.ui.menuRenderer.BeginManualMenu("Delete world");

	if (programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].size()
		&& programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].back() == "Delete world"
		)
	{

		if (selected.size())
		{
			drawBackground();

			programData.ui.menuRenderer.Text("Are you sure you want to delete:", Colors_White);
			programData.ui.menuRenderer.Text(selected.c_str(), Colors_White);


			if (programData.ui.menuRenderer.Button("Delete", Colors_Gray, programData.ui.buttonTexture))
			{
				const auto deletePath = std::filesystem::path(USER_CONTENT_PATH "worlds") / selected;

				std::error_code error;
				std::filesystem::remove_all(deletePath, error);
				if (error)
				{
					catalogError = "Could not delete the world: " + error.message();
				}
				else
				{
					selected = {};
					forceCatalogRefresh = true;
					programData.ui.menuRenderer.ExitCurrentMenu();
				}
			}

			if (programData.ui.menuRenderer.Button("Cancel", Colors_Gray, programData.ui.buttonTexture))
			{
				programData.ui.menuRenderer.ExitCurrentMenu();
			}
		}
		else
		{
			programData.ui.menuRenderer.ExitCurrentMenu();
		}

		
	}

	programData.ui.menuRenderer.EndMenu();



	programData.ui.menuRenderer.BeginManualMenu("Create world");

	
	if (programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].size()
		&& programData.ui.menuRenderer.internal.allMenuStacks
		[programData.ui.menuRenderer.internal.currentId].back() == "Create world"
		)
	{

		programData.ui.menuRenderer.temporalViewPort
			= glm::vec4(0, 0, programData.ui.renderer2d.windowW / 2.6f, programData.ui.renderer2d.windowH);

		programData.ui.menuRenderer.Text("Create a new world!", Colors_White);
		

		drawBackground();

		programData.ui.menuRenderer.InputText("Name:", name, sizeof(name),
			Colors_Gray, programData.ui.buttonTexture);

		programData.ui.menuRenderer.InputText("Seed:", seed, sizeof(seed),
			Colors_Gray, programData.ui.buttonTexture);

		programData.ui.menuRenderer.toggleOptions("Difficulty: ",
			"Peaceful|Easy|Normal|Hard", &difficultyIndex, true, Colors_White,
			nullptr, programData.ui.buttonTexture, Colors_Gray,
			"Peaceful disables natural sieges. Easy reduces damage and starvation. "
			"Hard increases damage, starvation and siege size.");
		programData.ui.menuRenderer.ToggleButton("Hardcore (one life)", Colors_White,
			&hardcore, programData.ui.buttonTexture, Colors_Gray);
		if (hardcore)
		{
			difficultyIndex = static_cast<int>(WorldDifficulty::Hard);
			programData.ui.menuRenderer.Text(
				"Hardcore locks Hard difficulty and disables respawning.", Colors_White);
		}
		
		//programData.ui.menuRenderer.Toggle("Super Flat", Colors_Gray, &superFlatWorld, programData.ui.buttonTexture, programData.ui.buttonTexture);

		std::string normalizedName;
		std::string nameError;
		const bool validName = normalizeWorldName(name, normalizedName, &nameError);
		std::filesystem::path finalName = std::filesystem::path(USER_CONTENT_PATH "worlds") / normalizedName;

		if (!validName)
		{
			programData.ui.menuRenderer.Button(nameError.c_str(), {0.6,0.4,0.4,1},
				programData.ui.buttonTexture);
		}
		else if (std::filesystem::exists(finalName))
		{
			programData.ui.menuRenderer.Button("Name already exists!", {0.6,0.4,0.4,1},
				programData.ui.buttonTexture);
		}
		else
		{
			bool create = 0;
			bool createAndPlay = 0;


			if (programData.ui.menuRenderer.Button("Create!", Colors_Gray,
				programData.ui.buttonTexture))
			{
				create = true;
			}

			if (programData.ui.menuRenderer.Button("Create and play", Colors_Gray,
				programData.ui.buttonTexture))
			{
				createAndPlay = true;
			}

			if (create || createAndPlay)
			{
				std::error_code err;
				bool settingsError = false;
				const bool createdDirectory = std::filesystem::create_directory(finalName, err);


				{
					std::ifstream f(RESOURCES_PATH "gameData/worldGenerator/default.wgenerator");
					if (f.is_open())
					{
						std::stringstream buffer;
						buffer << f.rdbuf();
						if (!settings.loadSettings(buffer.str().c_str()))
						{
							settingsError = true;
						}
					}
					else
					{
						settingsError = true;
					}

				}

				if (createdDirectory && !err && !settingsError)
				{
					const int finalSeed = worldSeedFromText(seed, static_cast<int>(time(nullptr)));

					{
						std::ofstream f(finalName / "worldGenSettings.wgenerator");
						settings.seed = finalSeed;
						settings.isSuperFlat = (currentIndex == 1);

						settings.sanitize();
						f << settings.saveSettings();
						settingsError = !f;
					}

					if (!settingsError)
					{
						WorldDifficultySettings difficultySettings;
						difficultySettings.difficulty = static_cast<WorldDifficulty>(
							glm::clamp(difficultyIndex, 0, 3));
						difficultySettings.hardcore = hardcore;
						difficultySettings.sanitize();
						settingsError = !saveWorldDifficultySettings(
							finalName, difficultySettings);
					}

					if (!settingsError && createAndPlay)
					{
						hostServer(normalizedName);
					}
					if (!settingsError)
					{
						selected = normalizedName;
						forceCatalogRefresh = true;
						name[0] = '\0';
						seed[0] = '\0';
						programData.ui.menuRenderer.ExitCurrentMenu();
					}
				}

				if (!createdDirectory || err || settingsError)
				{
					catalogError = err ? "Could not create the world: " + err.message() :
						(!createdDirectory ? "A world with that name already exists." :
							"Could not write the world settings.");
					if (createdDirectory)
					{
						std::error_code cleanupError;
						std::filesystem::remove_all(finalName, cleanupError);
					}
				}
			}

		}
		
		
		//programData.ui.menuRenderer.newColum(11);
		//programData.ui.menuRenderer.newColum(12);

		{
			auto &renderer = programData.ui.renderer2d;
			glui::Frame f({0,0, renderer.windowW, renderer.windowH});

			{
				glui::Frame f(glui::Box().xLeftPerc(0.35).yTopPerc(0.1).yDimensionPercentage(0.8).xDimensionPercentage(0.6)());

				{
					auto fullBox = glui::Box().xLeft().yTop().xDimensionPercentage(1.f).yDimensionPercentage(1.f)();
					

					auto buttonBox = glui::Box().xLeft().yBottom().xDimensionPercentage(1.f).yDimensionPixels(150.f)();
					glui::toggleOptions(renderer, buttonBox, "World type: ", Colors_White,
						"Normal|Super Flat", &currentIndex, true, programData.ui.font,
						programData.ui.buttonTexture, Colors_Gray, platform::getRelMousePosition(),
						platform::isLMouseHeld(), platform::isLMouseReleased());

					auto mapBox = glui::Box().xLeft().yTop().xDimensionPercentage(1.f).yDimensionPercentage(1.f)();
					mapBox.w -= buttonBox.w;

					if (mapBox.w > 0)
					{
						renderer.render9Patch(mapBox, 20,
							Colors_Gray, {}, 0.f, programData.ui.buttonTexture, 
							GL2D_DefaultTextureCoords, {0.2,0.8,0.8,0.2});

						//auto textureBox = mapBox;
						//textureBox = shrinkPercentage(textureBox, {0.6f, 0.6f});
						//
						//worldPreviewTexture.cleanup();
						//wg.applySettings(settings);
						//	
						//wg.generateChunkPreview(worldPreviewTexture, {textureBox.z,textureBox.w}, {});
						//
						//renderer.renderRectangle(textureBox, worldPreviewTexture);

					}

				}


			}


		}


	}
	else
	{
		memset(seed, 0, sizeof(seed));
		memset(name, 0, sizeof(name));
		currentIndex = 0;
		difficultyIndex = static_cast<int>(WorldDifficulty::Normal);
		hardcore = false;
		settings = {};
	}
	
	programData.ui.menuRenderer.EndMenu();


}

void ShadingSettings::normalize()
{
	auto finiteOr = [](float value, float fallback)
	{
		return std::isfinite(value) ? value : fallback;
	};
	auto finiteVectorOr = [&](glm::vec3 value, glm::vec3 fallback)
	{
		for (int component = 0; component < 3; ++component)
		{
			value[component] = finiteOr(value[component], fallback[component]);
		}
		return value;
	};

	waterColor = finiteVectorOr(waterColor, glm::vec3(6, 42, 52) / 255.f);
	underWaterColor = finiteVectorOr(underWaterColor, glm::vec3(0, 17, 25) / 255.f);
	underwaterDarkenStrength = finiteOr(underwaterDarkenStrength, 0.94f);
	underwaterDarkenDistance = finiteOr(underwaterDarkenDistance, 29.f);
	fogGradientUnderWater = finiteOr(fogGradientUnderWater, 1.9f);
	bloomTresshold = finiteOr(bloomTresshold, 0.5f);
	bloomMultiplier = finiteOr(bloomMultiplier, 0.5f);
	lightsStrength = finiteOr(lightsStrength, 1.f);
	exposure = finiteOr(exposure, 0.f);
	fogGradient = finiteOr(fogGradient, 16.f);
	toneMapSaturation = finiteOr(toneMapSaturation, 1.f);
	toneMapVibrance = finiteOr(toneMapVibrance, 1.f);
	toneMapGamma = finiteOr(toneMapGamma, 1.f);
	toneMapShadowBoost = finiteOr(toneMapShadowBoost, 0.f);
	toneMapHighlightBoost = finiteOr(toneMapHighlightBoost, 0.f);
	vignette = finiteOr(vignette, 0.15f);
	toneMapLift = finiteVectorOr(toneMapLift, glm::vec3(0.5f));
	toneMapGain = finiteVectorOr(toneMapGain, glm::vec3(0.5f));

	viewDistance = glm::clamp(viewDistance, 1, 50);
	tonemapper = glm::clamp(tonemapper, 0, 4);
	shadows = glm::clamp(shadows, 0, 2);
	waterType = glm::clamp(waterType, 0, 1);

	waterColor = glm::clamp(waterColor, glm::vec3(0.f), glm::vec3(2.f, 2.f, 2.f));
	underWaterColor = glm::clamp(underWaterColor, glm::vec3(0.f), glm::vec3(2.f, 2.f, 2.f));

	underwaterDarkenStrength = glm::clamp(underwaterDarkenStrength, 0.f, 1.f);
	underwaterDarkenDistance = glm::clamp(underwaterDarkenDistance, 0.f, 40.f);
	fogGradientUnderWater = glm::clamp(fogGradientUnderWater, 0.f, 32.f);
	workerThreadsForBaking = glm::clamp(workerThreadsForBaking, 0, 10);
	lodStrength = glm::clamp(lodStrength, 0, 5);

	bloomTresshold = glm::clamp(bloomTresshold, 0.1f, 1.f);
	bloomMultiplier = glm::clamp(bloomMultiplier, 0.0f, 1.f);
	
	PBR = glm::clamp(PBR, 0, 1);
	bloom = glm::clamp(bloom, 0, 1);
	SSR = glm::clamp(SSR, 0, 1);
	useLights = glm::clamp(useLights, 0, 1);
	lightsStrength = glm::clamp(lightsStrength, 0.1f, 2.f);

	maxLights = glm::clamp(maxLights, 1,100);

	exposure = glm::clamp(exposure, -2.f, 2.f);
	fogGradient = glm::clamp(fogGradient, 0.f, 100.f);


	toneMapSaturation = glm::clamp(toneMapSaturation, 0.f, 2.f);
	toneMapVibrance = glm::clamp(toneMapVibrance, 0.f, 2.f);
	toneMapGamma = glm::clamp(toneMapGamma, 0.1f, 2.f);
	toneMapShadowBoost = glm::clamp(toneMapShadowBoost, -1.f, 1.f);
	toneMapHighlightBoost = glm::clamp(toneMapHighlightBoost, -1.f, 1.f);
	vignette = glm::clamp(vignette, 0.f, 1.f);
	toneMapLift = glm::clamp(toneMapLift, glm::vec3(0.f), glm::vec3(1));
	toneMapGain = glm::clamp(toneMapGain, glm::vec3(0.f), glm::vec3(1));

}

#define GET_STR(x) "c_" #x

#define ADD_TO_RESULT(x) result += ("#define " GET_STR(x) " ") + std::to_string(x) + "\n"
#define ADD_TO_RESULT_VEC3(x) result += ("#define " GET_STR(x) " vec3(") + std::to_string(x.r) + "," + std::to_string(x.g) + "," + std::to_string(x.b) + ")\n"


std::string ShadingSettings::formatIntoGLSLcode()
{
	normalize();

	std::string result;
	result.reserve(500);


	ADD_TO_RESULT(shadows);
	ADD_TO_RESULT(PBR);
	ADD_TO_RESULT(SSR);
	ADD_TO_RESULT(useLights);



	return result;
}
