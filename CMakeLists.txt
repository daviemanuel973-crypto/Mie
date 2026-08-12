#https://github.com/meemknight/cmakeSetup

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GCC 15 defaults C sources to GNU C23, but the bundled raudio version predates
# C23 and defines false/true as enum constants. Keep vendor C code on GNU C17.
if (NOT MSVC)
	set(CMAKE_C_STANDARD 17)
	set(CMAKE_C_STANDARD_REQUIRED ON)
	set(CMAKE_C_EXTENSIONS ON)
endif()

project(ourCraft)

include(CTest)

option(OURCRAFT_ENABLE_AVX2 "Enable AVX2 optimizations" ON)
option(OURCRAFT_INSTALLABLE "Build with relocatable/installable resources" OFF)
option(OURCRAFT_LOW_END_BUILD "Tune runtime defaults for low-end Linux hardware" OFF)
option(OURCRAFT_FLATPAK "Enable Flatpak runtime compatibility renderer" OFF)
set(OURCRAFT_VERSION "0.7.1" CACHE STRING "Mie Survival semantic version")

# Keep the MSVC runtime statically linked for the Windows build.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (OURCRAFT_ENABLE_AVX2)
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
	endif()
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Configure Assimp
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_NO_EXCEPTIONS ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_EXPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MTL_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_MTL_EXPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_EXPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTFB_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTFB_EXPORTER OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
option(ASSIMP_BUILD_STATIC_LIB "If the library shall be build as static library." ON)
set(ASSIMP_BUILD_ZLIB "1" CACHE STRING "If zlib is used, static linking. Specify Zlib < 1.3")
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

add_subdirectory(thirdparty/glfw-3.3.7)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/stb_image)
add_subdirectory(thirdparty/stb_truetype)
add_subdirectory(thirdparty/gl2d)
add_subdirectory(thirdparty/raudio)

# GCC 14+ promotes this old raudio API mismatch to an error. The bundled dr_*
# APIs use an ABI-compatible 64-bit unsigned type on x86_64; preserve the
# legacy source as a warning instead of modifying vendor code.
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(raudio PRIVATE -Wno-error=incompatible-pointer-types)
endif()

add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/imgui-docking)
add_subdirectory(thirdparty/FastNoiseSIMD)
add_subdirectory(thirdparty/FastNoise2)
add_subdirectory(thirdparty/enet-1.3.18)
add_subdirectory(thirdparty/glui)
add_subdirectory(thirdparty/assimp-5.2.4)
add_subdirectory(thirdparty/magic_enum-0.9.3)
add_subdirectory(thirdparty/safeSave)
add_subdirectory(thirdparty/profilerLib)
add_subdirectory(thirdparty/zstd-1.5.5/build/cmake)
add_subdirectory(noiseTool)

# PBRTool is a Windows-only development utility (it includes Windows.h).
# Keep it available on Windows builds without making Linux/Flatpak compile it.
if (WIN32)
	add_subdirectory(PBRTool)
endif()

