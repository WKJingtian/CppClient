#include "pch.h"
#include "NetPackHandler.h"
#include "Game/HoldemPokerGame.h"
#include "Player/PlayerInfo.h"

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
		std::cout << speakerInfo.GetName() << ": " << msg << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_log_in)
	{
		// Format: id:u32, nickname:string, language:u32
		std::cout << "log in success" << std::endl;
		auto playerInfo = PlayerInfo(task);
		playerInfo.Print();
	}
	else if (task.MsgType() == RpcEnum::rpc_client_print_room)
	{
		// Format: count:u32, [roomId:i32, roomType:u16, userCnt:u32, [name:string, lang:u8]...]...
		uint32_t roomCnt = task.ReadUInt32();
		for (uint32_t i = 0; i < roomCnt; i++)
		{
			int roomIdx = task.ReadInt32();
			uint16_t roomType = task.ReadUInt16();
			uint32_t userCnt = task.ReadUInt32();
			std::cout << "Room " << roomIdx << " (Type: " << roomType << "): " << std::endl;
			for (uint32_t ii = 0; ii < userCnt; ii++)
			{
				auto memberInfo = PlayerInfo(task);
				memberInfo.Print();
			}
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_print_user)
	{
		// Format: count:u32, [name:string, lang:u8]...
		uint32_t userCnt = task.ReadUInt32();
		std::cout << "userCnt: " << userCnt << std::endl;
		for (uint32_t i = 0; i < userCnt; i++)
		{
			std::cout << "\t" << task.ReadString() << std::endl;
			task.ReadUInt8(); // language
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_goto_room)
	{
		// Format: roomId:i32
		std::cout << "Joined room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_leave_room)
	{
		// Format: roomId:i32
		std::cout << "Left room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_get_my_rooms)
	{
		// Format: count:u32, [roomId:i32, roomType:u16]...
		uint32_t roomCnt = task.ReadUInt32();
		std::cout << "You are in " << roomCnt << " room(s):" << std::endl;
		for (uint32_t i = 0; i < roomCnt; i++)
		{
			int roomId = task.ReadInt32();
			uint16_t roomType = task.ReadUInt16();
			std::cout << "\tRoom " << roomId << " (Type: " << roomType << ")" << std::endl;
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_create_room)
	{
		// Format: roomId:i32
		std::cout << "Created room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_error_respond)
	{
		// Format: errCode:u16
		auto errCode = task.ReadUInt16();
		std::cout << "Server sent error code: " << errCode << std::endl;
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

		auto stage = localGame.GetStage();
		int pot = localGame.GetTotalPot();
		int actingPlayer = localGame.ActingPlayerId();

		const auto& community = localGame.GetCommunity();
		const auto& seats = localGame.GetSeats();

		std::cout << "Room " << roomId << " - Stage: " << (int)stage
			<< ", Pot: " << pot
			<< ", Acting: " << actingPlayer << std::endl;

		std::cout << "  Community: ";
		for (const auto& c : community)
			if (c.IsValid()) std::cout << c.ToString() << " ";
		std::cout << std::endl;

		for (const Seat& seat : seats)
		{
			if (seat.playerId < 0) continue;
			std::cout << "  Seat " << seat.seatIndex
				<< ": Player " << seat.playerId
				<< ", Chips: " << seat.chips
				<< (seat.inHand ? " [IN HAND]" : "")
				<< (seat.folded ? " [FOLDED]" : "")
				<< (seat.sittingOut ? " [SITTING OUT]" : "")
				<< std::endl;

			if (seat.hole[0].IsValid())
			{
				std::cout << "    Hole: " << seat.hole[0].ToString()
					<< " " << seat.hole[1].ToString() << std::endl;
			}
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		// Format: seatIdx:i32, chips:i32, minBuyin:i32, bigBlind:i32, walletBalance:i32
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();
		int walletBalance = task.ReadInt32();

		std::cout << "Sat down at seat " << actualSeatIdx << std::endl;
		std::cout << "  Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			std::cout << "  Warning: Blinds not configured yet!" << std::endl;
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
			std::cout << "Buy-in successful! Table chips: " << tableChips
				<< ", Wallet: " << walletChips << std::endl;
			break;
		case HoldemPokerGame::BuyInResult::BelowMinimum:
			std::cout << "Buy-in failed: Below minimum amount" << std::endl;
			break;
		case HoldemPokerGame::BuyInResult::PlayerNotFound:
			std::cout << "Buy-in failed: Player not found at table" << std::endl;
			break;
		case HoldemPokerGame::BuyInResult::AlreadyInHand:
			std::cout << "Buy-in failed: Already in hand" << std::endl;
			break;
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_poker_standup)
	{
		// Format: success:u8
		bool success = task.ReadUInt8() != 0;
		std::cout << (success ? "Stood up from table" : "Failed to stand up") << std::endl;
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
			std::cout << "Blinds set: " << smallBlind << "/" << bigBlind
				<< ", Min buy-in: " << minBuyin << std::endl;
			break;
		case HoldemPokerGame::SetBlindsResult::GameInProgress:
			std::cout << "Cannot change blinds: Game in progress" << std::endl;
			break;
		case HoldemPokerGame::SetBlindsResult::InvalidValue:
			std::cout << "Invalid blind values" << std::endl;
			break;
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_poker_hand_result)
	{
		// Format: roomId:i32, then HandResult::Read format
		int roomId = task.ReadInt32();
		HandResult result;
		result.Read(task);

		std::cout << "=== Hand Result (Room " << roomId << ") ===" << std::endl;
		std::cout << "Total pot: " << result.totalPot << std::endl;
		std::cout << "Community: ";
		for (const auto& c : result.communityCards)
			if (c.IsValid()) std::cout << c.ToString() << " ";
		std::cout << std::endl;

		for (const auto& pr : result.playerResults)
		{
			std::cout << "  Player " << pr.playerId << ": ";
			if (pr.folded)
				std::cout << "FOLDED";
			else
			{
				std::cout << pr.holeCards[0].ToString() << " " << pr.holeCards[1].ToString()
					<< " (Rank: " << pr.handRank << ")";
			}
			if (pr.chipsWon > 0)
				std::cout << " WON " << pr.chipsWon;
			std::cout << std::endl;
		}
	}

	return 0;
}