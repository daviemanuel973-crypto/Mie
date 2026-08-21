#include "multyPlayer/packet.h"
#include <iostream>
#include <platformTools.h>
#include <cstdint>
#include <cstring>
#include <multyPlayer/createConnection.h>
#include <gameplay/fieldGuide.h>
#include <gameplay/fieldGuideProtocol.h>

void sendPacketAndCompress(ENetPeer *to, 
	Packet p, const char *data, size_t size, 
	bool reliable, int channel, 
	ENetPacketFreeCallback freeCallback, unsigned int packetId)
{
	size_t compressedSize = 0;
	char* compressedData = (char*)compressData(data, size, compressedSize);

	if (!compressedData)
	{
		sendPacket(to, p, data, size, reliable, channel,
			freeCallback, packetId);
	}
	else
	{
		if (compressedSize >= size)
		{
			sendPacket(to, p, data, size, reliable, channel,
				freeCallback, packetId);
		}
		else
		{
			
			//std::cout << "CHUNK SIZE BYTES: " << compressedSize << "\n";
			//std::cout << "CompressedSize: " << compressedSize << "\n";

			//std::cout << "compressed\n";
			p.setCompressed();
			sendPacket(to, p, compressedData, compressedSize, reliable, channel,
				freeCallback, packetId);
		}

		delete[] compressedData;
	
	}
}

void sendPacket(ENetPeer *to, Packet p,
	const char *data, size_t size, bool reliable, int channel,
	ENetPacketFreeCallback freeCallback, unsigned int packetId)
{

	permaAssertComment((data && size) || (!data && !size), "Assert failed in sendPacket");

	size_t flag = 0;

	//memcpy(data, &p, sizeof(Packet));

	if (reliable)
	{
		flag = ENET_PACKET_FLAG_RELIABLE;
	}
	else
	{
		flag = ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
	}

	ENetPacket *packet = enet_packet_create(nullptr, size + sizeof(Packet), flag);
	if (!packet) { return; }

	if (freeCallback)
	{
		packet->freeCallback = freeCallback;
		packet->userData = reinterpret_cast<void *>(static_cast<std::uintptr_t>(packetId));
	}

	memcpy(packet->data, &p, sizeof(Packet));
	if (size > 0)
	{
		memcpy(packet->data + sizeof(Packet), data, size);
	}

	enet_peer_send(to, channel, packet);
}

//ton't use in client code!!
void sendPacket(ENetPeer *to, uint32_t header, void *data, size_t size, bool reliable, int channel)
{
	Packet packet;
	packet.header = header;
	
	sendPacket(to, packet, (const char*)data, size, reliable, channel);
}

void sendPacket(ENetPeer *to, uint32_t header, std::uint64_t cid, void *data, size_t size, bool reliable, int channel)
{
	Packet packet;
	packet.header = header;
	packet.cid = cid;

	sendPacket(to, packet, (const char *)data, size, reliable, channel);
}

char *parsePacket(ENetEvent &event, Packet &p, size_t &dataSize)
{
	char *data = parsePacket(*event.packet, p, dataSize);

	// Field Guide progress is authoritative on the server. v0.9.4 uses the
	// dedicated header 52 with a raw 8-byte GuideProgress payload. Only accept this
	// client mirror update from the peer that this process connected to as its
	// server, so packets received by an integrated/dedicated server cannot
	// mutate client UI state.
	if (data && event.peer == getServer() && p.header == headerUpdateGuideProgress &&
		dataSize == sizeof(GuideProgress))
	{
		GuideProgress progress = {};
		std::memcpy(&progress, data, sizeof(progress));
		setClientGuideProgress(progress);
	}

	return data;
}

char *parsePacket(ENetPacket &packet, Packet &p, size_t &dataSize)
{
	size_t size = packet.dataLength;
	void *data = packet.data;
	dataSize = 0;
	p = {};

	if (!data || size < sizeof(Packet))
	{
		return nullptr;
	}

	memcpy(&p, data, sizeof(Packet));
	dataSize = size - sizeof(Packet);
	if (dataSize == 0)
	{
		return nullptr;
	}

	return static_cast<char *>(data) + sizeof(Packet);
}

float computeRestantTimer(std::uint64_t older, std::uint64_t newer)
{
	float rez = (((std::int64_t)newer - (std::int64_t)older)) / 1000.f;
	return rez;
}

void sendPlayerSkinPacket(ENetPeer *to, std::uint64_t cid, gl2d::Texture &t)
{
	if (!t.id) { return; }

	glm::ivec2 size = {};
	auto data = t.readTextureData(0, &size);

	if (size.x != PLAYER_SKIN_SIZE || size.y != PLAYER_SKIN_SIZE) { return; }

	sendPlayerSkinPacket(to, cid, data);
}

void sendPlayerSkinPacket(ENetPeer *to, std::uint64_t cid, std::vector<unsigned char> &data)
{
	Packet p;
	p.cid = cid;
	p.header = headerSendPlayerSkin;

	sendPacketAndCompress(to, p, (const char *)data.data(), data.size(), true,
		channelHandleConnections);
}
