#include "platformInput.h"

platform::Button keyBoard[platform::Button::BUTTONS_COUNT];
platform::Button leftMouse;
platform::Button rightMouse;

platform::ControllerButtons controllerButtons;
std::string typedInput;

int platform::isKeyHeld(int key)
{
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }

	return keyBoard[key].held;
}

int platform::isKeyPressedOn(int key)
{
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }

	return keyBoard[key].pressed;
}

int platform::isKeyReleased(int key)
{
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }

	return keyBoard[key].released;
}

int platform::isKeyTyped(int key)
{
	if (key < Button::A || key >= Button::BUTTONS_COUNT) { return 0; }

	return keyBoard[key].typed;
}

int platform::isLMousePressed()
{
	return leftMouse.pressed;
}

int platform::isRMousePressed()
{
	return rightMouse.pressed;
}

int platform::isLMouseReleased()
{
	return leftMouse.released;
}

int platform::isRMouseReleased()
{
	return rightMouse.released;
}


int platform::isLMouseHeld()
{
	return leftMouse.held;
}

int platform::isRMouseHeld()
{
	return rightMouse.held;
}

platform::ControllerButtons platform::getControllerButtons()
{
	return platform::isFocused() ? controllerButtons : platform::ControllerButtons{};
}

std::string platform::getTypedInput()
{
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
	for (int i = 0; i < platform::Button::BUTTONS_COUNT; i++)
	{
		updateButton(keyBoard[i], deltaTime);
	}

	updateButton(leftMouse, deltaTime);
	updateButton(rightMouse, deltaTime);

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
					updateButton(controllerButtons.buttons[button], deltaTime);
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

	scrollY = 0;
}



float platform::getScroll()
{
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
}
