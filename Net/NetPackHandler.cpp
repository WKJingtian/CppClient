#include "pch.h"
#include "NetPackHandler.h"

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

	return 0;
}