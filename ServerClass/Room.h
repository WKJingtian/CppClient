#pragma once
#include "Net/RpcError.h"
#include "Player/Player.h"
#include <thread>
#include <mutex>
#include <vector>

class PlayerInfo;
class Room
{
public:
	enum RoomType : uint16_t
	{
		HALL = 0,
		CHAT_ROOM = 1,
		POKER_ROOM = 2,
	};

protected:
	int _roomId;
	RoomType _type;
	std::vector<PlayerInfo> _members{};

public:
	Room(NetPack& pack);
	void WriteRoom(NetPack& pack);
	int GetRoomId() const;
	std::string GetTypeName() const;
	const std::vector<PlayerInfo>& GetMembers() const;

	static std::string GetRoomTypeName(RoomType type);
};
