#include "multyPlayer/chunkSaver.h"
#include <glm/vec2.hpp>
#include <filesystem>
#include <iostream>
#include <multyPlayer/serverChunkStorer.h>
#include <cmath>
#include <safeSave.h>
#include <multyPlayer/packet.h>
#include <multyPlayer/enetServerFunction.h>
#include <gameplay/serverSiegeRuntime.h>
#include <sstream>
#include <cstdio>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

constexpr unsigned int CHUNK_PACK = 4;

constexpr int chunkDataSize = CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE * sizeof(Block);
constexpr int headDist = 1 + (CHUNK_PACK * CHUNK_PACK * sizeof(glm::ivec2));

//////////////////////Chunks//format/////////////////////////////////////////
//
//	(     char     ) (			glm::ivec2 8bytes		 ) ( unsigned char ) ( unsigned char )
//	/* 1 element  */ /* CHUNK_PACK*CHUNK_PACK elements	*/ /*  1 element  */ /*chunkDataSize*/
//	/*chunks count*/ /*			Chunk positions			*/ /* fully loaded*/ /* chunk data  */
//
/////////////////////////////////////////////////////////////////////////////



glm::ivec2 determineFilePos(glm::ivec2 chunkPos)
{
	glm::vec2 floatPos = chunkPos;
	floatPos /= CHUNK_PACK;
	const glm::ivec2 filePos = {floorf(floatPos.x), floorf(floatPos.y)};
	return filePos;
}


void saveEntityIntoOppenedFile(std::ofstream &f)
{

}


template<class T>
void saveOneEntityTypeIntoOpenFile(std::ofstream &f, T &entityContainer)
{
	for (auto &e : entityContainer)
	{
		if (isServerSiegeEnemy(e.first)) { continue; }
		e.second.appendDataToDisk(f, e.first);
	}
}

void saveAllEntitiesIntoOpenFile(std::ofstream &f, EntityData &entityData)
{

	//todo generalize !!!!
	saveOneEntityTypeIntoOpenFile(f, entityData.droppedItems);
	saveOneEntityTypeIntoOpenFile(f, entityData.zombies);
	saveOneEntityTypeIntoOpenFile(f, entityData.pigs);
	saveOneEntityTypeIntoOpenFile(f, entityData.cats);
	saveOneEntityTypeIntoOpenFile(f, entityData.goblins);
	saveOneEntityTypeIntoOpenFile(f, entityData.scareCrows);

}

namespace
{
	std::string sidecarBackupPath(const std::string &fileName)
	{
		return fileName + ".bak";
	}

	std::string sidecarTempPath(const std::string &fileName)
	{
		return fileName + ".tmp";
	}

	bool removeSidecarAndBackup(const std::string &fileName)
	{
		bool success = true;
		for (const std::string candidate : {fileName, sidecarBackupPath(fileName), sidecarTempPath(fileName)})
		{
			std::error_code error;
			std::filesystem::remove(candidate, error);
			if (error) { success = false; }
		}
		return success;
	}

	bool promoteSidecarTempFile(const std::string &tempFile, const std::string &fileName)
	{
		const std::string backupFile = sidecarBackupPath(fileName);
		std::error_code error;
		std::filesystem::remove(backupFile, error);
		error.clear();

		const bool hadPrimary = std::filesystem::exists(fileName, error) && !error;
		error.clear();
		if (hadPrimary)
		{
			std::filesystem::rename(fileName, backupFile, error);
			if (error)
			{
				std::filesystem::remove(tempFile, error);
				return false;
			}
		}

		std::filesystem::rename(tempFile, fileName, error);
		if (!error) { return true; }

		std::error_code cleanupError;
		std::filesystem::remove(tempFile, cleanupError);
		if (hadPrimary && !std::filesystem::exists(fileName, cleanupError))
		{
			cleanupError.clear();
			std::filesystem::rename(backupFile, fileName, cleanupError);
		}
		return false;
	}

	bool writeSidecarAtomically(const std::string &fileName,
		const unsigned char *data, std::size_t size)
	{
		const std::string tempFile = sidecarTempPath(fileName);
		std::FILE *file = std::fopen(tempFile.c_str(), "wb");
		if (!file) { return false; }

		bool success = !size || std::fwrite(data, 1, size, file) == size;
		if (success) { success = std::fflush(file) == 0; }
		if (success)
		{
#ifdef _WIN32
			success = ::_commit(::_fileno(file)) == 0;
#else
			success = ::fsync(::fileno(file)) == 0;
#endif
		}
		if (std::fclose(file) != 0) { success = false; }
		if (!success)
		{
			std::error_code error;
			std::filesystem::remove(tempFile, error);
			return false;
		}
		return promoteSidecarTempFile(tempFile, fileName);
	}

