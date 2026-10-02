#include <gameplay/industryRecipes.h>
#include <algorithm>
#include <cmath>

namespace
{
	struct Definition { std::uint16_t machine; FurnaceRecipeDefinition recipe; };
	const Definition definitions[] = {
		{oreCrusher, {{{{copperOre,1,false}}}, {copperPowder,1}, 5.f}},
		{oreCrusher, {{{{leadOre,1,false}}}, {leadPowder,1}, 5.f}},
		{oreCrusher, {{{{ironOre,1,false}}}, {ironPowder,1}, 6.f}},
		{oreCrusher, {{{{silverOre,1,false}}}, {silverPowder,1}, 6.f}},
		{oreCrusher, {{{{goldOre,1,false}}}, {goldPowder,1}, 7.f}},
		{metalPress, {{{{copperIngot,1,false}}}, {copperPlate,2}, 4.f}},
		{metalPress, {{{{ironIngot,1,false}}}, {ironPlate,2}, 5.f}},
		{woodSawmill, {{{{woodLog,1,true}}}, {wooden_plank,6}, 4.f}},
	};

	bool inputsMatch(const FurnaceBlock &m, const FurnaceRecipeDefinition &r)
	{
		bool used[3] = {};
		for (const auto &needed : r.inputs)
		{
			if (!needed.type) { break; }
			bool found = false;
			for (int s=0; s<3; ++s)
			{
				const auto &item=m.items[s];
				if (!used[s] && item.metaData.empty() && item.counter>=needed.count &&
					(needed.anyWoodLog ? isFurnaceWoodLog(item.type) : item.type==needed.type))
				{ used[s]=true; found=true; break; }
			}
			if (!found) { return false; }
		}
		return true;
	}
}

bool isElectricConsumer(std::uint16_t type)
{ return type>=electricFurnace && type<=woodSawmill; }

std::uint32_t industryEnergyCapacity(std::uint16_t type)
{
	if (type==energyAccumulator) { return 5000; }
	if (type==fuelGenerator || isElectricConsumer(type)) { return 1000; }
	return 0;
}

IndustryProcess findIndustryProcess(const FurnaceBlock &m)
{
	if (m.blockType()==electricFurnace)
	{
		const auto &rs=getFurnaceRecipes();
		for (std::size_t i=0; i<rs.size(); ++i)
			if (inputsMatch(m,rs[i])) { return {rs[i],static_cast<std::uint16_t>(i)}; }
	}
	else for (std::size_t i=0; i<sizeof(definitions)/sizeof(definitions[0]); ++i)
		if (definitions[i].machine==m.blockType() && inputsMatch(m,definitions[i].recipe))
		{ return {definitions[i].recipe,static_cast<std::uint16_t>(100+i)}; }
	return {};
}

bool industryOutputFits(const FurnaceBlock &m, const IndustryProcess &p)
{
	if (p.id==FurnaceBlock::INVALID_RECIPE) { return false; }
	const auto &o=m.items[FURNACE_OUTPUT_SLOT];
	if (!o.type) { return true; }
	Item copy=o;
	return o.type==p.recipe.output.type && o.metaData.empty() &&
		static_cast<unsigned int>(o.counter)+p.recipe.output.count<=copy.getStackSize();
}

bool industrySlotAccepts(const FurnaceBlock &m, const Item &item, int slot)
{
	if (slot<0 || slot>=FURNACE_CAPACITY) { return false; }
	if (!item.type) { return true; }
	if (m.blockType()==itemExtractor) { return slot==0 && item.metaData.empty(); }
	if (!item.metaData.empty()) { return false; }
	if (m.blockType()==fuelGenerator) { return slot==FURNACE_FUEL_SLOT && isFurnaceFuel(item.type); }
	if (!isElectricConsumer(m.blockType()) || slot>=FURNACE_INPUT_CAPACITY) { return false; }
	if (m.blockType()==electricFurnace) { return isFurnaceInput(item.type); }
	for (const auto &d : definitions)
		if (d.machine==m.blockType()) for (const auto &n : d.recipe.inputs)
			if (n.type && (n.anyWoodLog ? isFurnaceWoodLog(item.type) : n.type==item.type)) { return true; }
	return false;
}

