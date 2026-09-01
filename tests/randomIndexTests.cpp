#include <gameplay/randomIndex.h>

#include <cstddef>
#include <iostream>
#include <random>

int main()
{
	std::minstd_rand rng(0x4D4945u);
	std::size_t index = 99;

	if (trySelectRandomIndex(rng, 0, index) || index != 0)
	{
		std::cerr << "empty selection was not rejected\n";
		return 1;
	}

	for (std::size_t count = 1; count <= 64; ++count)
	{
		for (int sample = 0; sample < 10000; ++sample)
		{
			if (!trySelectRandomIndex(rng, count, index) || index >= count)
			{
				std::cerr << "random index escaped bounds for count " << count << '\n';
				return 1;
			}
		}
	}

	std::cout << "random index bounds tests passed\n";
	return 0;
}
