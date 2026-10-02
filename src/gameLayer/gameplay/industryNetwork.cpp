#include <gameplay/industryNetwork.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	std::vector<IndustryPosition> neighbours(IndustryPosition p)
	{
		std::vector<IndustryPosition> result;
		const int directions[6][3]={{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
		for (const auto &d:directions)
		{
			const auto x=static_cast<long long>(p.x)+d[0],y=static_cast<long long>(p.y)+d[1],z=static_cast<long long>(p.z)+d[2];
			if (x>=INT32_MIN && x<=INT32_MAX && y>=0 && y<256 && z>=INT32_MIN && z<=INT32_MAX)
				result.push_back({static_cast<int>(x),static_cast<int>(y),static_cast<int>(z)});
		}
		return result;
	}
	bool chestType(std::uint16_t t)
	{ return t==woddenChest || t==goblinChest || t==copperChest || t==ironChest || t==silverChest || t==goldChest; }
	bool moveOne(Item &from, Item &to)
	{
		if (!from.type || !from.counter) { return false; }
		Item copy=to;
		if (to.type && (to.type!=from.type || to.metaData!=from.metaData || to.counter>=copy.getStackSize())) { return false; }
		if (!to.type) { to=from; to.counter=1; }
		else { ++to.counter; }
		if (!--from.counter) { from={}; }
		return true;
	}
}

bool IndustryNetwork::belongs(std::uint16_t t,int kind) const
{
	return kind==0 ? t==powerCable || t==fuelGenerator || t==energyAccumulator || isElectricConsumer(t) :
		t==itemPipe || t==itemExtractor || t==furnace || t==fuelGenerator || isElectricConsumer(t) || chestType(t);
}

void IndustryNetwork::invalidate()
{ dirtyTopology=true; buildStarted=false; building.clear(); buildingMembership.clear(); buildingActive.clear(); frontier.clear(); visited[0].clear(); visited[1].clear(); }

void IndustryNetwork::setNode(IndustryPosition p,std::uint16_t t)
{
	if (!belongs(t,0) && !belongs(t,1)) { removeNode(p); return; }
	auto it=nodes.find(p);
	if (it!=nodes.end() && it->second==t) { return; }
	nodes[p]=t; invalidate();
}

void IndustryNetwork::removeNode(IndustryPosition p)
{
	if (nodes.erase(p)) { invalidate(); }
	lastMachine.erase(p); lastExtractor.erase(p);
}

void IndustryNetwork::wake(IndustryPosition p)
{
	auto it=membership.find(p);
	if (it==membership.end()) { return; }
	for (int i:it->second) if (i>=0 && static_cast<std::size_t>(i)<networks.size() && !networks[i].queued)
	{ networks[i].queued=true; active.push_back(static_cast<std::size_t>(i)); }
}

void IndustryNetwork::rebuild(std::size_t budget)
{
	if (!buildStarted) { scan=nodes.begin(); buildKind=0; buildStarted=true; }
	while (budget)
	{
		if (frontier.empty())
		{
			while (scan!=nodes.end() && budget)
			{
				--budget; ++counters.topologyVisits;
				const auto entry=*scan++;
				if (!belongs(entry.second,buildKind) || visited[buildKind].count(entry.first)) { continue; }
				building.push_back({}); building.back().kind=buildKind; building.back().queued=true; buildingActive.push_back(building.size()-1);
				visited[buildKind].insert(entry.first); frontier.push_back(entry.first); break;
			}
			if (frontier.empty() && scan==nodes.end())
			{
				if (++buildKind<2) { scan=nodes.begin(); continue; }
				networks=std::move(building); membership.swap(buildingMembership); active.swap(buildingActive);
				dirtyTopology=false; networkCursor=0; return;
			}
			if (frontier.empty()) { return; }
		}
		if (!budget) { return; }
		--budget; ++counters.topologyVisits;
		const auto p=frontier.front(); frontier.pop_front();
		const auto t=nodes.at(p); auto &n=building.back(); n.members.push_back(p);
		auto member=buildingMembership.try_emplace(p,std::array<int,2>{-1,-1}).first;
		member->second[buildKind]=static_cast<int>(building.size()-1);
		if (buildKind==0 && t!=powerCable) { n.devices.push_back(p); }
		if (t==fuelGenerator) { n.generators.push_back(p); }
		if (t==energyAccumulator) { n.batteries.push_back(p); }
		if (isElectricConsumer(t)) { n.consumers.push_back(p); }
		if (t==itemExtractor) { n.extractors.push_back(p); }
		if (chestType(t) || t==furnace || t==fuelGenerator || isElectricConsumer(t)) { n.endpoints.push_back(p); }
		for (auto next:neighbours(p))
		{
			const auto it=nodes.find(next);
			if (it!=nodes.end() && belongs(it->second,buildKind) && visited[buildKind].insert(next).second) { frontier.push_back(next); }
		}
	}
}

bool IndustryNetwork::run(Network &n,const IndustryAccess &a)
{
	bool changed=false;
	if (n.kind==0)
	{
		// Route energy only to processors that have a fitting recipe/output. Buffer
		// subtraction and addition happen together on the authoritative owner thread.
		auto distribute=[&](const std::vector<IndustryPosition> &sources,const std::vector<IndustryPosition> &destinations,bool storage)
		{
			std::size_t source=0, visits=0; unsigned sent=0;
			for (std::size_t di=0;di<destinations.size();++di)
			{
				const auto p=destinations[(di+n.routeCursor)%destinations.size()];
				if (++visits>256) { break; }
				auto *d=a.machine(p); if (!d) { continue; }
				if (!storage && !industryOutputFits(*d,findIndustryProcess(*d))) { continue; }
				unsigned need=std::min(storage ? 100u : 20u,industryEnergyCapacity(d->blockType())-d->energyUnits);
				while (need && source<sources.size() && visits<512)
				{
					++visits;
					const auto sp=sources[source]; auto *s=a.machine(sp);
					if (!s || s==d || !s->energyUnits || sent>=100u) { ++source; sent=0; continue; }
					const auto moved=std::min(need,std::min(s->energyUnits,100u-sent));
					s->energyUnits-=moved; d->energyUnits+=moved; need-=moved;
					sent+=moved;
					a.changed(sp); a.changed(p); changed|=moved>0;
				}
			}
		};
		distribute(n.generators,n.consumers,false);
		distribute(n.batteries,n.consumers,false);
		distribute(n.generators,n.batteries,true);
		++n.routeCursor;
		const std::size_t count=std::min<std::size_t>(128,n.devices.size());
		for (std::size_t i=0;i<count;++i)
		{
			const auto p=n.devices[n.cursor++];
			if (n.cursor==n.devices.size()) { n.cursor=0; }
			auto *m=a.machine(p); if (!m) { continue; }
			auto stamp=lastMachine.try_emplace(p,clock-0.25f).first;
			const float dt=static_cast<float>(std::clamp(clock-stamp->second,0.,1.)); stamp->second=clock;
			++counters.machineUpdates;
			const auto result=m->tick(dt);
			if (result.changed) { a.changed(p); changed=true; }
		}
	}
	else
	{
		if (n.extractors.empty()) { return false; }
		const auto count=std::min<std::size_t>(32,n.extractors.size());
		for (std::size_t i=0;i<count;++i)
		{
			const auto p=n.extractors[n.cursor++]; if (n.cursor==n.extractors.size()) { n.cursor=0; }
			auto stamp=lastExtractor.try_emplace(p,clock-1.f).first;
			if (clock-stamp->second<1.f) { changed=true; continue; }
			stamp->second=clock;
			auto *extractor=a.machine(p); if (!extractor) { continue; }
			const auto filter=extractor->items[0].type;
			bool moved=false; std::size_t attempts=0;
			// An extractor adjacent to a processor owns that output port. It
			// must not pull finished goods back out of an adjacent receiving chest.
			bool processorSource=false;
			for (auto source:neighbours(p)) if (auto *m=a.machine(source))
				if (m->blockType()==furnace || isElectricConsumer(m->blockType())) { processorSource=true; break; }
			for (auto source:neighbours(p))
			{
				if (moved) { break; }
				Item *slots=nullptr; int size=0;
				if (auto *c=a.chest(source)) { if (processorSource) { continue; } slots=c->items; size=CHEST_CAPACITY; }
				else if (auto *m=a.machine(source)) { slots=&m->items[FURNACE_OUTPUT_SLOT]; size=1; }
				for (int s=0;s<size && !moved;++s)
				{
					if (!slots[s].type || (filter && slots[s].type!=filter)) { continue; }
					for (int priority=0;priority<2 && !moved;++priority)
					for (std::size_t di=0;di<n.endpoints.size();++di)
					{
						if (++attempts>256 || moved) { break; }
						const auto destination=n.endpoints[(di+n.routeCursor)%n.endpoints.size()];
						if (destination==source) { continue; }
						if ((a.chest(destination)!=nullptr) != (priority==1)) { continue; }
						if (auto *d=a.chest(destination))
						{
							for (auto &slot:d->items) if (moveOne(slots[s],slot)) { moved=true; break; }
						}
						else if (auto *d=a.machine(destination))
						{
							for (int slot=0;slot<FURNACE_OUTPUT_SLOT;++slot)
								if (d->canPlaceInSlot(slots[s],slot) && moveOne(slots[s],d->items[slot])) { moved=true; break; }
						}
						if (moved) { a.changed(source); a.changed(destination); wake(source); wake(destination); ++counters.transfers; changed=true; }
					}
				}
			}
		}
	}
	if (n.kind==1) { n.routeCursor+=256; }
	// A large component keeps its cycle alive until every device/extractor has
	// been visited. Idle components disappear from simulation until a mutation.
	n.cycleChanged|=changed;
	if (n.cursor==0) { const bool result=n.cycleChanged; n.cycleChanged=false; return result; }
	return true;
}

void IndustryNetwork::update(float seconds,const IndustryAccess &a)
{
	if (!std::isfinite(seconds) || seconds<=0.f || !a.machine || !a.chest || !a.changed) { return; }
	clock+=std::min(seconds,1.f); timer+=std::min(seconds,1.f);
	if (dirtyTopology) { rebuild(256); return; }
	if (timer<0.25f) { return; } timer=std::fmod(timer,0.25f);
	const auto count=std::min<std::size_t>(4,active.size());
	for (std::size_t i=0;i<count;++i)
	{
		const auto index=active.front(); active.pop_front();
		auto &n=networks[index]; n.queued=false;
		if (run(n,a) && !n.queued) { n.queued=true; active.push_back(index); }
	}
}

void IndustryNetwork::clear() { *this=IndustryNetwork{}; }
IndustryMetrics IndustryNetwork::metrics() const
{ auto m=counters; m.nodes=nodes.size(); m.networks=networks.size(); return m; }
