#include "platformInput.h"
#include <cmath>

platform::Button keyBoard[platform::Button::BUTTONS_COUNT];
platform::Button leftMouse;
platform::Button rightMouse;

platform::ControllerButtons controllerButtons;
std::string typedInput;

namespace
{
	bool inputStateUpdatedForCurrentFrame = false;
	float lastInputDeltaTime = 1.f / 60.f;

	void ensureInputStateUpdated()
	{
		if (!inputStateUpdatedForCurrentFrame)
		{
			platform::internal::updateAllButtons(lastInputDeltaTime);
		}
	}
}

int platform::isKeyHeld(int key)
{
	if (!platform::isFocused()) { return 0; }
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }
	ensureInputStateUpdated();
	return keyBoard[key].held;
}

int platform::isKeyPressedOn(int key)
{
	if (!platform::isFocused()) { return 0; }
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }
	ensureInputStateUpdated();
	return keyBoard[key].pressed;
}

int platform::isKeyReleased(int key)
{
	if (!platform::isFocused()) { return 0; }
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }
	ensureInputStateUpdated();
	return keyBoard[key].released;
}

int platform::isKeyTyped(int key)
{
	if (!platform::isFocused()) { return 0; }
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }
	ensureInputStateUpdated();
	return keyBoard[key].typed;
}

int platform::isLMousePressed()
{
	if (!platform::isFocused()) { return 0; }
	ensureInputStateUpdated();
	return leftMouse.pressed;
}

int platform::isRMousePressed()
{
	if (!platform::isFocused()) { return 0; }
	ensureInputStateUpdated();
	return rightMouse.pressed;
}

int platform::isLMouseReleased()
{
	if (!platform::isFocused()) { return 0; }
	ensureInputStateUpdated();
	return leftMouse.released;
}

int platform::isRMouseReleased()
{
	if (!platform::isFocused()) { return 0; }
	ensureInputStateUpdated();
	return rightMouse.released;
}


int platform::isLMouseHeld()
{
	if (!platform::isFocused()) { return 0; }
	ensureInputStateUpdated();
	return leftMouse.held;
}

int platform::isRMouseHeld()
{
	if (!platform::isFocused()) { return 0; }
	ensureInputStateUpdated();
	return rightMouse.held;
}

platform::ControllerButtons platform::getControllerButtons()
{
	if (!platform::isFocused()) { return platform::ControllerButtons{}; }
	ensureInputStateUpdated();
	return controllerButtons;
}

std::string platform::getTypedInput()
{
	if (!platform::isFocused()) { return {}; }
	ensureInputStateUpdated();
	return typedInput;
}

void platform::internal::setButtonState(int button, int newState)
{
	processEventButton(keyBoard[button], newState);
}

void platform::internal::setLeftMouseState(int newState)
{
	processEventButton(leftMouse, newState);
}

void platform::internal::setRightMouseState(int newState)
{
	processEventButton(rightMouse, newState);
}


float scrollY = 0;
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	(void)window;
	(void)xoffset;
	scrollY += static_cast<float>(yoffset);
}

void platform::internal::updateAllButtons(float deltaTime)
{
	if (std::isfinite(deltaTime) && deltaTime >= 0.f && deltaTime <= 1.f)
	{
		lastInputDeltaTime = deltaTime;
	}

	// The platform loop also calls this after gameLogic. Once the first input
	// query has processed GLFW events for the current frame, that trailing call
	// must not advance repeat timers or clear edge-triggered state a second time.
	if (inputStateUpdatedForCurrentFrame)
	{
		return;
	}

	for (int i = 0; i < platform::Button::BUTTONS_COUNT; i++)
	{
		updateButton(keyBoard[i], lastInputDeltaTime);
	}

	updateButton(leftMouse, lastInputDeltaTime);
	updateButton(rightMouse, lastInputDeltaTime);

	bool controllerPolled = false;
	for(int joystick = 0; joystick <= GLFW_JOYSTICK_LAST; joystick++)
	{
		if(glfwJoystickPresent(joystick) && glfwJoystickIsGamepad(joystick))
		{
			GLFWgamepadstate state;

			if (glfwGetGamepadState(joystick, &state))
			{
				for (int button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; button++)
				{
					processEventButton(controllerButtons.buttons[button], state.buttons[button] == GLFW_PRESS);
					updateButton(controllerButtons.buttons[button], lastInputDeltaTime);
				}

				controllerButtons.LT = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
				controllerButtons.RT = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];

				controllerButtons.LStick.x = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
				controllerButtons.LStick.y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];

				controllerButtons.RStick.x = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
				controllerButtons.RStick.y = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

				controllerPolled = true;
				break;
			}
		}
	}

	if (!controllerPolled)
	{
		controllerButtons.setAllToZero();
	}

	inputStateUpdatedForCurrentFrame = true;
}



float platform::getScroll()
{
	if (!platform::isFocused()) { return 0.f; }
	ensureInputStateUpdated();
	return scrollY;
}

void platform::internal::resetInputsToZero()
{
	resetTypedInput();

	for (int i = 0; i < platform::Button::BUTTONS_COUNT; i++)
	{
		resetButtonToZero(keyBoard[i]);
	}

	resetButtonToZero(leftMouse);
	resetButtonToZero(rightMouse);
	
	scrollY = 0;

	controllerButtons.setAllToZero();
}

void platform::internal::addToTypedInput(char c)
{
	typedInput += c;
}

void platform::internal::resetTypedInput()
{
	typedInput.clear();
	scrollY = 0.f;
	inputStateUpdatedForCurrentFrame = false;
}
