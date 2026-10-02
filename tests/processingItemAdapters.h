#pragma once
#include <gameplay/items.h>
#include <cstring>
// Actual wire layout: empty=uint16 type; populated=type/count/uint16 metadata length.
std::size_t Item::formatIntoData(std::vector<unsigned char> &data)
{
	const auto start=data.size();
	const std::uint16_t length=static_cast<std::uint16_t>(metaData.size());
	const auto size=type ? 6+length : 2; data.resize(start+size);
	std::memcpy(data.data()+start,&type,2);
	if (type)
	{ std::memcpy(data.data()+start+2,&counter,2); std::memcpy(data.data()+start+4,&length,2); if(length) { std::memcpy(data.data()+start+6,metaData.data(),length); } }
	return data.size()-start;
}
int Item::readFromData(void *input,size_t size)
{
	*this={}; if(!input || size<2) { return -1; } auto *data=static_cast<unsigned char *>(input);
	std::memcpy(&type,data,2); if(!type) { return 2; } if(size<6) { return -1; }
	std::uint16_t length=0; std::memcpy(&counter,data+2,2); std::memcpy(&length,data+4,2);
	if(length>size-6) { return -1; } metaData.assign(data+6,data+6+length); return 6+length;
}
void Item::sanitize()
{ if(!type || !counter || (type>=BlocksCount && (type<ItemsStartPoint || type>=lastItem))) { *this={}; } else if(counter>getStackSize()) { counter=getStackSize(); } }
unsigned short Item::getStackSize()
{ return type==fieldGuide || type==bedroll ? 1 : 999; }
