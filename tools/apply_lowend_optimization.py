from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected exactly one match, got {count}: {old[:100]!r}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# Build-time low-end profile.
replace_once(
    "CMakeLists.txt",
    'option(OURCRAFT_ENABLE_AVX2 "Enable AVX2 optimizations" ON)\noption(OURCRAFT_INSTALLABLE "Build with relocatable/installable resources" OFF)\n',
    'option(OURCRAFT_ENABLE_AVX2 "Enable AVX2 optimizations" ON)\noption(OURCRAFT_INSTALLABLE "Build with relocatable/installable resources" OFF)\noption(OURCRAFT_LOW_END_BUILD "Tune runtime defaults for low-end Linux hardware" OFF)\n',
)
replace_once(
    "CMakeLists.txt",
    'target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES} ${SHARED_SOURCES})\n\n',
    '''target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES} ${SHARED_SOURCES})\n\nif (OURCRAFT_LOW_END_BUILD)\n\ttarget_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC OURCRAFT_LOW_END_BUILD=1)\n\t# Keep floating-point behavior conservative while still favoring Release throughput.\n\tif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")\n\t\ttarget_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE\n\t\t\t$<$<CONFIG:Release>:-O3;-fno-math-errno;-fno-trapping-math;-fomit-frame-pointer>)\n\tendif()\nendif()\n\n''',
)

# Fresh-install rendering defaults. Existing users can apply the same values from the UI button.
replace_once(
    "include/gameLayer/rendering/renderSettings.h",
    '''\tint viewDistance = 15;\n\tint tonemapper = 0;\n\tint shadows = 0;\n\tint waterType = 1;\n\tint workerThreadsForBaking = 2; //MOVE TODO\n\tint lodStrength = 1; //MOVE TODO\n\tint PBR = 1;\n\tint maxLights = 40;\n\tint useLights = 1;\n\tfloat lightsStrength = 1.f;\n\tbool FXAA = 1;\n''',
    '''#if defined(OURCRAFT_LOW_END_BUILD)\n\tint viewDistance = 5;\n\tint tonemapper = 0;\n\tint shadows = 0;\n\tint waterType = 0;\n\tint workerThreadsForBaking = 1;\n\tint lodStrength = 5;\n\tint PBR = 0;\n\tint maxLights = 8;\n\tint useLights = 1;\n\tfloat lightsStrength = 1.f;\n\tbool FXAA = 1;\n#else\n\tint viewDistance = 15;\n\tint tonemapper = 0;\n\tint shadows = 0;\n\tint waterType = 1;\n\tint workerThreadsForBaking = 2; //MOVE TODO\n\tint lodStrength = 1; //MOVE TODO\n\tint PBR = 1;\n\tint maxLights = 40;\n\tint useLights = 1;\n\tfloat lightsStrength = 1.f;\n\tbool FXAA = 1;\n#endif\n''',
)
replace_once(
    "include/gameLayer/rendering/renderSettings.h",
    '\tint bloom = 1;\n\n\tint SSR = 1;\n',
    '''#if defined(OURCRAFT_LOW_END_BUILD)\n\tint bloom = 0;\n\n\tint SSR = 0;\n#else\n\tint bloom = 1;\n\n\tint SSR = 1;\n#endif\n''',
)
replace_once(
    "include/gameLayer/rendering/renderSettings.h",
    'bool checkIfShadingSettingsChangedForShaderReloads();\n\nvoid saveShadingSettings();',
    'bool checkIfShadingSettingsChangedForShaderReloads();\n\nvoid applyLowEndPerformancePreset(ProgramData &programData);\n\nvoid saveShadingSettings();',
)