	enum class EntitySidecarLoadResult
	{
		missing,
		loaded,
		corrupt,
	};

	EntitySidecarLoadResult loadEntitySidecarCandidate(const std::string &fileName,
		EntityData &entityData)
	{
		std::ifstream f(fileName, std::ios::binary);
		if (!f.is_open()) { return EntitySidecarLoadResult::missing; }

		for (;;)
		{
			if (f.peek() == std::ifstream::traits_type::eof())
			{
				return EntitySidecarLoadResult::loaded;
			}

			Marker marker = 0;
			if (!readMarker(f, marker) || marker == 0)
			{
				return EntitySidecarLoadResult::corrupt;
			}

			std::uint64_t eid = 0;
			if (!readEntityId(f, eid) || eid == 0)
			{
				return EntitySidecarLoadResult::corrupt;
			}

			bool success = false;
			switch (marker)
			{
			case Markers::droppedItem:
			{
				DroppedItemServer item;
				success = getEntityTypeFromEID(eid) == EntityType::droppedItems &&
					item.loadFromDisk(f) && entityData.droppedItems.insert({eid, item}).second;
			}
			break;
			case Markers::zombie:
			{
				ZombieServer zombie;
				if (getEntityTypeFromEID(eid) == EntityType::zombies && zombie.loadFromDisk(f))
				{
					success = entityData.zombies.insert({eid, std::move(zombie)}).second;
				}
			}
			break;
			case Markers::pig:
			{
				PigServer pig;
				success = getEntityTypeFromEID(eid) == EntityType::pigs && pig.loadFromDisk(f) &&
					entityData.pigs.insert({eid, pig}).second;
			}
			break;
			case Markers::cat:
			{
				CatServer cat;
				success = getEntityTypeFromEID(eid) == EntityType::cats && cat.loadFromDisk(f) &&
					entityData.cats.insert({eid, cat}).second;
			}
			break;
			case Markers::goblin:
			{
				GoblinServer goblin;
				success = getEntityTypeFromEID(eid) == EntityType::goblins && goblin.loadFromDisk(f) &&
					entityData.goblins.insert({eid, goblin}).second;
			}
			break;
			case Markers::scareCrow:
			{
				ScareCrowServer scareCrow;
				success = getEntityTypeFromEID(eid) == EntityType::scareCrow && scareCrow.loadFromDisk(f) &&
					entityData.scareCrows.insert({eid, scareCrow}).second;
			}
			break;
			default:
				return EntitySidecarLoadResult::corrupt;
			}

			if (!success) { return EntitySidecarLoadResult::corrupt; }
			reserveEntityId(eid);
		}
	}
}

