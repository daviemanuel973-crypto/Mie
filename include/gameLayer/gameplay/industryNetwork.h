#pragma once
#include <gameplay/industryRecipes.h>
#include <gameplay/blocks/chestBlock.h>
#include <array>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <vector>

struct IndustryPosition
{
	int x=0,y=0,z=0;
	bool operator<(const IndustryPosition &o) const
	{ return x!=o.x ? x<o.x : y!=o.y ? y<o.y : z<o.z; }
	bool operator==(const IndustryPosition &o) const { return x==o.x && y==o.y && z==o.z; }
};

struct IndustryAccess
{
	std::function<FurnaceBlock *(IndustryPosition)> machine;
	std::function<ChestBlock *(IndustryPosition)> chest;
	std::function<void(IndustryPosition)> changed;
};

struct IndustryMetrics
{
	std::uint64_t topologyVisits=0, machineUpdates=0, transfers=0;
	std::size_t nodes=0, networks=0;
};

// Server-owner-thread only. Stores positions and routes, never chunk pointers.
// Rebuilding is incremental; simulation pauses safely while topology is dirty.
class IndustryNetwork
{
public:
	void setNode(IndustryPosition pos, std::uint16_t type);
	void removeNode(IndustryPosition pos);
	void wake(IndustryPosition pos);
	void update(float seconds, const IndustryAccess &access);
	void clear();
	bool rebuilding() const { return dirtyTopology; }
	IndustryMetrics metrics() const;
private:
	struct Network
	{
		int kind=0;
		std::vector<IndustryPosition> members, devices, generators, batteries, consumers, extractors, endpoints;
		std::size_t cursor=0, routeCursor=0;
		bool queued=false, cycleChanged=false;
	};
	bool belongs(std::uint16_t type, int kind) const;
	void invalidate();
	void rebuild(std::size_t budget);
	bool run(Network &network, const IndustryAccess &access);
	std::map<IndustryPosition,std::uint16_t> nodes;
	std::vector<Network> networks, building;
	std::map<IndustryPosition,std::array<int,2>> membership, buildingMembership;
	std::array<std::set<IndustryPosition>,2> visited;
	std::deque<IndustryPosition> frontier;
	std::deque<std::size_t> active, buildingActive;
	std::map<IndustryPosition,std::uint16_t>::iterator scan;
	std::map<IndustryPosition,double> lastMachine, lastExtractor;
	bool dirtyTopology=true, buildStarted=false;
	int buildKind=0;
	double clock=0., timer=0.;
	std::size_t networkCursor=0;
	IndustryMetrics counters;
};
