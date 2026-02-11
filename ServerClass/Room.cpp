#include "pch.h"
#include "Net/NetPack.h"
#include "Room.h"
#include "Player/PlayerInfo.h"

Room::Room(NetPack& pack) :
	_type(static_cast<RoomType>(pack.ReadUInt16())),
	_roomId(pack.ReadInt32())
{
	//_roomId = (pack.ReadInt32());
	//_type = static_cast<RoomType>(pack.ReadUInt16());
	auto memberSize = pack.ReadUInt32();
	_members.reserve(memberSize);
	for (int i = 0; i < memberSize; i++)
		_members.emplace_back(PlayerInfo(pack));
}
void Room::WriteRoom(NetPack& pack)
{
	pack.WriteInt32(_roomId);
	pack.WriteUInt16(_type);  // Write room type
	pack.WriteUInt32((uint32_t)_members.size());
	for (auto p : _members) // have to copy here or code gets messy...
		p.WriteInfo(pack);
}
int Room::GetRoomId() const
{
	return _roomId;
}
std::string Room::GetTypeName() const
{
	return GetRoomTypeName(_type);
}
const std::vector<PlayerInfo>& Room::GetMembers() const
{
	return _members;
}

std::string Room::GetRoomTypeName(RoomType type)
{
	switch (type)
	{
	case HALL:
		return "LOBBY";
	case CHAT_ROOM:
		return "CHAT ROOM";
	case POKER_ROOM:
		return "HOLD'EM POKER ROOM";
	}
	return "INVALID";
}