//todo if the loading chunk fails we should not load the entities there and rather delete those files if exist!!
bool WorldSaver::loadChunk(ChunkData &c)
{


	{
		const glm::ivec2 pos = {c.x, c.z};

		std::string fileName;
		fileName.reserve(256);
		fileName = savePath;
		fileName += "/c";
		fileName += std::to_string(pos.x);
		fileName += '_';
		fileName += std::to_string(pos.y);
		fileName += ".chz";

		// New saves use SafeSave's checksum and mirrored recovery file. Keep the
		// original .chz reader below so existing worlds remain compatible.
		std::vector<char> safeData;
		if (sfs::safeLoad(safeData, fileName.c_str(), false) == sfs::noError)
		{
			size_t newSize = 0;
			char *newData = static_cast<char *>(unCompressData(safeData.data(), safeData.size(), newSize));
			if (newData)
			{
				defer([&] { delete[] newData; });
				static_assert(sizeof(c.blocks) == chunkDataSize);
				if (newSize == chunkDataSize)
				{
					memcpy(c.blocks, newData, newSize);
					c.clearLightLevels();
					return 1;
				}
			}
			std::cout << "Server warning: safe chunk data was invalid, trying legacy recovery.\n";
		}

		std::vector<unsigned char> data;
		if (sfs::readEntireFile(data, fileName.c_str()) != sfs::noError) 
		{
			//return 0; 
		}
		else
		{
			size_t newSize = 0;
			char *newData = (char*)unCompressData((char*)data.data(), data.size(), newSize);

			if (newData)
			{
				defer([&] { delete[] newData; });

				static_assert(sizeof(c.blocks) == chunkDataSize);
				if (newSize != chunkDataSize)
				{
					std::cout << "Server error chunk file corupted size!\n";
					return 0;
				}

				memcpy(c.blocks, newData, newSize);
				c.clearLightLevels();
				return 1;
			}
			else
			{
				std::cout << "Server error chunk file corupted!\n";
				return 0;
			}

		}


	}


	//old implemenetation

	glm::ivec2 filePos = determineFilePos({c.x, c.z});

	std::string fileName;
	fileName.reserve(256);
	fileName = savePath;
	fileName += "/c";
	fileName += std::to_string(filePos.x);
	fileName += '_';
	fileName += std::to_string(filePos.y);
	fileName += ".chunks";

	//bool zippedVersion = 0;

	if (!std::filesystem::exists(fileName))
	{
		//try the zipped version first

		//fileName += 'z';

		return 0;
		//if (!std::filesystem::exists(fileName))
		//{
		//	return 0;
		//}
		//else
		//{
		//	zippedVersion = true;
		//}

	}

	//if (zippedVersion)
	//{
	//	std::vector<unsigned char> data;
	//	if (sfs::readEntireFile(data, fileName.c_str()) != sfs::noError) { return 0; };
	//
	//	size_t newDataSize = 0;
	//	char* newData = (char*)unCompressData((char*)data.data(), data.size(), newDataSize);
	//	defer([&] { std::cout << "Deffer works!!\n"; delete[] newData; }); //todo check that defer works!
	//
	//	if (!newData)
	//	{
	//		//todo report error here
	//		std::cout << "Server Error: corupted chunk!!!!\n";
	//		return 0;
	//	}
	//
	//	std::string_view view(newData, newDataSize);
	//	std::basic_stringbuf<char> buf(view.data(), std::ios::in);
	//	std::istream f(&buf);
	//
	//	f.seekg(0);
	//
	//	char count = 0;
	//	f.read(&count, 1);
	//
	//	if (count > CHUNK_PACK * CHUNK_PACK)
	//	{
	//		//todo error report.
	//		std::cout << "Corrupted file size bigger";
	//		return 0;
	//	}
	//
	//	glm::ivec2 positions[CHUNK_PACK * CHUNK_PACK];
	//
	//	f.read((char *)positions, count * sizeof(glm::ivec2));
	//
	//	//then we see if that chunk was already loaded
	//	int loadIndex = -1;
	//	for (int i = 0; i < count; i++)
	//	{
	//		if (positions[i] == glm::ivec2{c.x, c.z})
	//		{
	//			loadIndex = i;
	//			break;
	//		}
	//	}
	//
	//	if (loadIndex == -1)
	//	{
	//		//chunk file exists but there is no such chunk generate new chunk...
	//		return 0;
	//	}
	//
	//	loadChunkAtIndex(f, c, loadIndex);
	//	c.clearLightLevels();
	//
	//
	//	return 1;
	//}
	//else
	{
		//if it does exist, we first read it's content
		std::fstream f(fileName, std::ios::in | std::ios::out | std::ios::binary);

		f.seekg(0);

		char count = 0;
		f.read(&count, 1);

		if (count > CHUNK_PACK * CHUNK_PACK)
		{
			//todo error report.
			std::cout << "Corrupted file size bigger";
			return 0;
		}

		glm::ivec2 positions[CHUNK_PACK * CHUNK_PACK];

		f.read((char *)positions, count * sizeof(glm::ivec2));

		//then we see if that chunk was already loaded
		int loadIndex = -1;
		for (int i = 0; i < count; i++)
		{
			if (positions[i] == glm::ivec2{c.x, c.z})
			{
				loadIndex = i;
				break;
			}
		}

		if (loadIndex == -1)
		{
			//chunk file exists but there is no such chunk generate new chunk...
			f.close();
			return 0;
		}

		loadChunkAtIndex(f, c, loadIndex);
		c.clearLightLevels();


		f.close();
		return 1;
	}


}

