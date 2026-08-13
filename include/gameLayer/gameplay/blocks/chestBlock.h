#pragma once
#include <vector>
#include <gameplay/items.h>


constexpr static int CHEST_CAPACITY = 27;

struct ChestBlock
{

	Item items[CHEST_CAPACITY] = {};
		
	size_t formatIntoData(std::vector<unsigned char> &appendTo);

	bool readFromBuffer(unsigned char *data, size_t s, size_t &outReadSize);

	// Read-only adapter used by hardened world loading. The legacy item parser
	// still accepts mutable bytes, so parse a bounded copy rather than allowing
	// it to write into the persisted world buffer.
	bool readFromBuffer(const unsigned char *data, size_t s, size_t &outReadSize)
	{
		if (!data && s != 0) { return false; }
		std::vector<unsigned char> payload;
		if (s != 0) { payload.assign(data, data + s); }
		return readFromBuffer(payload.data(), payload.size(), outReadSize);
	}

	bool isDataValid();

	void normalize();
};