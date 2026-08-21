#include <metrics.h>
#include <cmath>
#include <limits>

int divideChunk(int x)
{
	const int quotient = x / CHUNK_SIZE;
	const int remainder = x % CHUNK_SIZE;
	return quotient - (remainder < 0 ? 1 : 0);
}

int divideChunk(double x)
{
	if (!std::isfinite(x)) { return 0; }
	// Blocks are centred on integer coordinates. Match from3DPointToBlock so a
	// point remains in block/chunk 0 until it crosses -0.5 or +15.5.
	const double chunk = std::floor((x + 0.5) / static_cast<double>(CHUNK_SIZE));
	if (chunk <= static_cast<double>(std::numeric_limits<int>::min()))
	{
		return std::numeric_limits<int>::min();
	}
	if (chunk >= static_cast<double>(std::numeric_limits<int>::max()))
	{
		return std::numeric_limits<int>::max();
	}
	return static_cast<int>(chunk);
}
glm::ivec2 fromBlockPosToChunkPos(glm::ivec3 blockPos)
{
	return glm::ivec2(divideChunk(blockPos.x), divideChunk(blockPos.z));
}
glm::ivec2 fromBlockPosToChunkPos(int x, int z)
{
	return glm::ivec2(divideChunk(x), divideChunk(z));
}
glm::ivec3 fromBlockPosToBlockPosInChunk(glm::ivec3 blockPos)
{
	const int modX = blockPos.x - divideChunk(blockPos.x) * CHUNK_SIZE;
	const int modZ = blockPos.z - divideChunk(blockPos.z) * CHUNK_SIZE;
	return {modX, blockPos.y, modZ};
}

int divideMetaChunk(int chunkPos)
{
	const int quotient = chunkPos / META_CHUNK_SIZE;
	const int remainder = chunkPos % META_CHUNK_SIZE;
	return quotient - (remainder < 0 ? 1 : 0);
}


//todo move into a new header file
glm::ivec2 determineChunkThatIsEntityIn(glm::dvec3 position)
{
	return {divideChunk(position.x), divideChunk(position.z)};
}