bool WorldSaver::saveChunk(ChunkData &c)
{

	const glm::ivec2 pos = {c.x, c.z};

	std::string fileName;
	fileName.reserve(256);
	fileName = savePath;
	fileName += "/c";
	fileName += std::to_string(pos.x);
	fileName += '_';
	fileName += std::to_string(pos.y);
	fileName += ".chz";

	size_t newSize = 0;
	char *newData = (char*)compressDataForce((char*)&c.blocks, sizeof(c.blocks), newSize);
	static_assert(sizeof(c.blocks) == chunkDataSize);

	if (!newData)
	{
		std::cout << "Server error saving chunk, cant compress data!!!\n";
		return false;
	}
	defer([&] { delete[] newData; });

	const auto saveResult = sfs::safeSave(newData, newSize, fileName.c_str(), true);
	if (saveResult != sfs::noError)
	{
		std::cout << "Server error saving chunk safely: " << sfs::getErrorString(saveResult) << "\n";
		return false;
	}

	return true;

	/*
	{
		//old implementation
		const glm::ivec2 pos = {c.x, c.z};
		const glm::ivec2 filePos = determineFilePos(pos);

		std::string fileName;
		fileName.reserve(256);
		fileName = savePath;
		fileName += "/c";
		fileName += std::to_string(filePos.x);
		fileName += '_';
		fileName += std::to_string(filePos.y);
		fileName += ".chunks";


		if (!std::filesystem::exists(fileName))
		{
			//if the chunk doesn't exist, we create it
			std::fstream f;
			f.open(fileName, std::ios::out | std::ios::binary);

			unsigned char fillData = 0xFF;

			char count = 1;

			f.write(&count, 1);

			for (int i = 0; i < CHUNK_PACK * CHUNK_PACK * sizeof(glm::ivec2); i++)
			{
				f.write((char *)&fillData, sizeof(fillData));
			}

			f.seekp(1);
			//f.seekg(1);

			f.write((char *)&pos, sizeof(pos));

			appendChunkDataInFile(f, c);

			f.close();
		}
		else
		{
			//if it does exist, we first read it's content
			std::fstream f(fileName, std::ios::in | std::ios::out | std::ios::binary);

			f.seekg(0);

			char count = 0;
			f.read(&count, 1);

			if (count > CHUNK_PACK * CHUNK_PACK)
			{
				//todo error report.
				//todo probably try to recreate this chunk?
				std::cout << "corrupted file bigger while saving\n";
			}

			glm::ivec2 positions[CHUNK_PACK * CHUNK_PACK];

			f.read((char *)positions, count * sizeof(glm::ivec2));

			//then we see if that chunk was already loaded
			int loadIndex = -1;
			for (int i = 0; i < count; i++)
			{
				if (positions[i] == pos)
				{
					loadIndex = i;
					break;
				}
			}

			if (loadIndex != -1)
			{
				saveChunkDataInFile(f, c, loadIndex);
			}
			else
			{
				//we add the new chunk
				f.seekp(0, std::ios_base::beg);
				count++;
				f.write(&count, 1);

				f.seekp(1 + (count - 1) * sizeof(glm::ivec2), std::ios_base::beg);
				f.write((char *)&pos, sizeof(glm::ivec2));

				appendChunkDataInFile(f, c);
			}

			//const char* test = "12345678";
			//f.write(test, 8);
			//f.write((char*)&pos.x, sizeof(pos.x));
			f.close();
		}

	};
	*/
}

bool WorldSaver::saveChunkBlockData(SavedChunk &c)
{
	const glm::ivec2 pos = {c.chunk.x, c.chunk.z};
	std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".block";

	std::vector<unsigned char> data;
	c.blockData.formatBlockData(data, pos.x, pos.y);
	if (data.empty())
	{
		return removeSidecarAndBackup(fileName);
	}

	if (!writeSidecarAtomically(fileName, data.data(), data.size()))
	{
		std::cout << "Server error saving block sidecar transactionally: " << fileName << "\n";
		return false;
	}
	return true;
}

bool fileIsEmpty(std::ifstream &f)
{
	return f.peek() == std::ifstream::traits_type::eof();
}

void WorldSaver::loadEntityData(EntityData &entityData,
	glm::ivec2 chunkPosition)
{
	const glm::ivec2 pos = chunkPosition;
	const std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".entity";

	EntityData loaded;
	const auto primaryResult = loadEntitySidecarCandidate(fileName, loaded);
	if (primaryResult == EntitySidecarLoadResult::loaded)
	{
		entityData = std::move(loaded);
		return;
	}

	loaded = {};
	const auto backupResult = loadEntitySidecarCandidate(sidecarBackupPath(fileName), loaded);
	if (backupResult == EntitySidecarLoadResult::loaded)
	{
		entityData = std::move(loaded);
		std::cout << "Server warning: recovered entity sidecar from backup for chunk "
			<< pos.x << ',' << pos.y << ".\n";
		return;
	}

	if (primaryResult == EntitySidecarLoadResult::corrupt ||
		backupResult == EntitySidecarLoadResult::corrupt)
	{
		std::cout << "Server warning: entity sidecar corrupted for chunk "
			<< pos.x << ',' << pos.y << "; starting that chunk with no persisted entities.\n";
	}
	entityData = {};
}

