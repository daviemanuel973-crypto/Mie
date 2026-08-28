#include "otherPlatformFunctions.h"
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "Shell32.lib")
#endif
#else
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#undef max
#undef min


//https://stackoverflow.com/questions/21421074/how-to-create-a-full-screen-window-on-the-current-monitor-with-glfw
GLFWmonitor* getCurrentMonitor(GLFWwindow* window)
{
	int nmonitors, i;
	int wx, wy, ww, wh;
	int mx, my, mw, mh;
	int overlap, bestoverlap;
	GLFWmonitor* bestmonitor;
	GLFWmonitor** monitors;
	const GLFWvidmode* mode;

	bestoverlap = 0;
	bestmonitor = NULL;

	glfwGetWindowPos(window, &wx, &wy);
	glfwGetWindowSize(window, &ww, &wh);
	monitors = glfwGetMonitors(&nmonitors);

	for (i = 0; i < nmonitors; i++)
	{
		mode = glfwGetVideoMode(monitors[i]);
		glfwGetMonitorPos(monitors[i], &mx, &my);
		mw = mode->width;
		mh = mode->height;

		overlap =
			std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) *
			std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));

		if (bestoverlap < overlap)
		{
			bestoverlap = overlap;
			bestmonitor = monitors[i];
		}
	}

	return bestmonitor;
}

bool openPathWithDefaultApplication(const char *path)
{
	if (!path || !path[0]) { return false; }

#ifdef _WIN32
	const auto result = reinterpret_cast<INT_PTR>(
		ShellExecuteA(nullptr, "open", path, nullptr, nullptr, SW_SHOWNORMAL));
	return result > 32;
#else
	// Double-fork so the game does not retain a zombie after xdg-open/open exits.
	const pid_t launcher = fork();
	if (launcher < 0) { return false; }
	if (launcher == 0)
	{
		const pid_t opener = fork();
		if (opener < 0) { _exit(127); }
		if (opener == 0)
		{
#if defined(__APPLE__)
			execlp("open", "open", path, static_cast<char *>(nullptr));
#else
			execlp("xdg-open", "xdg-open", path, static_cast<char *>(nullptr));
#endif
			_exit(127);
		}
		_exit(0);
	}

	int status = 0;
	pid_t waited = 0;
	do
	{
		waited = waitpid(launcher, &status, 0);
	}
	while (waited < 0 && errno == EINTR);

	return waited == launcher && WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}
