#include <multyPlayer/packet.h>

#include <zstd-1.5.5/lib/zstd.h>

#include <limits>
#include <new>

namespace
{
	constexpr unsigned long long maxDecompressedPacketSize = 32ULL * 1024ULL * 1024ULL;
}

namespace
{
	void *compressDataAtLevel(const char *data, size_t size, size_t &compressedSize, int level)
	{
		compressedSize = 0;
		if (!data || size == 0) { return nullptr; }

		const size_t maximumCompressedSize = ZSTD_compressBound(size);
		char *result = new (std::nothrow) char[maximumCompressedSize];
		if (!result) { return nullptr; }

		compressedSize = ZSTD_compress(result, maximumCompressedSize, data, size, level);
		if (ZSTD_isError(compressedSize))
		{
			delete[] result;
			compressedSize = 0;
			return nullptr;
		}
		return result;
	}
}

void *compressData(const char *data, size_t size, size_t &compressedSize)
{
	return compressDataAtLevel(data, size, compressedSize, 1);
}

void *compressDataForce(const char *data, size_t size, size_t &compressedSize)
{
	return compressDataAtLevel(data, size, compressedSize, 3);
}

void *unCompressData(const char *data, size_t compressedSize, size_t &originalSize)
{
	return unCompressDataBounded(data, compressedSize, originalSize,
		static_cast<size_t>(maxDecompressedPacketSize));
}

void *unCompressDataBounded(const char *data, size_t compressedSize, size_t &originalSize,
	size_t maximumOriginalSize)
{
	originalSize = 0;
	if (!data || compressedSize == 0 || maximumOriginalSize == 0) { return nullptr; }

	const unsigned long long frameSize = ZSTD_getFrameContentSize(data, compressedSize);
	if (frameSize == ZSTD_CONTENTSIZE_ERROR || frameSize == ZSTD_CONTENTSIZE_UNKNOWN ||
		frameSize == 0 || frameSize > maxDecompressedPacketSize ||
		frameSize > static_cast<unsigned long long>(maximumOriginalSize) ||
		frameSize > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
	{
		return nullptr;
	}
	originalSize = static_cast<size_t>(frameSize);

	char *decompressedData = new (std::nothrow) char[originalSize];
	if (!decompressedData) { return nullptr; }

	const size_t result = ZSTD_decompress(decompressedData, originalSize, data, compressedSize);
	if (ZSTD_isError(result) || result != originalSize)
	{
		delete[] decompressedData;
		originalSize = 0;
		return nullptr;
	}
	return decompressedData;
}
