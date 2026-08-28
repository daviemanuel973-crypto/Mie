#!/usr/bin/env python3
from pathlib import Path

GLFW = Path("src/platform/glfwMain.cpp")
text = GLFW.read_text(encoding="utf-8")

old_include = '#include "gameLayer.h"\n#include <fstream>\n'
new_include = '#include "gameLayer.h"\n#include "runtimeSmoke.h"\n#include <fstream>\n'
if text.count(old_include) != 1:
    raise RuntimeError("glfw include marker changed")
text = text.replace(old_include, new_include, 1)

old_init = '''#pragma region initGame\n\tif (!initGame())\n\t{\n\t\treturn 0;\n\t}\n#pragma endregion\n\n\n\t//long lastTime = clock();\n'''
new_init = '''#pragma region initGame\n\tif (!initGame())\n\t{\n\t\treturn 0;\n\t}\n\n\tconst bool runtimeSmoke = runtimeSmokeRequested();\n\tif (runtimeSmoke && !beginRuntimeSmokeTest())\n\t{\n\t\tcloseGame();\n\t\treturn finishRuntimeSmokeTest(false);\n\t}\n#pragma endregion\n\n\n\t//long lastTime = clock();\n'''
if text.count(old_init) != 1:
    raise RuntimeError("glfw init marker changed")
text = text.replace(old_init, new_init, 1)

old_logic = '''\t\tif (!gameLogic(augmentedDeltaTime))\n\t\t{\n\t\t\tcloseGame();\n\t\t\treturn 0;\n\t\t}\n\n\t#pragma endregion\n'''
new_logic = '''\t\tif (!gameLogic(augmentedDeltaTime))\n\t\t{\n\t\t\tcloseGame();\n\t\t\treturn runtimeSmoke ? finishRuntimeSmokeTest(false) : 0;\n\t\t}\n\n\t\tif (runtimeSmoke)\n\t\t{\n\t\t\tconst RuntimeSmokeFrameResult smokeResult = runtimeSmokeFramePassed();\n\t\t\tif (smokeResult != RuntimeSmokeFrameResult::running)\n\t\t\t{\n\t\t\t\tcloseGame();\n\t\t\t\treturn finishRuntimeSmokeTest(smokeResult == RuntimeSmokeFrameResult::passed);\n\t\t\t}\n\t\t}\n\n\t#pragma endregion\n'''
if text.count(old_logic) != 1:
    raise RuntimeError("glfw gameLogic marker changed")
text = text.replace(old_logic, new_logic, 1)
GLFW.write_text(text, encoding="utf-8")

Path("include/gameLayer/runtimeSmoke.h").write_text(r'''#pragma once

enum class RuntimeSmokeFrameResult
{
    running,
    passed,
    failed,
};

bool runtimeSmokeRequested();
bool beginRuntimeSmokeTest();
RuntimeSmokeFrameResult runtimeSmokeFramePassed();
int finishRuntimeSmokeTest(bool runtimePassed);
''', encoding="utf-8")

Path("src/gameLayer/runtimeSmoke.cpp").write_text(r'''#include <runtimeSmoke.h>

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
''', encoding="utf-8")

Path(".github/workflows/runtime-smoke.yml").write_text(r'''name: Runtime Smoke

on:
  pull_request:
    paths:
      - 'VERSION'
      - 'CMakeLists.txt'
      - 'src/**'
      - 'include/**'
      - 'shared/**'
      - 'resources/**'
      - '.github/workflows/runtime-smoke.yml'
  workflow_dispatch:

permissions:
  contents: read

concurrency:
  group: runtime-smoke-${{ github.event.pull_request.head.ref || github.ref_name }}
  cancel-in-progress: true

jobs:
  real-server-client-world:
    runs-on: ubuntu-24.04
    timeout-minutes: 20
    steps:
      - name: Checkout
        uses: actions/checkout@v6

      - name: Install native runtime dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            cmake ninja-build xvfb mesa-vulkan-drivers libgl1-mesa-dri \
            libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
            libwayland-dev libxkbcommon-dev libzstd-dev

      - name: Configure low-end native build
        run: |
          cmake -S . -B build-runtime-smoke -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DOURCRAFT_ENABLE_AVX2=OFF \
            -DOURCRAFT_LOW_END_BUILD=ON \
            -DBUILD_TESTING=OFF

      - name: Build Mie
        run: cmake --build build-runtime-smoke --target ourCraft --parallel 2

      - name: Run real server/client/world smoke
        shell: bash
        run: |
          set -o pipefail
          rm -rf resources/worlds/__mie_runtime_smoke__
          MIE_RUNTIME_SMOKE_TEST=1 \
          MIE_FRAME_LIMIT=30 \
          MESA_LOADER_DRIVER_OVERRIDE=zink \
          LIBGL_ALWAYS_SOFTWARE=1 \
          GALLIUM_DRIVER=llvmpipe \
          timeout 90s xvfb-run -a -s '-screen 0 1280x720x24' \
            ./build-runtime-smoke/ourCraft 2>&1 | tee runtime-smoke.log
          test ! -e resources/worlds/__mie_runtime_smoke__
          grep -F '[runtime-smoke] PASS:' runtime-smoke.log

      - name: Upload smoke log on failure
        if: failure()
        uses: actions/upload-artifact@v4
        with:
          name: runtime-smoke-failure-log
          path: runtime-smoke.log
          if-no-files-found: warn
          retention-days: 3
''', encoding="utf-8")

print("v0.9.6 runtime smoke integration staged")
