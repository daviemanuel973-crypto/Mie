#include <gameplay/randomIndex.h>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <random>

int main()
{
	std::minstd_rand rng(0x4D4945u);
	std::size_t index = 99;

	assert(!trySelectRandomIndex(rng, 0, index));
	assert(index == 0);

	for (std::size_t count = 1; count <= 64; ++count)
	{
		for (int sample = 0; sample < 10000; ++sample)
		{
			assert(trySelectRandomIndex(rng, count, index));
			assert(index < count);
		}
	}

	std::cout << "random index bounds tests passed\n";
	return 0;
}
