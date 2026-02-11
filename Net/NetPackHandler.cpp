#include "pch.h"
#include "NetPackHandler.h"
#include "Game/HoldemPokerGame.h"
#include "Player/PlayerInfo.h"
#include "Helper/GameElementPrinter.h"
#include "ServerClass/Room.h"
#include "Audio/AudioCenter.h"

std::queue<NetPack> NetPackHandler::_taskList = std::queue<NetPack>();
std::mutex NetPackHandler::_mutex{};
void NetPackHandler::AddTask(NetPack&& pack)
{
	std::unique_lock<std::mutex> lock(_mutex);
	_taskList.push(std::move(pack));
}
int NetPackHandler::DoOneTask()
{
	std::unique_lock<std::mutex> lock(_mutex);
	if (_taskList.size() == 0)
		return 1; // no task to do

	auto& task = _taskList.front();
	_taskList.pop();

	if (task.MsgType() == RpcEnum::rpc_client_send_text)
	{
		// Format: name:string, language:u8, msg:string, includeName:i8
		auto speakerInfo = PlayerInfo(task);
		auto msg = task.ReadString();
		bool includeName = task.ReadInt8() == 1;
		if (includeName)
		{
			std::string saidString = "said";
			switch (speakerInfo.GetLanguage())
			{
			case Chinese:
				saidString = "˵";
				break;
			default: break;
			}
			AudioCenter::Inst().AddVoiceMsg(std::format("{} {} {}", speakerInfo.GetName().c_str(), saidString, msg.c_str()), speakerInfo.GetLanguage());
		}
		else
			AudioCenter::Inst().AddVoiceMsg(msg.c_str(), speakerInfo.GetLanguage());
		Console::Out() << speakerInfo.GetName() << ": " << msg << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_log_in)
	{
		// Format: id:u32, nickname:string, language:u32
		Console::Out() << "log in success" << std::endl;
		auto playerInfo = PlayerInfo(task);
		GameElementPrinter::Print(playerInfo);
	}
	else if (task.MsgType() == RpcEnum::rpc_client_print_room)
	{
		// Format: count:u32, [roomId:i32, roomType:u16, userCnt:u32, [name:string, lang:u8]...]...
		uint32_t roomCnt = task.ReadUInt32();
		Console::Out() << "roomCnt: " << roomCnt << std::endl;
		for (uint32_t i = 0; i < roomCnt; i++)
		{
			auto room = Room(task);
			GameElementPrinter::Print(room);
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_print_user)
	{
		// Format: count:u32, [name:string, lang:u8]...
		uint32_t userCnt = task.ReadUInt32();
		Console::Out() << "userCnt: " << userCnt << std::endl;
		for (uint32_t i = 0; i < userCnt; i++)
		{
			auto info = PlayerInfo(task);
			GameElementPrinter::Print(info);
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_goto_room)
	{
		// Format: roomId:i32
		Console::Out() << "Joined room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_leave_room)
	{
		// Format: roomId:i32
		Console::Out() << "Left room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_get_my_rooms)
	{
		// Format: count:u32, [roomId:i32, roomType:u16]...
		uint32_t roomCnt = task.ReadUInt32();
		Console::Out() << "You are in " << roomCnt << " room(s):" << std::endl;
		for (uint32_t i = 0; i < roomCnt; i++)
		{
			int roomId = task.ReadInt32();
			uint16_t roomType = task.ReadUInt16();
			Console::Out() << "\tRoom " << roomId << " (Type: " << Room::GetRoomTypeName((Room::RoomType)roomType) << ")" << std::endl;
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_create_room)
	{
		// Format: roomId:i32
		Console::Out() << "Created room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_error_respond)
	{
		// Format: errCode:u16
		auto errCode = task.ReadUInt16();
		Console::Out() << "Server sent error code: " << errCode << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_tick)
	{
		// do nothing
	}
	else if (task.MsgType() == RpcEnum::rpc_client_get_poker_table_info)
	{
		// Format: roomId:i32, then HoldemPokerGame::ReadTable format
		int roomId = task.ReadInt32();

		HoldemPokerGame localGame;
		localGame.ReadTable(task);
		GameElementPrinter::Print(localGame, roomId);
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		// Format: seatIdx:i32, chips:i32, minBuyin:i32, bigBlind:i32, walletBalance:i32
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();
		int walletBalance = task.ReadInt32();

		Console::Out() << "Sat down at seat " << actualSeatIdx << std::endl;
		Console::Out() << "  Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			Console::Out() << "  Warning: Blinds not configured yet!" << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_poker_buyin)
	{
		// Format: result:u8, tableChips:i32, walletChips:i32
		auto result = static_cast<HoldemPokerGame::BuyInResult>(task.ReadUInt8());
		int tableChips = task.ReadInt32();
		int walletChips = task.ReadInt32();

		switch (result)
		{
		case HoldemPokerGame::BuyInResult::Success:
			Console::Out() << "Buy-in successful! Table chips: " << tableChips
				<< ", Wallet: " << walletChips << std::endl;
			break;
		case HoldemPokerGame::BuyInResult::BelowMinimum:
			Console::Out() << "Buy-in failed: Below minimum amount" << std::endl;
			break;
		case HoldemPokerGame::BuyInResult::PlayerNotFound:
			Console::Out() << "Buy-in failed: Player not found at table" << std::endl;
			break;
		case HoldemPokerGame::BuyInResult::AlreadyInHand:
			Console::Out() << "Buy-in failed: Already in hand" << std::endl;
			break;
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_poker_standup)
	{
		// Format: success:u8
		bool success = task.ReadUInt8() != 0;
		Console::Out() << (success ? "Stood up from table" : "Failed to stand up") << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_poker_set_blinds)
	{
		// Format: result:u8, smallBlind:i32, bigBlind:i32, minBuyin:i32
		auto result = static_cast<HoldemPokerGame::SetBlindsResult>(task.ReadUInt8());
		int smallBlind = task.ReadInt32();
		int bigBlind = task.ReadInt32();
		int minBuyin = task.ReadInt32();

		switch (result)
		{
		case HoldemPokerGame::SetBlindsResult::Success:
			Console::Out() << "Blinds set: " << smallBlind << "/" << bigBlind
				<< ", Min buy-in: " << minBuyin << std::endl;
			break;
		case HoldemPokerGame::SetBlindsResult::GameInProgress:
			Console::Out() << "Cannot change blinds: Game in progress" << std::endl;
			break;
		case HoldemPokerGame::SetBlindsResult::InvalidValue:
			Console::Out() << "Invalid blind values" << std::endl;
			break;
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_poker_hand_result)
	{
		// Format: roomId:i32, then HandResult::Read format
		int roomId = task.ReadInt32();
		HandResult result;
		result.Read(task);
		GameElementPrinter::Print(result, roomId);
	}
	else if (task.MsgType() == RpcEnum::rpc_client_ping)
	{
		const int64_t UpLatencyMs = task.ReadInt64();
		const int64_t serverSendMs = task.ReadInt64();
		const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		const int64_t DnLatencyMs = nowMs - serverSendMs;
		Console::Out() << "[PING RESULT]:" << std::endl;
		Console::Out() << '\t' << "UP LATENCY: " << UpLatencyMs << " Ms"  << std::endl;
		Console::Out() << '\t' << "DN LATENCY: " << DnLatencyMs << " Ms" << std::endl;
	}

	return 0;
}
