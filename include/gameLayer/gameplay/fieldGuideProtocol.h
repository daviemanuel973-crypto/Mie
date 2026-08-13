#pragma once

#include <cstdint>
#include <multyPlayer/packet.h>

static_assert(headerUpdateWorldTime == 50, "Field Guide protocol requires stable legacy packet numbering");
inline constexpr std::uint32_t headerUpdateGuideProgress = 51;
