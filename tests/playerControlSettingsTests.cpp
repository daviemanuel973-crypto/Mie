#include <gameplay/playerControlSettings.h>

#include <iostream>
#include <limits>

namespace
{
	int failures = 0;
	void check(bool condition, const char *message)
	{
		if (!condition)
		{
			std::cerr << "FAIL: " << message << '\n';
			++failures;
		}
	}
}

int main()
{
	PlayerControlSettings settings;
	settings.autoJump = true;
	settings.autoRun = true;

	MovementAssistInput clearHalfBlock;
	clearHalfBlock.movingForward = true;
	clearHalfBlock.grounded = true;
	clearHalfBlock.halfBlockAhead = true;
	clearHalfBlock.hasHeadClearance = true;
	MovementAssistOutput output = evaluateMovementAssist(settings, clearHalfBlock);
	check(output.jump && output.run,
		"an enabled clear half block activates automatic jump and run");

	MovementAssistInput blockedHalfBlock = clearHalfBlock;
	blockedHalfBlock.hasHeadClearance = false;
	output = evaluateMovementAssist(settings, blockedHalfBlock);
	check(!output.jump && output.run,
		"head obstruction suppresses automatic jump without disabling automatic run");

	settings.autoJump = false;
	settings.autoRun = false;
	output = evaluateMovementAssist(settings, clearHalfBlock);
	check(!output.jump && !output.run, "both movement assists are independently disableable");

	settings.cameraMode = static_cast<PlayerCameraMode>(99);
	settings.thirdPersonDistance = std::numeric_limits<float>::quiet_NaN();
	settings.normalize();
	check(settings.cameraMode == PlayerCameraMode::FirstPerson &&
		settings.thirdPersonDistance == 4.f,
		"invalid persisted camera settings fall back safely");

	check(std::string(getPlayerCameraModeName(PlayerCameraMode::FirstPerson)) == "First person" &&
		std::string(getPlayerCameraModeName(PlayerCameraMode::ThirdPersonBack)) ==
			"Third person back" &&
		std::string(getPlayerCameraModeName(PlayerCameraMode::ThirdPersonFront)) ==
			"Third person front", "all three camera modes have stable labels");

	if (failures != 0)
	{
		std::cerr << failures << " player control test(s) failed\n";
		return 1;
	}
	std::cout << "player control settings tests passed\n";
	return 0;
}
