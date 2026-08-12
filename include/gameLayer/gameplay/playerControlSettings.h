#pragma once

#include <cstdint>

enum class PlayerCameraMode : std::uint8_t
{
	FirstPerson = 0,
	ThirdPersonBack = 1,
	ThirdPersonFront = 2,
};

struct PlayerControlSettings
{
	bool autoJump = false;
	bool autoRun = false;
	PlayerCameraMode cameraMode = PlayerCameraMode::FirstPerson;
	float thirdPersonDistance = 4.f;

	void normalize();
	bool operator==(const PlayerControlSettings &other) const;
	bool operator!=(const PlayerControlSettings &other) const { return !(*this == other); }
};

struct MovementAssistInput
{
	bool movingForward = false;
	bool grounded = false;
	bool halfBlockAhead = false;
	bool hasHeadClearance = false;
};

struct MovementAssistOutput
{
	bool jump = false;
	bool run = false;
};

MovementAssistOutput evaluateMovementAssist(const PlayerControlSettings &settings,
	const MovementAssistInput &input);

PlayerControlSettings &getPlayerControlSettings();
void loadPlayerControlSettings();
void savePlayerControlSettings();
void cyclePlayerCameraMode();
const char *getPlayerCameraModeName(PlayerCameraMode mode);
