#pragma once

#include <cstdint>
#include <multyPlayer/packet.h>

static_assert(headerUpdateWorldTime == 50, "Field Guide protocol requires stable legacy packet numbering");
static_assert(headerUpdateWorldDifficulty == 51, "World difficulty protocol numbering changed");
static_assert(headerUpdateGuideProgress == 52, "Field Guide protocol numbering changed");
