#include <runtimeSmoke.h>

#include <config.h>
#include <multyPlayer/server.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>

bool hostServer(const std::string &path);

namespace
{
constexpr const char *SMOKE_WORLD_NAME = "__mie_runtime_smoke__";
constexpr int MIN_SMOKE_FRAMES = 180;
// llvmpipe on the low-end CI runner can stay below 6 FPS while baking the first
// visible chunks. Keep the 180-frame gate and allow it enough steady-state time.
constexpr auto DEFAULT_SMOKE_RUNTIME = std::chrono::seconds(3);
constexpr auto SMOKE_STARTUP_GRACE = std::chrono::seconds(90);
constexpr int MAX_CONFIGURED_SMOKE_SECONDS = 3'600;

std::filesystem::path smokeWorldPath()
{
    return std::filesystem::path(USER_CONTENT_PATH) / "worlds" / SMOKE_WORLD_NAME;
}

std::chrono::steady_clock::time_point smokeStarted;
int smokeFrames = 0;
bool smokeBegan = false;
bool smokeClockStarted = false;
std::chrono::seconds minimumSmokeRuntime = DEFAULT_SMOKE_RUNTIME;
std::vector<double> smokeFrameSeconds;

std::chrono::seconds configuredSmokeRuntime()
{
    const char *value = std::getenv("MIE_RUNTIME_SMOKE_SECONDS");
    if (!value || !*value) { return DEFAULT_SMOKE_RUNTIME; }
    char *end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < DEFAULT_SMOKE_RUNTIME.count() ||
        parsed > MAX_CONFIGURED_SMOKE_SECONDS)
    {
        std::cerr << "[runtime-smoke] invalid MIE_RUNTIME_SMOKE_SECONDS; using "
            << DEFAULT_SMOKE_RUNTIME.count() << " seconds\n";
        return DEFAULT_SMOKE_RUNTIME;
    }
    return std::chrono::seconds(parsed);
}

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

double percentile(const std::vector<double> &sorted, double quantile)
{
    if (sorted.empty()) { return 0.0; }
    const double scaled = quantile * static_cast<double>(sorted.size() - 1u);
    const std::size_t lower = static_cast<std::size_t>(scaled);
    const std::size_t upper = std::min(lower + 1u, sorted.size() - 1u);
    const double fraction = scaled - static_cast<double>(lower);
    return sorted[lower] + (sorted[upper] - sorted[lower]) * fraction;
}

void writeSmokeMetrics()
{
    std::vector<double> sorted = smokeFrameSeconds;
    std::sort(sorted.begin(), sorted.end());
    double total = 0.0;
    for (double seconds : sorted) { total += seconds; }
    const double averageSeconds = sorted.empty() ? 0.0 : total / sorted.size();
    const double durationSeconds = smokeClockStarted
        ? std::chrono::duration<double>(std::chrono::steady_clock::now() - smokeStarted).count()
        : 0.0;

    std::ofstream report("mie-runtime-smoke-metrics.json", std::ios::trunc);
    if (!report)
    {
        std::cerr << "[runtime-smoke] could not write frame metrics\n";
        return;
    }
    report << std::fixed << std::setprecision(6)
        << "{\n"
        << "  \"frames\": " << smokeFrames << ",\n"
        << "  \"duration_seconds\": " << durationSeconds << ",\n"
        << "  \"average_fps\": " << (averageSeconds > 0.0 ? 1.0 / averageSeconds : 0.0) << ",\n"
        << "  \"frame_ms_average\": " << averageSeconds * 1000.0 << ",\n"
        << "  \"frame_ms_p50\": " << percentile(sorted, 0.50) * 1000.0 << ",\n"
        << "  \"frame_ms_p95\": " << percentile(sorted, 0.95) * 1000.0 << ",\n"
        << "  \"frame_ms_p99\": " << percentile(sorted, 0.99) * 1000.0 << ",\n"
        << "  \"frame_ms_max\": " << (sorted.empty() ? 0.0 : sorted.back() * 1000.0) << "\n"
        << "}\n";
}
}

bool runtimeSmokeRequested()
{
    const char *value = std::getenv("MIE_RUNTIME_SMOKE_TEST");
    return value && std::string(value) == "1";
}

bool runtimeSmokeReusesExistingWorld()
{
    const char *value = std::getenv("MIE_RUNTIME_SMOKE_REUSE_WORLD");
    return value && std::string(value) == "1";
}

bool beginRuntimeSmokeTest()
{
    const bool reuseWorld = runtimeSmokeReusesExistingWorld();
    if (!reuseWorld)
    {
        removeSmokeWorld();
    }
    else if (!hasPersistedWorldData(smokeWorldPath()))
    {
        std::cerr << "[runtime-smoke] recovery requested without persisted world data\n";
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(smokeWorldPath(), ec);
    if (ec)
    {
        std::cerr << "[runtime-smoke] could not create isolated world: " << ec.message() << "\n";
        return false;
    }

    std::cout << "[runtime-smoke] starting real local server/client world"
        << (reuseWorld ? " in recovery mode\n" : "\n");
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
    smokeFrameSeconds.clear();
    smokeBegan = true;
    minimumSmokeRuntime = configuredSmokeRuntime();
    // Start the stability window after the first complete gameplay frame.
    // Slow software renderers can spend tens of seconds in their first frame,
    // which is startup cost rather than a steady-state hang.
    smokeClockStarted = false;
    return true;
}

RuntimeSmokeFrameResult runtimeSmokeFramePassed(double frameSeconds)
{
    if (!smokeBegan) { return RuntimeSmokeFrameResult::failed; }
    ++smokeFrames;
    const auto now = std::chrono::steady_clock::now();
    if (!smokeClockStarted)
    {
        smokeStarted = now;
        smokeClockStarted = true;
    }
    else if (std::isfinite(frameSeconds) && frameSeconds >= 0.0)
    {
        smokeFrameSeconds.push_back(frameSeconds);
    }
    const auto elapsed = now - smokeStarted;

    if (!isServerRunning())
    {
        std::cerr << "[runtime-smoke] server stopped after " << smokeFrames << " frames\n";
        return RuntimeSmokeFrameResult::failed;
    }
    if (elapsed > minimumSmokeRuntime + SMOKE_STARTUP_GRACE)
    {
        std::cerr << "[runtime-smoke] timed out before stable runtime gate\n";
        return RuntimeSmokeFrameResult::failed;
    }
    if (smokeFrames >= MIN_SMOKE_FRAMES && elapsed >= minimumSmokeRuntime)
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
    writeSmokeMetrics();
    removeSmokeWorld();
    smokeBegan = false;
    smokeClockStarted = false;

    if (runtimePassed && persisted)
    {
        std::cout << "[runtime-smoke] PASS: server, client, gameplay frames and world persistence\n";
        return 0;
    }
    std::cerr << "[runtime-smoke] FAIL\n";
    return 3;
}