# Lower-cost FXAA and HBAO disabled in the dedicated low-end binary.
replace_once(
    "include/gameLayer/rendering/renderer.h",
    '''\t\tint ITERATIONS = 12;\n\t\tfloat quaityMultiplier = 0.8;\n\t\tfloat SUBPIXEL_QUALITY = 0.95;\n''',
    '''#if defined(OURCRAFT_LOW_END_BUILD)\n\t\tint ITERATIONS = 6;\n\t\tfloat quaityMultiplier = 0.65;\n\t\tfloat SUBPIXEL_QUALITY = 0.60;\n#else\n\t\tint ITERATIONS = 12;\n\t\tfloat quaityMultiplier = 0.8;\n\t\tfloat SUBPIXEL_QUALITY = 0.95;\n#endif\n''',
)
replace_once(
    "include/gameLayer/rendering/renderer.h",
    '\tbool ssao = 1;\n',
    '''#if defined(OURCRAFT_LOW_END_BUILD)\n\tbool ssao = 0;\n#else\n\tbool ssao = 1;\n#endif\n''',
)

# Expose a one-click preset for existing saves/settings.
replace_once(
    "src/gameLayer/rendering/renderSettings.cpp",
    '#define DEFAULT_COLOR_PICKER_TRANSPARENT programData.ui.buttonTexture, programData.ui.buttonTexture, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}\n\n',
    '''#define DEFAULT_COLOR_PICKER_TRANSPARENT programData.ui.buttonTexture, programData.ui.buttonTexture, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}, {(float)0x7F / 255.0f, (float)0x7F / 255.0f, (float)0x7F / 255.0f, 0.65}\n\nvoid applyLowEndPerformancePreset(ProgramData &programData)\n{\n\tauto &settings = getShadingSettings();\n\tsettings.viewDistance = 5;\n\tsettings.lodStrength = 5;\n\tsettings.workerThreadsForBaking = 1;\n\tsettings.shadows = 0;\n\tsettings.waterType = 0;\n\tsettings.PBR = 0;\n\tsettings.SSR = 0;\n\tsettings.bloom = 0;\n\tsettings.bloomMultiplier = 0.f;\n\tsettings.maxLights = 8;\n\tsettings.useLights = 1;\n\tsettings.FXAA = 1;\n\n\tprogramData.renderer.frustumCulling = true;\n\tprogramData.renderer.sortChunks = true;\n\tprogramData.renderer.ssao = false;\n\tprogramData.renderer.fxaaData.ITERATIONS = 6;\n\tprogramData.renderer.fxaaData.quaityMultiplier = 0.65f;\n\tprogramData.renderer.fxaaData.SUBPIXEL_QUALITY = 0.60f;\n}\n\n''',
)
replace_once(
    "src/gameLayer/rendering/renderSettings.cpp",
    '\tprogramData.ui.menuRenderer.Text("Rendering Settings...", Colors_White);\n\n',
    '''\tprogramData.ui.menuRenderer.Text("Rendering Settings...", Colors_White);\n\n\tif (programData.ui.menuRenderer.Button("Performance preset (Intel HD / low-end)", Colors_Gray,\n\t\tprogramData.ui.buttonTexture))\n\t{\n\t\tapplyLowEndPerformancePreset(programData);\n\t}\n\n''',
)

# Avoid GPU allocations/readbacks for disabled effects on low-end hardware.
replace_once(
    "src/gameLayer/rendering/renderer.cpp",
    '\tif (filterBloomSize != glm::ivec2(screenX, screenY))\n',
    '\tif (getShadingSettings().bloom && filterBloomSize != glm::ivec2(screenX, screenY))\n',
)
replace_once(
    "src/gameLayer/rendering/renderer.cpp",
    '\tfboHBAO.updateSize(screenX / 2, screenY / 2);\n',
    '\tif (ssao)\n\t{\n\t\tfboHBAO.updateSize(screenX / 2, screenY / 2);\n\t}\n',
)
replace_once(
    "src/gameLayer/rendering/renderer.cpp",
    '#pragma region get automatic exposure\n\tif(1)\n',
    '#pragma region get automatic exposure\n#if !defined(OURCRAFT_LOW_END_BUILD)\n\tif(1)\n',
)
replace_once(
    "src/gameLayer/rendering/renderer.cpp",
    '\t\tadaptiveExposure.update(deltaTime, averageLuminosity);\n\n\t}\n#pragma endregion\n',
    '''\t\tadaptiveExposure.update(deltaTime, averageLuminosity);\n\n\t}\n#else\n\t// A full-screen mipmap chain plus async readback every other frame is expensive\n\t// on integrated GPUs. Keep a stable exposure in the low-end profile.\n\taverageLuminosity = 0.5f;\n\tadaptiveExposure.currentExposure = 1.6f;\n#endif\n#pragma endregion\n''',
)