void WorldSaver::loadBlockData(SavedChunk &c)
{
	const glm::ivec2 pos = {c.chunk.x, c.chunk.z};
	const std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".block";

	auto tryLoad = [&](const std::string &candidate) -> bool
	{
		std::vector<unsigned char> data;
		if (sfs::readEntireFile(data, candidate.c_str()) != sfs::noError || data.empty())
		{
			return false;
		}
		BlocksWithDataHolder loaded;
		if (!loaded.loadBlockData(data, pos.x, pos.y)) { return false; }
		c.blockData = std::move(loaded);
		return true;
	};

	if (tryLoad(fileName)) { return; }
	if (tryLoad(sidecarBackupPath(fileName)))
	{
		std::cout << "Server warning: recovered block-data sidecar from backup for chunk "
			<< pos.x << ',' << pos.y << ".\n";
		return;
	}
	c.blockData = {};
}

bool WorldSaver::saveEntitiesForChunk(SavedChunk &c)
{
	const glm::ivec2 pos = {c.chunk.x, c.chunk.z};
	const std::string fileName = savePath + "/c" + std::to_string(pos.x) + '_' +
		std::to_string(pos.y) + ".entity";
	const std::string tempFile = sidecarTempPath(fileName);

	std::ofstream f(tempFile, std::ios::binary | std::ios::trunc);
	if (!f.is_open()) { return false; }
	saveAllEntitiesIntoOpenFile(f, c.entityData);
	const std::streampos written = f.tellp();
	f.flush();
	const bool writeSucceeded = f.good();
	f.close();

	if (!writeSucceeded || written < std::streampos(0))
	{
		std::error_code error;
		std::filesystem::remove(tempFile, error);
		return false;
	}
	if (written == std::streampos(0))
	{
		std::error_code error;
		std::filesystem::remove(tempFile, error);
		return removeSidecarAndBackup(fileName);
	}
	if (!promoteSidecarTempFile(tempFile, fileName))
	{
		std::cout << "Server error saving entity sidecar transactionally: " << fileName << "\n";
		return false;
	}
	return true;
}

//todo
void WorldSaver::appendEntitiesForChunk(glm::ivec2 chunkPos)
{

	const glm::ivec2 filePos = (chunkPos);

	std::string fileName;
	fileName.reserve(256);
	fileName = savePath;
	fileName += "/c";
	fileName += std::to_string(filePos.x);
	fileName += '_';
	fileName += std::to_string(filePos.y);
	fileName += ".entity";

	std::ofstream f;
	f.open(fileName, std::ios::binary | std::ios::app);

	if (f.is_open())
	{


		f.close();
	}

}

void WorldSaver::saveChunkDataInFile(std::fstream &f, ChunkData &c, int index)
{
	f.seekp(headDist + (chunkDataSize * index), std::ios_base::beg);
	f.write((char *)c.blocks, chunkDataSize);
}

void WorldSaver::appendChunkDataInFile(std::fstream &f, ChunkData &c)
{
	f.seekp(0, std::ios_base::end);
	f.write((char *)c.blocks, chunkDataSize);
}

void WorldSaver::loadChunkAtIndex(std::fstream &f, ChunkData &c, int index)
{
	f.seekg(headDist + (chunkDataSize * index), std::ios_base::beg);
	f.read((char *)c.blocks, chunkDataSize);
}

void WorldSaver::loadChunkAtIndex(std::istream &f, ChunkData &c, int index)
{
	f.seekg(headDist + (chunkDataSize * index), std::ios_base::beg);
	f.read((char *)c.blocks, chunkDataSize);
}

void WorldSaver::saveEntityId(std::uint64_t eid)
{
	std::string fileName;
	fileName.reserve(256);
	fileName = savePath;
	fileName += "/eid.bin";
	std::ofstream f;
	f.open(fileName, std::ios::binary | std::ios::trunc);

	appendData(f, &eid, sizeof(eid));

	f.close();
}

//todo try to recover if fails
bool WorldSaver::loadEntityId(std::uint64_t &eid)
{

	std::string fileName;
	fileName.reserve(256);
	fileName = savePath;
	fileName += "/eid.bin";
	std::ifstream f;
	f.open(fileName, std::ios::binary);
	bool success = 0;

	if (f.is_open())
	{
		success = readData(f, &eid, sizeof(eid));
		f.close();
	}

	return success;
}
