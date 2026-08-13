#include <gameplay/items.h>
#include <gameplay/zombie.h>
#include <gameplay/goblin.h>

#include <cassert>

int main()
{
	Item copperDagger(ItemTypes::copperKnife);
	const WeaponStats dagger = copperDagger.getWeaponStats();
	assert(dagger.damage >= 8.f);
	assert(dagger.range >= 1.f);

	Zombie zombie;
	Goblin goblin;
	assert(zombie.life.life == 40);
	assert(zombie.life.maxLife == 40);
	assert(goblin.life.life == 30);
	assert(goblin.life.maxLife == 30);

	return 0;
}