# Linux bindless compatibility through Mesa Zink and stable 60/30 FPS pacing.
replace_once(
    "src/platform/glfwMain.cpp",
    '#include <errorReporting.h>\n',
    '''#include <errorReporting.h>\n#include <thread>\n#include <cstdlib>\n#include <cstring>\n\n#ifdef __linux__\n#include <unistd.h>\n#endif\n''',
)
replace_once("src/platform/glfwMain.cpp", 'int main()\n{', 'int main(int argc, char **argv)\n{')
replace_once(
    "src/platform/glfwMain.cpp",
    '''\tint w = 500;\n\tint h = 500;\n\twind = glfwCreateWindow(w, h, "geam", nullptr, nullptr);\n\tglfwMakeContextCurrent(wind);\n\t//glfwSwapInterval(1);\n''',
    '''#if defined(OURCRAFT_LOW_END_BUILD)\n\tint w = 1280;\n\tint h = 720;\n#else\n\tint w = 500;\n\tint h = 500;\n#endif\n\twind = glfwCreateWindow(w, h, "geam", nullptr, nullptr);\n\tglfwMakeContextCurrent(wind);\n\tglfwSwapInterval(0);\n''',
)
replace_once(
    "src/platform/glfwMain.cpp",
    '''\tif (!GLAD_GL_NV_bindless_texture)\n\t{\n\t\tstd::cout << "Error, Bindless texture extension not supported!\\nUsually integrated GPUs don't support this extension, this will be fixed in the future.\\n";\n\t\tstd::cout << "Press enter to try anyway...\\n";\n\t\tsystem("pause");\n\t}\n''',
    '''\tif (glGetTextureHandleARB == nullptr || glMakeTextureHandleResidentARB == nullptr)\n\t{\n#ifdef __linux__\n\t\tconst char *mesaDriver = std::getenv("MESA_LOADER_DRIVER_OVERRIDE");\n\t\tif (!mesaDriver || std::strcmp(mesaDriver, "zink") != 0)\n\t\t{\n\t\t\tstd::cerr << "Native OpenGL does not expose ARB_bindless_texture; retrying through Mesa Zink.\\n";\n\t\t\tglfwDestroyWindow(wind);\n\t\t\twind = nullptr;\n\t\t\tglfwTerminate();\n\t\t\tif (setenv("MESA_LOADER_DRIVER_OVERRIDE", "zink", 1) != 0)\n\t\t\t{\n\t\t\t\tstd::cerr << "Could not enable the Zink compatibility path.\\n";\n\t\t\t\treturn 1;\n\t\t\t}\n\t\t\texecv("/proc/self/exe", argv);\n\t\t\tstd::cerr << "Could not restart the game through Zink.\\n";\n\t\t\treturn 1;\n\t\t}\n\t\tstd::cerr << "ARB_bindless_texture is unavailable even through Zink.\\n";\n\t\treturn 1;\n#else\n\t\tstd::cout << "Error, ARB_bindless_texture is not supported by this GPU/driver.\\n";\n#ifdef _WIN32\n\t\tsystem("pause");\n#endif\n\t\treturn 1;\n#endif\n\t}\n''',
)
replace_once(
    "src/platform/glfwMain.cpp",
    '\twhile (!glfwWindowShouldClose(wind))\n\t{\n',
    '''\twhile (!glfwWindowShouldClose(wind))\n\t{\n#if defined(OURCRAFT_LOW_END_BUILD)\n\t\tauto frameStartLowEnd = std::chrono::steady_clock::now();\n#endif\n''',
)
replace_once(
    "src/platform/glfwMain.cpp",
    '\t\tglfwPollEvents();\n\n\t#pragma endregion\n',
    '''\t\tglfwPollEvents();\n\n#if defined(OURCRAFT_LOW_END_BUILD)\n\t\t// Favor stable frame pacing over oscillating between 30 and 60 FPS.\n\t\tauto frameWorkEnd = std::chrono::steady_clock::now();\n\t\tdouble workMs = std::chrono::duration<double, std::milli>(frameWorkEnd - frameStartLowEnd).count();\n\t\tstatic double smoothedWorkMs = 16.67;\n\t\tstatic int slowFrames = 0;\n\t\tstatic int fastFrames = 0;\n\t\tstatic bool stable30Mode = false;\n\n\t\tsmoothedWorkMs = smoothedWorkMs * 0.95 + workMs * 0.05;\n\t\tif (!stable30Mode)\n\t\t{\n\t\t\tif (smoothedWorkMs > 20.5)\n\t\t\t{\n\t\t\t\tif (++slowFrames >= 90)\n\t\t\t\t{\n\t\t\t\t\tstable30Mode = true;\n\t\t\t\t\tslowFrames = 0;\n\t\t\t\t}\n\t\t\t}\n\t\t\telse\n\t\t\t{\n\t\t\t\tslowFrames = std::max(0, slowFrames - 2);\n\t\t\t}\n\t\t}\n\t\telse\n\t\t{\n\t\t\tif (smoothedWorkMs < 13.5)\n\t\t\t{\n\t\t\t\tif (++fastFrames >= 300)\n\t\t\t\t{\n\t\t\t\t\tstable30Mode = false;\n\t\t\t\t\tfastFrames = 0;\n\t\t\t\t}\n\t\t\t}\n\t\t\telse { fastFrames = 0; }\n\t\t}\n\n\t\tauto targetFrame = !platform::isFocused() ? std::chrono::microseconds(66667) :\n\t\t\t(stable30Mode ? std::chrono::microseconds(33333) : std::chrono::microseconds(16667));\n\t\tstd::this_thread::sleep_until(frameStartLowEnd + targetFrame);\n#endif\n\n\t#pragma endregion\n''',
)