# MY_SOURCES is a list of all the source files for my game.
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shared/*.cpp")

add_executable("${CMAKE_PROJECT_NAME}")
set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 17)

target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES} ${SHARED_SOURCES})

if (OURCRAFT_LOW_END_BUILD)
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC OURCRAFT_LOW_END_BUILD=1)
	# Keep floating-point behavior conservative while still favoring Release throughput.
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE
			$<$<CONFIG:Release>:-O3;-fno-math-errno;-fno-trapping-math;-fomit-frame-pointer>)
	endif()
endif()

if (OURCRAFT_FLATPAK)
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC OURCRAFT_FLATPAK=1)
endif()

# The original Windows-oriented source relies on a few declarations being
# visible before template definitions and contains a Win32-only debug picker.
# Force-include a tiny compatibility header only for GNU/Linux builds.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND UNIX AND NOT APPLE)
	target_compile_options("${CMAKE_PROJECT_NAME}" PRIVATE
		-include "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/linuxLegacyCompat.h")
endif()

if (OURCRAFT_INSTALLABLE)
	include(GNUInstallDirs)

	if (WIN32)
		# Windows installers are relocatable. The installer/shortcut launches from
		# the application directory, so the legacy literal-concatenation resource
		# code can safely resolve resources/ without baking a build-machine path.
		target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="resources/")
		target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC
			INTERNAL_BUILD=0
			MIE_VERSION="${OURCRAFT_VERSION}"
			USER_CONTENT_PATH="resources/"
			USER_SETTINGS_PATH="playerSettings/"
			USER_ERROR_LOG_PATH="errorLogs.txt")
		set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES OUTPUT_NAME "Mie")
		install(TARGETS "${CMAKE_PROJECT_NAME}" RUNTIME DESTINATION ".")
		install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/" DESTINATION "resources")
	else()
		# Flatpak has a fixed /app prefix, while writable save/settings data is
		# provided by its launcher in the app's private XDG data directory.
		set(OURCRAFT_RESOURCE_DIR "${CMAKE_INSTALL_FULL_DATADIR}/ourcraft/resources/")
		target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="${OURCRAFT_RESOURCE_DIR}")
		target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC
			INTERNAL_BUILD=0
			MIE_VERSION="${OURCRAFT_VERSION}"
			USER_CONTENT_PATH="resources/"
			USER_SETTINGS_PATH="playerSettings/"
			USER_ERROR_LOG_PATH="errorLogs.txt")
		set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES OUTPUT_NAME "ourcraft")
		install(TARGETS "${CMAKE_PROJECT_NAME}" RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
		install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/ourcraft/resources")
	endif()
else()
	# Developer build: load assets directly from the checkout.
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC
		MIE_VERSION="${OURCRAFT_VERSION}"
		USER_CONTENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
		USER_SETTINGS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/playerSettings/"
		USER_ERROR_LOG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/errorLogs.txt")
endif()

if(MSVC)
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _CRT_SECURE_NO_WARNINGS)
	set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

if(WIN32)
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _WIN32)
endif()

target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/shared/")
target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/")
target_include_directories("${CMAKE_PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/")

target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glm glfw glad stb_image stb_truetype
	gl2d raudio imgui fastNoiseSIMD FastNoise2 enet glui
	magic_enum safeSave profilerLib
	libzstd_static assimp)

if (WIN32 AND OURCRAFT_INSTALLABLE)
	add_executable(MieLauncher WIN32
		windows/MieLauncher.cpp
		src/gameLayer/updateProtocol.cpp)
	target_include_directories(MieLauncher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	target_compile_definitions(MieLauncher PRIVATE
		UNICODE _UNICODE
		MIE_VERSION="${OURCRAFT_VERSION}")
	target_link_libraries(MieLauncher PRIVATE winhttp bcrypt)
	set_target_properties(MieLauncher PROPERTIES OUTPUT_NAME "MieLauncher")
	install(TARGETS MieLauncher RUNTIME DESTINATION ".")
endif()

if (BUILD_TESTING)
	add_executable(playerPersistenceTests
		tests/playerPersistenceTests.cpp
		src/gameLayer/multyPlayer/playerSaveFormat.cpp)
	target_include_directories(playerPersistenceTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	add_test(NAME player-persistence-format COMMAND playerPersistenceTests)

	add_executable(worldCatalogTests
		tests/worldCatalogTests.cpp
		src/gameLayer/worldCatalog.cpp)
	target_include_directories(worldCatalogTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	add_test(NAME world-catalog COMMAND worldCatalogTests)

	add_executable(updateProtocolTests
		tests/updateProtocolTests.cpp
		src/gameLayer/updateProtocol.cpp)
	target_include_directories(updateProtocolTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	add_test(NAME update-protocol COMMAND updateProtocolTests)

	add_executable(siegeDirectorTests
		tests/siegeDirectorTests.cpp
		src/gameLayer/gameplay/siege.cpp
		src/gameLayer/gameplay/worldTime.cpp)
	target_include_directories(siegeDirectorTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	add_test(NAME siege-director COMMAND siegeDirectorTests)

	add_executable(nativeSystemsTests
		tests/nativeSystemsTests.cpp
		src/gameLayer/native/contentRegistry.cpp
		src/gameLayer/native/gameplayFoundation.cpp
		src/gameLayer/native/gameplayScheduler.cpp
		src/gameLayer/native/prototypeMachine.cpp
		src/gameLayer/native/worldSchema.cpp)
	target_include_directories(nativeSystemsTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	add_test(NAME native-systems COMMAND nativeSystemsTests)

	add_executable(playerControlSettingsTests
		tests/playerControlSettingsTests.cpp
		src/gameLayer/gameplay/playerControlSettings.cpp)
	target_include_directories(playerControlSettingsTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/"
		"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/safeSave/include/")
	target_compile_definitions(playerControlSettingsTests PRIVATE
		USER_SETTINGS_PATH="${CMAKE_CURRENT_BINARY_DIR}/testPlayerSettings/")
	target_link_libraries(playerControlSettingsTests PRIVATE safeSave)
	add_test(NAME player-control-settings COMMAND playerControlSettingsTests)

	add_executable(entityStatsTests
		tests/entityStatsTests.cpp
		src/gameLayer/gameplay/entityStats.cpp)
	target_include_directories(entityStatsTests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
	target_link_libraries(entityStatsTests PRIVATE glm)
	add_test(NAME entity-stats COMMAND entityStatsTests)

	set_tests_properties(
		player-persistence-format
		world-catalog
		update-protocol
		siege-director
		native-systems
		player-control-settings
		entity-stats
		PROPERTIES TIMEOUT 60)
endif()
