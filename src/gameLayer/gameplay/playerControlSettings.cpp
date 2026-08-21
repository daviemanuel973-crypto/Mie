#include <gameplay/playerControlSettings.h>

#include <algorithm>
#include <cmath>
#include <safeSave.h>

namespace
{
	PlayerControlSettings settings;
}

void PlayerControlSettings::normalize()
{
	const auto cameraValue = static_cast<unsigned int>(cameraMode);
	if (cameraValue > static_cast<unsigned int>(PlayerCameraMode::ThirdPersonFront))
	{
		cameraMode = PlayerCameraMode::FirstPerson;
	}
	if (!std::isfinite(thirdPersonDistance)) { thirdPersonDistance = 4.f; }
	thirdPersonDistance = std::clamp(thirdPersonDistance, 2.f, 8.f);
}

bool PlayerControlSettings::operator==(const PlayerControlSettings &other) const
{
	return autoJump == other.autoJump && autoRun == other.autoRun &&
		cameraMode == other.cameraMode && thirdPersonDistance == other.thirdPersonDistance;
}

MovementAssistOutput evaluateMovementAssist(const PlayerControlSettings &settingsToUse,
	const MovementAssistInput &input)
{
	MovementAssistOutput output;
	const bool traversingHalfBlock = input.movingForward && input.halfBlockAhead &&
		input.hasHeadClearance;
	output.run = settingsToUse.autoRun && input.movingForward;
	output.jump = settingsToUse.autoJump && input.grounded && traversingHalfBlock;
	return output;
}

bool updateBufferedJump(BufferedJumpState &state, float deltaTime,
	bool grounded, bool jumpPressed, float coyoteSeconds, float bufferSeconds)
{
	if (!std::isfinite(deltaTime) || deltaTime < 0.f) { deltaTime = 0.f; }
	if (!std::isfinite(coyoteSeconds) || coyoteSeconds < 0.f) { coyoteSeconds = 0.f; }
	if (!std::isfinite(bufferSeconds) || bufferSeconds < 0.f) { bufferSeconds = 0.f; }

	state.coyoteSecondsRemaining = grounded ? coyoteSeconds :
		std::max(0.f, state.coyoteSecondsRemaining - deltaTime);
	state.bufferedSecondsRemaining = jumpPressed ? bufferSeconds :
		std::max(0.f, state.bufferedSecondsRemaining - deltaTime);

	if (state.coyoteSecondsRemaining > 0.f && state.bufferedSecondsRemaining > 0.f)
	{
		state.coyoteSecondsRemaining = 0.f;
		state.bufferedSecondsRemaining = 0.f;
		return true;
	}
	return false;
}

PlayerControlSettings &getPlayerControlSettings()
{
	return settings;
}

void loadPlayerControlSettings()
{
	settings = {};
	sfs::SafeSafeKeyValueData data;
	if (sfs::safeLoad(data, USER_SETTINGS_PATH "playerControls", 0) == sfs::noError)
	{
		data.getBool("autoJump", settings.autoJump);
		data.getBool("autoRun", settings.autoRun);
		int cameraMode = static_cast<int>(settings.cameraMode);
		data.getInt("cameraMode", cameraMode);
		settings.cameraMode = static_cast<PlayerCameraMode>(cameraMode);
		data.getFloat("thirdPersonDistance", settings.thirdPersonDistance);
	}
	settings.normalize();
}

void savePlayerControlSettings()
{
	settings.normalize();
	sfs::SafeSafeKeyValueData data;
	data.setBool("autoJump", settings.autoJump);
	data.setBool("autoRun", settings.autoRun);
	data.setInt("cameraMode", static_cast<int>(settings.cameraMode));
	data.setFloat("thirdPersonDistance", settings.thirdPersonDistance);
	sfs::safeSave(data, USER_SETTINGS_PATH "playerControls", 0);
}

void cyclePlayerCameraMode()
{
	const auto next = (static_cast<unsigned int>(settings.cameraMode) + 1u) % 3u;
	settings.cameraMode = static_cast<PlayerCameraMode>(next);
	savePlayerControlSettings();
}

const char *getPlayerCameraModeName(PlayerCameraMode mode)
{
	switch (mode)
	{
		case PlayerCameraMode::FirstPerson: return "First person";
		case PlayerCameraMode::ThirdPersonBack: return "Third person back";
		case PlayerCameraMode::ThirdPersonFront: return "Third person front";
	}
	return "First person";
}
