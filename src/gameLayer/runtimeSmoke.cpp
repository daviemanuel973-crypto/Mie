#include <runtimeSmoke.h>

#include <config.h>
#include <multyPlayer/server.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

bool hostServer(const std::string &path);

namespace
{
constexpr const char *SMOKE_WORLD_NAME = "__mie_runtime_smoke__";
constexpr int MIN_SMOKE_FRAMES = 180;
constexpr auto MIN_SMOKE_RUNTIME = std::chrono::seconds(3);
constexpr auto MAX_SMOKE_RUNTIME = std::chrono::seconds(30);

std::filesystem::path smokeWorldPath()
{
    return std::filesystem::path(USER_CONTENT_PATH) / "worlds" / SMOKE_WORLD_NAME;
}

std::chrono::steady_clock::time_point smokeStarted;
int smokeFrames = 0;
bool smokeBegan = false;

bool hasPersistedWorldData(const std::filesystem::path &root)
{
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || ec) { return false; }
    for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end && !ec; it.increment(ec))
    {
        if (it->is_regular_file(ec) && !ec && it->file_size(ec) > 0 && !ec) { return true; }
    }
    return false;
}

void removeSmokeWorld()
{
    std::error_code ec;
    std::filesystem::remove_all(smokeWorldPath(), ec);
    if (ec)
    {
        std::cerr << "[runtime-smoke] cleanup warning: " << ec.message() << "\n";
    }
}
}

bool runtimeSmokeRequested()
{
    const char *value = std::getenv("MIE_RUNTIME_SMOKE_TEST");
    return value && std::string(value) == "1";
}

bool beginRuntimeSmokeTest()
{
    removeSmokeWorld();
    std::error_code ec;
    std::filesystem::create_directories(smokeWorldPath(), ec);
    if (ec)
    {
        std::cerr << "[runtime-smoke] could not create isolated world: " << ec.message() << "\n";
        return false;
    }

    std::cout << "[runtime-smoke] starting real local server/client world\n";
    if (!hostServer(SMOKE_WORLD_NAME))
    {
        std::cerr << "[runtime-smoke] hostServer failed\n";
        return false;
    }
    if (!isServerRunning())
    {
        std::cerr << "[runtime-smoke] server stopped during startup\n";
        return false;
    }

    smokeFrames = 0;
    smokeBegan = true;
    smokeStarted = std::chrono::steady_clock::now();
    return true;
}

RuntimeSmokeFrameResult runtimeSmokeFramePassed()
{
    if (!smokeBegan) { return RuntimeSmokeFrameResult::failed; }
    ++smokeFrames;
    const auto elapsed = std::chrono::steady_clock::now() - smokeStarted;

    if (!isServerRunning())
    {
        std::cerr << "[runtime-smoke] server stopped after " << smokeFrames << " frames\n";
        return RuntimeSmokeFrameResult::failed;
    }
    if (elapsed > MAX_SMOKE_RUNTIME)
    {
        std::cerr << "[runtime-smoke] timed out before stable runtime gate\n";
        return RuntimeSmokeFrameResult::failed;
    }
    if (smokeFrames >= MIN_SMOKE_FRAMES && elapsed >= MIN_SMOKE_RUNTIME)
    {
        std::cout << "[runtime-smoke] runtime gate reached after " << smokeFrames << " frames\n";
        return RuntimeSmokeFrameResult::passed;
    }
    return RuntimeSmokeFrameResult::running;
}

int finishRuntimeSmokeTest(bool runtimePassed)
{
    const bool persisted = hasPersistedWorldData(smokeWorldPath());
    if (!persisted)
    {
        std::cerr << "[runtime-smoke] no persisted world data was produced\n";
    }
    removeSmokeWorld();
    smokeBegan = false;

    if (runtimePassed && persisted)
    {
        std::cout << "[runtime-smoke] PASS: server, client, gameplay frames and world persistence\n";
        return 0;
    }
    std::cerr << "[runtime-smoke] FAIL\n";
    return 3;
}
