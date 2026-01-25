#include "pch.h"
#include "NetPackHandler.h"
#include "Game/HoldemPokerGame.h"

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
		auto name = task.ReadString();
		Language lang = (Language)task.ReadUInt8();
		auto msg = task.ReadString();
		bool includeName = task.ReadInt8() == 1;
		if (includeName)
		{
			std::string saidString = "said";
			switch (lang)
			{
			case Chinese:
				saidString = "˵";
				break;
			default: break;
			}
			AudioCenter::Inst().AddVoiceMsg(std::format("{} {} {}", name.c_str(), saidString, msg.c_str()), lang);
		}
		else
			AudioCenter::Inst().AddVoiceMsg(msg.c_str(), lang);
		std::cout << name << ": " << msg << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_log_in)
	{
		std::cout << "log in success" << std::endl;
		std::cout << "\t _id: " << task.ReadUInt32() << std::endl;
		std::cout << "\t nickname: " << task.ReadString() << std::endl;
		std::cout << "\t language: " << task.ReadUInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_print_room)
	{
		uint32_t roomCnt = task.ReadUInt32();
		for (uint32_t i = 0; i < roomCnt; i++)
		{
			int roomIdx = task.ReadInt32();
			uint32_t userCnt = task.ReadUInt32();
			std::cout << "Room " << roomIdx << ": " << std::endl;
			for (uint32_t ii = 0; ii < userCnt; ii++)
			{
				std::cout << "\t" << task.ReadString() << std::endl;
				task.ReadUInt8(); // language, useless here
			}
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_print_user)
	{
		uint32_t userCnt = task.ReadUInt32();
		std::cout << "userCnt: " << userCnt << std::endl;
		for (uint32_t i = 0; i < userCnt; i++)
		{
			std::cout << "\t" << task.ReadString() << std::endl;
			task.ReadUInt8(); // language, useless here
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_goto_room)
	{
		std::cout << "Now in room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_create_room)
	{
		std::cout << "Created room " << task.ReadInt32() << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_error_respond)
	{
		auto errCode = task.ReadUInt16();
		std::cout << "Server sent error code: " << errCode << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_tick)
	{
		// do nothing
	}
	else if (task.MsgType() == RpcEnum::rpc_client_get_poker_table_info)
	{
		task.ReadInt32();
		task.ReadUInt8();
		task.ReadInt32();
	}
	else if (task.MsgType() == RpcEnum::rpc_client_get_poker_table_info)
	{
		int roomId = task.ReadInt32();

		// Use shared HoldemPokerGame class to parse entire table state
		HoldemPokerGame localGame;
		localGame.ReadTable(task);

		// Now you can access all data through the game object
		auto stage = localGame.GetStage();
		int pot = localGame.GetTotalPot();
		int actingPlayer = localGame.ActingPlayerId();
		int smallBlind = localGame.GetSmallBlind();
		int bigBlind = localGame.GetBigBlind();

		const auto& community = localGame.GetCommunity();
		const auto& seats = localGame.GetSeats();
		const auto& sidePots = localGame.GetSidePots();

		// Example: print table state
		std::cout << "Room " << roomId << " - Stage: " << (int)stage
			<< ", Pot: " << pot << std::endl;

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

			// Hole cards are available in seat.hole[0] and seat.hole[1]
			// if server sent them (self or showdown)
			if (seat.hole[0].IsValid())
			{
				std::cout << "    Hole: " << seat.hole[0].ToString()
					<< " " << seat.hole[1].ToString() << std::endl;
			}
		}
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();   // Always 0, need to buy in
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();       // -1 if not set
		int walletBalance = task.ReadInt32();  // Account balance

		std::cout << "Sat down at seat " << actualSeatIdx << std::endl;
		std::cout << "Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			std::cout << "Warning: Blinds not configured yet!" << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();   // Always 0, need to buy in
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();       // -1 if not set
		int walletBalance = task.ReadInt32();  // Account balance

		std::cout << "Sat down at seat " << actualSeatIdx << std::endl;
		std::cout << "Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			std::cout << "Warning: Blinds not configured yet!" << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();   // Always 0, need to buy in
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();       // -1 if not set
		int walletBalance = task.ReadInt32();  // Account balance

		std::cout << "Sat down at seat " << actualSeatIdx << std::endl;
		std::cout << "Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			std::cout << "Warning: Blinds not configured yet!" << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();   // Always 0, need to buy in
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();       // -1 if not set
		int walletBalance = task.ReadInt32();  // Account balance

		std::cout << "Sat down at seat " << actualSeatIdx << std::endl;
		std::cout << "Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			std::cout << "Warning: Blinds not configured yet!" << std::endl;
	}
	else if (task.MsgType() == RpcEnum::rpc_client_sit_down)
	{
		int actualSeatIdx = task.ReadInt32();
		int initialChips = task.ReadInt32();   // Always 0, need to buy in
		int minBuyin = task.ReadInt32();
		int bigBlind = task.ReadInt32();       // -1 if not set
		int walletBalance = task.ReadInt32();  // Account balance

		std::cout << "Sat down at seat " << actualSeatIdx << std::endl;
		std::cout << "Min buy-in: " << minBuyin
			<< ", Your wallet: " << walletBalance << std::endl;

		if (bigBlind < 0)
			std::cout << "Warning: Blinds not configured yet!" << std::endl;
	}

	return 0;
}