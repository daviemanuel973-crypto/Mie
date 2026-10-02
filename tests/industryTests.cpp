#include <gameplay/industryNetwork.h>
#include "processingItemAdapters.h"
#include <iostream>
#include <cmath>
#include <limits>

namespace
{
	int failures=0;
	void check(bool result,const char *message)
	{ if(!result) { std::cerr<<"FAIL: "<<message<<'\n'; ++failures; } }
	struct World
	{
		IndustryNetwork network;
		std::map<IndustryPosition,FurnaceBlock> machines;
		std::map<IndustryPosition,ChestBlock> chests;
		std::uint64_t changes=0;
		void add(IndustryPosition p,std::uint16_t t)
		{ machines[p].processingType=t; network.setNode(p,t); }
		void addChest(IndustryPosition p) { chests[p]=ChestBlock{}; network.setNode(p,woddenChest); }
		IndustryAccess access()
		{
			return {[&](IndustryPosition p)->FurnaceBlock *{ auto it=machines.find(p); return it==machines.end()?nullptr:&it->second; },
				[&](IndustryPosition p)->ChestBlock *{ auto it=chests.find(p); return it==chests.end()?nullptr:&it->second; },
				[&](IndustryPosition p){ ++changes; network.wake(p); }};
		}
		void run(int ticks) { auto a=access(); for(int i=0;i<ticks;++i) { network.update(0.05f,a); } }
		unsigned total(std::uint16_t type)
		{ unsigned n=0; for(auto &c:chests) for(auto &s:c.second.items) if(s.type==type) n+=s.counter; for(auto &c:machines) for(auto &s:c.second.items) if(s.type==type) n+=s.counter; return n; }
	};
}

