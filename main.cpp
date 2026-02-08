#include "pch.h"

std::vector<std::string> Split(const std::string& str)
{
	std::stringstream ss(str);
	std::string token;
	std::vector<std::string> result;
	while (std::getline(ss, token, ' ')) {
		result.push_back(token);
	}
	return result;
}

int main(int* args)
{
	system("chcp 936");
	Console::Start();
	Console::Out() << "cpp client project start" << std::endl;

	AudioCenter::Inst();

	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		Console::Err() << "WSAStartup failed: " << iResult << std::endl;
		Console::Stop();
		return 1;
	}

	const std::string serverPort = "4242";
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	// when testing using 127.0.0.1
	iResult = getaddrinfo("127.0.0.1", serverPort.c_str(), &hints, &result);
	//iResult = getaddrinfo("43.128.29.250", serverPort.c_str(), &hints, &result);
	if (iResult != 0) {
		Console::Err() << "getaddrinfo failed: " << iResult << std::endl;
		WSACleanup();
		Console::Stop();
		return 1;
	}

	SOCKET connectSocket = INVALID_SOCKET;
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;
	// Create a SOCKET for connecting to server
	connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (connectSocket == INVALID_SOCKET) {
		Console::Err() << "Error at socket(): " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		Console::Stop();
		return 1;
	}
	// Connect to server.
	iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
	}
	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message
	freeaddrinfo(result);
	if (connectSocket == INVALID_SOCKET) {
		Console::Err() << "Unable to connect to server!" << std::endl;
		WSACleanup();
		Console::Stop();
		return 1;
	}

	Player selfPlayer(std::move(connectSocket));
	auto inputFunc = [&]()
	{
		int currentRoom = -1;  // Track current active room for multi-room support
		
		while (!selfPlayer.Expired())
		{
			std::string input;
			if (!Console::ReadLine(input))
				break;
			auto tokens = Split(input);
			
			if (tokens.empty()) continue;
			
			if (input == "QUIT\0")
				selfPlayer.Delete();
			else if (input == "MUTE\0")
				AudioCenter::Inst().m_mute = true;
			else if (input == "UNMUTE\0")
				AudioCenter::Inst().m_mute = false;
			else if (input == "PING\0")
			{
				const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
				selfPlayer.Send(RpcEnum::rpc_server_ping, [nowMs](NetPack& pack) { pack.WriteInt64(nowMs); });
			}
			else if (tokens[0] == "LOGIN" && tokens.size() == 3)
			{
				auto idStr = tokens[1];
				auto pwdStr = tokens[2];
				int id = 0;
				try { id = std::stoi(idStr); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_log_in, [id, pwdStr](NetPack& pack) {
					pack.WriteUInt32(id);
					pack.WriteString(pwdStr);
				});
			}
			else if (tokens[0] == "REGISTER" && tokens.size() == 3)
			{
				auto nameStr = tokens[1];
				auto pwdStr = tokens[2];
				selfPlayer.Send(RpcEnum::rpc_server_register, [nameStr, pwdStr](NetPack& pack) {
					pack.WriteString(nameStr);
					pack.WriteString(pwdStr);
				});
			}
			else if (tokens[0] == "SETNAME" && tokens.size() == 2)
			{
				auto setTo = tokens[1];
				selfPlayer.Send(RpcEnum::rpc_server_set_name, [setTo](NetPack& pack) { pack.WriteString(setTo); });
			}
			else if (tokens[0] == "SETLANG" && tokens.size() == 2)
			{
				auto setTo = tokens[1];
				selfPlayer.Send(RpcEnum::rpc_server_set_language, [setTo](NetPack& pack) { pack.WriteString(setTo); });
			}
			else if (tokens[0] == "MAKEROOM" && tokens.size() == 2)
			{
				uint16_t type = 0;
				try { type = (uint16_t)std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_create_room, [type](NetPack& pack) { pack.WriteUInt16(type); });
			}
			else if (tokens[0] == "TOROOM" && tokens.size() == 2)
			{
				int roomIdx = 0;
				try { roomIdx = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				currentRoom = roomIdx;
				selfPlayer.Send(RpcEnum::rpc_server_goto_room, [roomIdx](NetPack& pack) { pack.WriteInt32(roomIdx); });
			}
			else if (tokens[0] == "LEAVEROOM" && tokens.size() == 2)
			{
				int roomIdx = 0;
				try { roomIdx = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_leave_room, [roomIdx](NetPack& pack) { pack.WriteInt32(roomIdx); });
			}
			else if (input == "MYROOMS\0")
			{
				selfPlayer.Send(RpcEnum::rpc_server_get_my_rooms, [](NetPack& pack) {});
			}
			else if (tokens[0] == "USEROOM" && tokens.size() == 2)
			{
				try { currentRoom = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				Console::Out() << "Current room set to " << currentRoom << std::endl;
			}
			else if (input == "ROOMLIST\0")
			{
				selfPlayer.Send(RpcEnum::rpc_server_print_room, [](NetPack& pack) {});
			}
			else if (input == "USERLIST\0")
			{
				selfPlayer.Send(RpcEnum::rpc_server_print_user, [](NetPack& pack) {});
			}
			// ========== Poker commands (require currentRoom) ==========
			else if (tokens[0] == "TABLEINFO")
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_get_poker_table_info, [currentRoom](NetPack& pack) {
					pack.WriteInt32(currentRoom);
				});
			}
			else if (tokens[0] == "SIT" && tokens.size() >= 2)
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				int seatIdx = 0;
				try { seatIdx = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_sit_down, [currentRoom, seatIdx](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteInt32(seatIdx);
				});
			}
			else if (tokens[0] == "BUYIN" && tokens.size() >= 2)
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				int amount = 0;
				try { amount = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_buyin, [currentRoom, amount](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteInt32(amount);
				});
			}
			else if (tokens[0] == "STANDUP")
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_standup, [currentRoom](NetPack& pack) {
					pack.WriteInt32(currentRoom);
				});
			}
			else if (tokens[0] == "SETBLINDS" && tokens.size() >= 3)
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				int smallBlind = 0, bigBlind = 0;
				try { smallBlind = std::stoi(tokens[1]); bigBlind = std::stoi(tokens[2]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_set_blinds, [currentRoom, smallBlind, bigBlind](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteInt32(smallBlind);
					pack.WriteInt32(bigBlind);
				});
			}
			else if (tokens[0] == "CHECK" || tokens[0] == "CALL")
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_action, [currentRoom](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteUInt8(0);  // CheckCall
					pack.WriteInt32(0);
				});
			}
			else if ((tokens[0] == "BET" || tokens[0] == "RAISE") && tokens.size() >= 2)
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				int amount = 0;
				try { amount = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_action, [currentRoom, amount](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteUInt8(1);  // BetRaise
					pack.WriteInt32(amount);
				});
			}
			else if (tokens[0] == "ADDHOLDEMBOT")
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_add_bot, [currentRoom](NetPack& pack) { 
					pack.WriteInt32(currentRoom); 
				});
			}
			else if (tokens[0] == "RMHOLDEMBOT" && tokens.size() == 2)
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				int seatID = 0;
				try { seatID = std::stoi(tokens[1]); }
				catch (std::exception const& e) { Console::Out() << e.what() << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_kick_bot, [currentRoom, seatID](NetPack& pack) { 
					pack.WriteInt32(currentRoom); 
					pack.WriteInt32(seatID);
				});
			}
			else if (tokens[0] == "FOLD")
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl; continue; }
				selfPlayer.Send(RpcEnum::rpc_server_poker_action, [currentRoom](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteUInt8(2);  // Fold
					pack.WriteInt32(0);
				});
			}
			// ========== Help ==========
			else if (tokens[0] == "HELP")
			{
				if (tokens.size() == 2)
				{
					if (tokens[1] == "-HOLDEM")
						Console::Out() << HelpStrings::HOLDEM << std::endl;
					else if (tokens[1] == "-GENERIC")
						Console::Out() << HelpStrings::GENERIC << std::endl;
				}
				else
				{
					Console::Out() << "Available commands:" << std::endl;
					Console::Out() << "  HELP -GENERIC    - General commands" << std::endl;
					Console::Out() << "  HELP -HOLDEM     - Poker commands" << std::endl;
				}
			}
			// ========== Error handling ==========
			else if (tokens[0] == "LOGIN" || tokens[0] == "REGISTER" || tokens[0] == "SETNAME" ||
				tokens[0] == "SETLANG" || tokens[0] == "TOROOM" || tokens[0] == "LEAVEROOM" ||
				tokens[0] == "MAKEROOM" || tokens[0] == "USEROOM")
			{
				Console::Out() << "ERROR: Missing arguments for " << tokens[0] << std::endl;
			}
			// ========== Send text message ==========
			else
			{
				if (currentRoom < 0) { Console::Out() << "ERROR: Use USEROOM <id> first to send messages" << std::endl; continue; }
				Console::Out() << "SENDING MSG: " << input << std::endl;
				auto msg = input;
				if (input.size() >= 4)
				{
					auto initCmd = input.substr(0, 4);
					if (initCmd == "[en]")
					{
						selfPlayer.Send(RpcEnum::rpc_server_set_language, [](NetPack& pack) { pack.WriteString("en"); });
						msg = input.substr(4, input.length() - 4);
					}
					else if (initCmd == "[cn]")
					{
						selfPlayer.Send(RpcEnum::rpc_server_set_language, [](NetPack& pack) { pack.WriteString("cn"); });
						msg = input.substr(4, input.length() - 4);
					}
				}
				selfPlayer.Send(RpcEnum::rpc_server_send_text, [currentRoom, msg](NetPack& pack) {
					pack.WriteInt32(currentRoom);
					pack.WriteString(msg);
				});
			}
		}
	};

	auto inputThread = std::thread(inputFunc);
	while (!selfPlayer.Expired())
	{
		auto er = NetPackHandler::DoOneTask();
		while (er != 1)
		{
			if(er > 1) Console::Out() << "NetPackHandler::DoOneTask WARNING: " << er << std::endl;
			er = NetPackHandler::DoOneTask();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	Console::Stop();
	inputThread.join();
}