FurnaceTickResult tickIndustryMachine(FurnaceBlock &m, float dt)
{
	FurnaceTickResult result;
	if (!std::isfinite(dt) || dt<=0.f || !isIndustryBlock(m.blockType())) { return result; }
	dt=std::min(dt,1.f);
	const auto beforeEnergy=m.energyUnits;
	const auto beforeProgress=m.progressSeconds;
	if (m.blockType()==fuelGenerator)
	{
		const auto capacity=industryEnergyCapacity(m.blockType());
		if (m.energyUnits>=capacity) { return result; }
		if (m.fuelSecondsRemaining<=0.f)
		{
			Item &fuel=m.items[FURNACE_FUEL_SLOT];
			const float duration=getFurnaceFuelSeconds(fuel.type);
			if (!fuel.type || !fuel.counter || duration<=0.f || !fuel.metaData.empty()) { return result; }
			--fuel.counter; if (!fuel.counter) { fuel={}; }
			m.fuelSecondsRemaining=m.fuelSecondsTotal=duration;
			result.changed=result.needsNetworkSync=true;
		}
		const float step=std::min(dt,std::min(m.fuelSecondsRemaining,
			(static_cast<float>(capacity-m.energyUnits)-m.energyRemainder)/100.f));
		m.energyRemainder+=std::max(0.f,step)*100.f;
		const auto produced=static_cast<std::uint32_t>(m.energyRemainder);
		m.energyUnits+=produced; m.energyRemainder-=produced;
		m.fuelSecondsRemaining-=std::max(0.f,step);
		result.changed|=step>0.f;
	}
	else if (isElectricConsumer(m.blockType()))
	{
		const auto p=findIndustryProcess(m);
		if (m.activeRecipe!=p.id)
		{ m.activeRecipe=p.id; m.progressSeconds=0.f; result.changed=result.needsNetworkSync=true; }
		if (industryOutputFits(m,p))
		{
			constexpr float rate=20.f;
			const float step=std::min(dt,std::min(p.recipe.durationSeconds-m.progressSeconds,
				static_cast<float>(m.energyUnits)/rate));
			if (step>0.f)
			{
				const auto previousCost=static_cast<std::uint32_t>(std::ceil(m.progressSeconds*rate-0.0001f));
				const auto nextCost=static_cast<std::uint32_t>(std::ceil((m.progressSeconds+step)*rate-0.0001f));
				const auto cost=nextCost-previousCost;
				if (cost<=m.energyUnits)
				{ m.energyUnits-=cost; m.progressSeconds+=step; result.changed=true; }
			}
			if (m.progressSeconds+0.0001f>=p.recipe.durationSeconds)
			{
				bool used[3]={};
				for (auto n:p.recipe.inputs)
				{
					if (!n.type) { break; }
					for (int s=0;s<3;++s) if (!used[s] && m.items[s].counter>=n.count &&
						(n.anyWoodLog ? isFurnaceWoodLog(m.items[s].type) : m.items[s].type==n.type))
					{ used[s]=true; m.items[s].counter-=n.count; if (!m.items[s].counter) { m.items[s]={}; } break; }
				}
				auto &o=m.items[FURNACE_OUTPUT_SLOT];
				if (!o.type) { o=Item(p.recipe.output.type,p.recipe.output.count); }
				else { o.counter+=p.recipe.output.count; }
				m.progressSeconds=0.f; m.activeRecipe=FurnaceBlock::INVALID_RECIPE;
				result.changed=result.needsNetworkSync=true;
			}
		}
	}
	result.changed|=beforeEnergy!=m.energyUnits || beforeProgress!=m.progressSeconds;
	if (result.changed)
	{
		m.networkSyncAccumulator+=dt;
		if (m.networkSyncAccumulator>=0.25f)
		{ m.networkSyncAccumulator=0.f; result.needsNetworkSync=true; }
	}
	return result;
}

float industryProgressFraction(const FurnaceBlock &m)
{
	const auto p=findIndustryProcess(m);
	return p.id!=FurnaceBlock::INVALID_RECIPE && p.recipe.durationSeconds>0.f ?
		std::clamp(m.progressSeconds/p.recipe.durationSeconds,0.f,1.f) : 0.f;
}

const char *industryTitle(std::uint16_t type)
{
	switch(type)
	{
	case fuelGenerator: return "Fuel Generator - fuel -> Mie Energy (ME)";
	case energyAccumulator: return "Accumulator - 5,000 ME storage";
	case electricFurnace: return "Electric Furnace - 20 ME/s";
	case oreCrusher: return "Ore Crusher - ore -> powder, 20 ME/s";
	case metalPress: return "Metal Press - ingot -> 2 plates, 20 ME/s";
	case woodSawmill: return "Sawmill - log -> 6 planks, 20 ME/s";
	case itemExtractor: return "Extractor - input cell 1 sets an optional filter";
	default: return "Furnace - timed server processing";
	}
}