int main()
{
	FurnaceBlock generator; generator.processingType=fuelGenerator; generator.items[3]=Item(charcoal,1);
	for(int i=0;i<4;++i) { generator.tick(0.25f); }
	check(generator.energyUnits==100 && generator.items[3].type==0,"one second produces 100 ME and starts exactly one charcoal burn");
	generator.energyUnits=1000; const float fuel=generator.fuelSecondsRemaining; generator.tick(0.25f);
	check(generator.fuelSecondsRemaining==fuel,"a full generator stops consuming its remaining fuel");

	FurnaceBlock crusher; crusher.processingType=oreCrusher; crusher.items[0]=Item(copperOre,2); crusher.energyUnits=200;
	for(int i=0;i<20;++i) { crusher.tick(0.25f); }
	check(crusher.items[4].type==copperPowder && crusher.items[4].counter==1 && crusher.items[0].counter==1 && crusher.energyUnits==100,
		"crusher consumes one ore and exactly 100 ME in five seconds");
	FurnaceBlock blocked=crusher; blocked.items[4]=Item(ironIngot,999); const auto energy=blocked.energyUnits; blocked.tick(1.f);
	check(blocked.energyUnits==energy && blocked.progressSeconds==0.f,"blocked output consumes neither inputs nor power");
	FurnaceBlock empty; empty.processingType=metalPress; empty.items[0]=Item(ironIngot,1); empty.tick(1.f);
	check(empty.items[0].counter==1 && empty.progressSeconds==0.f,"power is required for processing");
	empty.energyUnits=100; for(int i=0;i<20;++i) { empty.tick(0.25f); }
	check(empty.items[4].type==ironPlate && empty.items[4].counter==2,"press turns one iron ingot into two plates");
	check(!empty.canPlaceInSlot(Item(charcoal),3) && !empty.canPlaceInSlot(Item(ironPlate),4),"electric processors reject fuel and output insertion");
	FurnaceBlock saw; saw.processingType=woodSawmill; saw.energyUnits=80; saw.items[0]=Item(birch_log,1);
	for(int i=0;i<16;++i) { saw.tick(0.25f); }
	check(saw.items[4].type==wooden_plank && saw.items[4].counter==6 && saw.energyUnits==0,"sawmill accepts alternate logs and conserves its 80-ME recipe cost");

	std::vector<unsigned char> payload; crusher.formatIntoData(payload); FurnaceBlock restored; size_t read=0;
	check(restored.readFromBuffer(payload.data(),payload.size(),read) && read==payload.size() && restored.blockType()==oreCrusher && restored.energyUnits==crusher.energyUnits,
		"industrial buffers, inventory and progress round-trip in their own format");
	for(size_t i=0;i<payload.size();++i) { check(!restored.readFromBuffer(payload.data(),i,read),"truncated state is rejected"); }
	auto bad=payload; const std::uint32_t overflow=1001; std::memcpy(bad.data()+8,&overflow,4);
	check(!restored.readFromBuffer(bad.data(),bad.size(),read),"over-capacity energy is rejected");
	FurnaceBlock legacy; legacy.items[0]=Item(copperOre,2); legacy.items[3]=Item(charcoal,1); legacy.tick(0.25f);
	payload.clear(); legacy.formatIntoData(payload);
	check(std::memcmp(payload.data(),"MIEF",4)==0 && restored.readFromBuffer(payload.data(),payload.size(),read) && restored.processingType==0,
		"legacy MIEF format 1 remains unchanged and readable");

	World w; w.add({0,64,0},fuelGenerator); w.add({1,64,0},powerCable); w.add({2,64,0},oreCrusher);
	w.machines[{0,64,0}].items[3]=Item(charcoal,2); w.machines[{2,64,0}].items[0]=Item(copperOre,2); w.run(400);
	check(w.total(copperPowder)==2,"generator, cable and crusher work as one authoritative production chain");
	const auto progress=w.machines[{2,64,0}].progressSeconds; w.network.removeNode({1,64,0}); w.machines.erase({1,64,0});
	w.machines[{2,64,0}].items[0]=Item(ironOre,1); w.machines[{2,64,0}].energyUnits=0; w.network.wake({2,64,0}); w.run(200);
	check(w.total(ironPowder)==0 && w.machines[{2,64,0}].progressSeconds==progress,"removing a cable invalidates topology and prevents disconnected production");
	w.add({1,64,0},powerCable); w.run(400); check(w.total(ironPowder)==0,"blocked old output remains intact after reconnecting");
	w.machines[{2,64,0}].items[4]={}; w.network.wake({2,64,0}); w.run(200); check(w.total(ironPowder)==1,"opening output wakes an idle consumer");

	World pipes; pipes.addChest({0,64,0}); pipes.add({1,64,0},itemExtractor); pipes.add({2,64,0},itemPipe); pipes.addChest({3,64,0});
	pipes.chests[{0,64,0}].items[0]=Item(copperOre,10); pipes.run(120);
	check(pipes.chests[{3,64,0}].items[0].type==copperOre && pipes.total(copperOre)==10,"pipes move stacks without loss or entity spawning");
	pipes.machines[{1,64,0}].items[0]=Item(ironOre,1); pipes.network.wake({1,64,0});
	const auto source=pipes.chests[{0,64,0}].items[0].counter; pipes.run(80);
	check(pipes.chests[{0,64,0}].items[0].counter==source && pipes.machines[{1,64,0}].items[0].counter==1,"filter is not consumed and rejects nonmatching loads");
	pipes.chests[{0,64,0}].items[1]=Item(ironOre,4); pipes.network.wake({0,64,0}); pipes.run(120);
	check(pipes.total(ironOre)==5 && pipes.chests[{3,64,0}].items[1].type==ironOre,"matching goods wake a sleeping pipe network and preserve totals including the filter");
	pipes.network.removeNode({2,64,0}); pipes.machines.erase({2,64,0}); pipes.chests[{0,64,0}].items[1]=Item(ironOre,4); pipes.run(120);
	check(pipes.chests[{0,64,0}].items[1].counter==4,"disconnected routes cannot reach the old destination");

	World factory;
	factory.add({2,220,2},fuelGenerator); factory.add({3,220,2},powerCable);
	factory.add({4,220,2},oreCrusher); factory.add({5,220,2},powerCable); factory.add({6,220,2},powerCable);
	factory.add({4,220,3},itemExtractor); factory.add({5,220,3},itemPipe); factory.add({6,220,3},electricFurnace);
	factory.add({6,220,4},itemExtractor); factory.addChest({7,220,4});
	factory.machines[{2,220,2}].items[3]=Item(charcoal,4);
	factory.machines[{4,220,2}].items[0]=Item(copperOre,3);
	factory.machines[{6,220,4}].items[0]=Item(copperIngot,1);
	factory.run(1200);
	check(factory.chests[{7,220,4}].items[0].type==copperIngot && factory.chests[{7,220,4}].items[0].counter==3,
		"complete smoke factory delivers three copper ingots to the chest");
	check(factory.total(copperIngot)==4 && factory.total(copperOre)==0 && factory.total(copperPowder)==0,
		"complete factory preserves three process units plus one untouched filter");
	World battery; battery.add({0,64,0},energyAccumulator); battery.add({1,64,0},powerCable); battery.add({2,64,0},oreCrusher);
	battery.machines[{0,64,0}].energyUnits=500; battery.machines[{2,64,0}].items[0]=Item(copperOre,1); battery.run(300);
	check(battery.total(copperPowder)==1 && battery.machines[{0,64,0}].energyUnits+battery.machines[{2,64,0}].energyUnits==400,
		"battery discharges without generating energy and spends exactly one recipe cost");

	World idle; for(int x=0;x<1000;++x) { idle.add({x*2,64,0},fuelGenerator); }
	for(int i=0;i<30;++i) { const auto before=idle.network.metrics().topologyVisits; idle.run(1); check(idle.network.metrics().topologyVisits-before<=256,"topology rebuild has a per-tick work budget"); }
	idle.run(4000); auto before=idle.network.metrics(); idle.run(1000); auto after=idle.network.metrics();
	check(before.machineUpdates==after.machineUpdates && before.topologyVisits==after.topologyVisits,"one thousand idle machines generate no simulation or topology work");
	idle.machines[{0,64,0}].items[3]=Item(charcoal,1); idle.network.wake({0,64,0}); idle.run(20);
	check(idle.machines[{0,64,0}].energyUnits>0,"inventory mutation wakes just the affected idle network");
	if(!failures) { std::cout<<"All industry energy, production, logistics, persistence and budget contracts passed\n"; }
	return failures?1:0;
}
