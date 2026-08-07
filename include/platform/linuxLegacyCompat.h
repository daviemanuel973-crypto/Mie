#pragma once

#ifndef _WIN32

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <glm/glm.hpp>

// Declarations used by template code before their definitions. MSVC accepted
// the old include order; GCC performs stricter two-phase lookup.
int getRandomNumber(std::minstd_rand &rng, int min, int max);
float getRandomNumberFloat(std::minstd_rand &rng, float min, float max);
bool getRandomChance(std::minstd_rand &rng, float chance);
std::array<glm::ivec2, 9> *getChunkNeighboursOffsets();
float computeRestantTimer(std::uint64_t older, std::uint64_t newer);

// A tiny compatibility surface for an optional ImGui debug file picker that
// was written directly against Win32. On Linux the editable path field remains
// available; pressing the Windows-only dialog button simply returns false.
using HWND = void *;
using DWORD = std::size_t;

struct OPENFILENAMEA
{
	DWORD lStructSize = 0;
	HWND hwndOwner = nullptr;
	char *lpstrFile = nullptr;
	DWORD nMaxFile = 0;
	const char *lpstrInitialDir = nullptr;
	const char *lpstrFilter = nullptr;
	DWORD nFilterIndex = 0;
	DWORD Flags = 0;
};

inline void ZeroMemory(void *destination, std::size_t size)
{
	std::memset(destination, 0, size);
}

inline int GetOpenFileNameA(OPENFILENAMEA *)
{
	return 0;
}

constexpr DWORD OFN_PATHMUSTEXIST = 0;
constexpr DWORD OFN_FILEMUSTEXIST = 0;
constexpr DWORD OFN_HIDEREADONLY = 0;

#ifndef MAXSHORT
#define MAXSHORT SHRT_MAX
#endif

#endif
