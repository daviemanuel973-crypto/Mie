#pragma once
#include <GLFW/glfw3.h>

GLFWmonitor* getCurrentMonitor(GLFWwindow* window);

// Opens a file or directory with the desktop's default application.
// Returns false only when the launcher itself could not be started.
bool openPathWithDefaultApplication(const char *path);