# Flatpak always builds the performance-tuned variant.
replace_once(
    "flatpak/io.github.daviemanuel973.Mie.yml",
    '-DOURCRAFT_INSTALLABLE=ON -DOURCRAFT_ENABLE_AVX2=OFF',
    '-DOURCRAFT_INSTALLABLE=ON -DOURCRAFT_ENABLE_AVX2=OFF -DOURCRAFT_LOW_END_BUILD=ON',
)
replace_once(
    ".github/workflows/linux-flatpak-ci.yml",
    '      - agent/survival-linux-flatpak\n',
    '      - agent/survival-linux-flatpak\n      - agent/linux-lowend-performance\n',
)

Path("docs/LOW_END_PERFORMANCE.md").write_text(
    """# Low-end Linux performance profile\n\nTarget reference hardware:\n\n- Intel Core i5-7200U (2C/4T)\n- Intel HD Graphics 620\n- 8 GB RAM\n- HDD\n- Linux x86_64 / Flatpak\n\nThe profile targets stable 30 FPS first and up to 60 FPS at 1280x720 when GPU/CPU headroom permits. FPS is an engineering target until benchmarked on the reference machine.\n\n## Changes\n\n- Default view distance reduced from 15 to 5 (chunk matrix from 30x30 to 10x10).\n- Aggressive terrain LOD (strength 5).\n- One extra chunk-baking worker to leave CPU time for the game/server thread.\n- Cheap water, shadows off, PBR off, bloom off, SSR off, HBAO/SSAO off.\n- Point lights limited from 40 to 8.\n- Lower-cost FXAA.\n- Bloom buffers are not allocated while bloom is disabled.\n- HBAO buffers are not resized/allocated while SSAO is disabled.\n- Automatic exposure GPU mipmap/readback is skipped in the low-end build.\n- Adaptive 60/30 FPS frame pacing with a 15 FPS background cap.\n- Linux automatically retries via Mesa Zink when native OpenGL lacks ARB_bindless_texture (important for Intel Mesa).\n- One-click Performance preset remains available in Rendering settings.\n\nAll gameplay/survival systems remain enabled. Expensive effects can still be re-enabled manually.\n""",
    encoding="utf-8",
)

print("Low-end optimization source transformation completed.")
