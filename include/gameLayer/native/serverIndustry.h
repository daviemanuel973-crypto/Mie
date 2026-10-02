#pragma once
#include <gameplay/industryNetwork.h>
struct ServerChunkStorer;
void resetServerIndustry();
void updateServerIndustry(ServerChunkStorer &chunks,float seconds);
IndustryMetrics getServerIndustryMetrics();
bool industrySmokeVerified();
