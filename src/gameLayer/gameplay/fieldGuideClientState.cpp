#include <gameplay/fieldGuide.h>
#include <rendering/UiEngine.h>
#include <platform/platformInput.h>

#include <algorithm>
#include <sstream>
#include <string>

namespace
{
	GuideProgress clientGuideProgress = {};
	bool showGuideCrafts = false;
}

GuideProgress getClientGuideProgress()
{
	GuideProgress result = clientGuideProgress;
	result.sanitize();
	return result;
}

void setClientGuideProgress(GuideProgress progress)
{
	progress.sanitize();
	clientGuideProgress = progress;
}

void resetClientGuideProgress()
{
	clientGuideProgress = {};
	showGuideCrafts = false;
}

void renderFieldGuideUi(UiENgine &ui, int w, int h, bool insideInventory,
	int &currentInventoryTab)
{
	if (!insideInventory || w <= 0 || h <= 0) { return; }

	glui::Frame frame({0, 0, w, h});
	const float minDimension = std::min(w * 0.65f, h * 0.85f);
	const glm::vec4 inventoryBox = glui::Box().xCenter().yCenter()
		.yDimensionPixels(minDimension).xAspectRatio(1.33f)();

	glm::vec4 guideButton = inventoryBox;
	guideButton.z = std::clamp(inventoryBox.z * 0.18f, 126.f, 180.f);
	guideButton.w = std::clamp(inventoryBox.w * 0.055f, 30.f, 42.f);
	guideButton.x += inventoryBox.z - guideButton.z;
	guideButton.y -= guideButton.w * 0.92f;

	if (glui::drawButton(ui.renderer2d, guideButton, Colors_White, "FIELD GUIDE",
		ui.font, ui.buttonTexture, platform::getRelMousePosition(),
		platform::isLMouseHeld(), platform::isLMouseReleased()))
	{
		currentInventoryTab = INVENTORY_TAB_FIELD_GUIDE;
	}

	if (currentInventoryTab != INVENTORY_TAB_FIELD_GUIDE) { return; }

	glm::vec4 panel = inventoryBox;
	panel.x += inventoryBox.z * 0.065f;
	panel.z *= 0.87f;
	panel.y += inventoryBox.w * 0.055f;
	panel.w *= 0.72f;
	ui.renderer2d.render9Patch(panel, 24, {0.25f, 0.19f, 0.11f, 0.96f}, {}, 0.f,
		ui.buttonTexture, GL2D_DefaultTextureCoords, {0.2f, 0.8f, 0.8f, 0.2f});

	const float titleSize = std::clamp(inventoryBox.w / 28.f, 22.f, 34.f);
	const float lineSize = std::clamp(inventoryBox.w / 48.f, 14.f, 21.f);
	const float smallSize = std::clamp(inventoryBox.w / 58.f, 12.f, 18.f);
	const float left = panel.x + panel.z * 0.045f;
	const float top = panel.y + panel.w * 0.055f;

	auto drawText = [&](float x, float y, const std::string &text, float size,
		glm::vec4 color = Colors_White)
	{
		ui.renderer2d.renderText({x, y}, text.c_str(), ui.font, color, size, 3, 0, false);
	};

	drawText(left, top, "MIE FIELD GUIDE", titleSize, {1.f, 0.88f, 0.58f, 1.f});

	glm::vec4 pageButton = panel;
	pageButton.z = std::clamp(panel.z * 0.28f, 150.f, 230.f);
	pageButton.w = std::clamp(panel.w * 0.075f, 28.f, 40.f);
	pageButton.x += panel.z - pageButton.z - panel.z * 0.04f;
	pageButton.y += panel.w - pageButton.w - panel.w * 0.035f;

	if (!showGuideCrafts)
	{
		drawText(left, top + titleSize * 1.55f, "MAIN OBJECTIVES", lineSize,
			{0.92f, 0.82f, 0.62f, 1.f});
		drawText(left, top + titleSize * 2.35f,
			"Each reward can be earned only once.", smallSize,
			{0.86f, 0.84f, 0.78f, 1.f});

		const GuideProgress progress = getClientGuideProgress();
		const auto &definitions = getGuideObjectiveDefinitions();
		const float listTop = top + titleSize * 3.15f;
		const float rowHeight = (pageButton.y - listTop - smallSize * 1.8f) /
			static_cast<float>(definitions.size());

		for (std::size_t i = 0; i < definitions.size(); ++i)
		{
			const auto &definition = definitions[i];
			const float y = listTop + rowHeight * static_cast<float>(i);
			std::string state = "[ ] ";
			glm::vec4 color = Colors_White;
			if (progress.rewarded(definition.objective))
			{
				state = "[DONE] ";
				color = {0.48f, 0.92f, 0.56f, 1.f};
			}
			else if (progress.completed(definition.objective))
			{
				state = "[REWARD PENDING] ";
				color = {0.96f, 0.72f, 0.24f, 1.f};
			}

			drawText(left, y, state + definition.title, lineSize, color);
			std::ostringstream detail;
			detail << definition.description << "  Reward: ";
			if (definition.rewardCount > 1) { detail << definition.rewardCount << "x "; }
			detail << definition.rewardLabel;
			drawText(left + panel.z * 0.025f, y + lineSize * 1.25f,
				detail.str(), smallSize, {0.88f, 0.86f, 0.80f, 1.f});
		}

		drawText(left, pageButton.y - smallSize * 1.25f,
			"Pending rewards are delivered automatically when inventory space is available.",
			smallSize, {0.96f, 0.72f, 0.24f, 1.f});

		if (glui::drawButton(ui.renderer2d, pageButton, Colors_White, "CRAFTING GUIDE",
			ui.font, ui.buttonTexture, platform::getRelMousePosition(),
			platform::isLMouseHeld(), platform::isLMouseReleased()))
		{
			showGuideCrafts = true;
		}
	}
	else
	{
		drawText(left, top + titleSize * 1.55f, "IMPORTANT CRAFTS", lineSize,
			{0.92f, 0.82f, 0.62f, 1.f});

		const char *crafts[] = {
			"WORKBENCH  -  4 wooden planks.",
			"FURNACE  -  8 cobblestone at a workbench.",
			"CHARCOAL  -  1 log becomes 2 charcoal in a furnace.",
			"TORCHES  -  1 charcoal + 1 stick becomes 4 torches.",
			"TIN  -  2 cassiterite + 1 charcoal becomes 1 tin in a furnace.",
			"BRONZE  -  3 copper + 1 tin + 1 charcoal becomes 4 bronze.",
			"BRONZE PICKAXE  -  4 bronze + 3 planks at a workbench.",
			"BRONZE AXE/SHOVEL  -  3 bronze + 3 planks at a workbench.",
			"BRONZE SWORD  -  4 bronze + 1 stick at a workbench."
		};

		float y = top + titleSize * 2.65f;
		const float advance = (pageButton.y - y - smallSize) / 9.f;
		for (const char *craft : crafts)
		{
			drawText(left, y, craft, smallSize, {0.92f, 0.86f, 0.72f, 1.f});
			y += advance;
		}

		if (glui::drawButton(ui.renderer2d, pageButton, Colors_White,
			"BACK TO OBJECTIVES", ui.font, ui.buttonTexture,
			platform::getRelMousePosition(), platform::isLMouseHeld(),
			platform::isLMouseReleased()))
		{
			showGuideCrafts = false;
		}
	}
}
