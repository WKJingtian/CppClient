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
	std::cout << "cpp client project start" << std::endl;

	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
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
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET connectSocket = INVALID_SOCKET;
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;
	// Create a SOCKET for connecting to server
	connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (connectSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
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
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	AudioCenter::Inst();

	Player selfPlayer(std::move(connectSocket));
	auto inputFunc = [&]()
	{
		char inputBuffer[200];
		while (!selfPlayer.Expired())
		{
			bool waitingReturn = false;
			std::cin.getline(inputBuffer, 200);
			std::string input = std::string(inputBuffer);
			auto tokens = Split(input);
			if (input == "QUIT\0")
				selfPlayer.Delete();
			else if (input == "MUTE\0")
				AudioCenter::Inst().m_mute = true;
			else if (input == "UNMUTE\0")
				AudioCenter::Inst().m_mute = false;
			else if (tokens[0] == "LOGIN" && tokens.size() == 3)
			{
				auto idStr = tokens[1];
				auto pwdStr = tokens[2];
				auto id = 0;
				try { id = std::stoi(idStr); }
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
				}
				selfPlayer.Send(RpcEnum::rpc_server_log_in, [id, pwdStr](NetPack& pack)
					{
						pack.WriteUInt32(id);
						pack.WriteString(pwdStr);
					});
			}
			else if (tokens[0] == "REGISTER" && tokens.size() == 3)
			{
				auto nameStr = tokens[1];
				auto pwdStr = tokens[2];
				selfPlayer.Send(RpcEnum::rpc_server_register, [nameStr, pwdStr](NetPack& pack)
					{
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
				auto typeStr = tokens[1];
				uint16_t type = 0;
				try { type = (uint16_t)std::stoi(typeStr); }
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
				}
				selfPlayer.Send(RpcEnum::rpc_server_create_room, [type](NetPack& pack) { pack.WriteUInt16(type); });
			}
			else if (tokens[0] == "TOROOM" && tokens.size() == 2)
			{
				auto goTo = tokens[1];
				auto roomIdx = 0;
				try { roomIdx = std::stoi(goTo); }
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
				}
				selfPlayer.Send(RpcEnum::rpc_server_goto_room, [roomIdx](NetPack& pack) { pack.WriteInt32(roomIdx); });
			}
			else if (input == "ROOMLIST\0")
			{
				selfPlayer.Send(RpcEnum::rpc_server_print_room, [](NetPack& pack) {});
			}
			else if (input == "USERLIST\0")
			{
				selfPlayer.Send(RpcEnum::rpc_server_print_user, [](NetPack& pack) {});
			}
			// holdem poker section
			else if (tokens[0] == "TABLEINFO")
			{
				// Request current table state (no parameters needed)
				selfPlayer.Send(RpcEnum::rpc_server_get_poker_table_info, [](NetPack& pack) {});
			}
			else if (tokens[0] == "SIT" && tokens.size() >= 2)
			{
				int seatIdx = 0;
				try { seatIdx = std::stoi(tokens[1]); }
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
					return;
				}
				selfPlayer.Send(RpcEnum::rpc_server_sit_down, [seatIdx](NetPack& pack) {
					pack.WriteInt32(seatIdx);
					});
			}
			else if (tokens[0] == "SIT" && tokens.size() >= 2)
			{
				int seatIdx = 0;
				try { seatIdx = std::stoi(tokens[1]); }
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
					return;
				}
				selfPlayer.Send(RpcEnum::rpc_server_sit_down, [seatIdx](NetPack& pack) {
					pack.WriteInt32(seatIdx);
					});
			}
			else if (tokens[0] == "STANDUP")
			{
				// No parameters needed
				selfPlayer.Send(RpcEnum::rpc_server_poker_standup, [](NetPack& pack) {});
			}
			else if (tokens[0] == "STANDUP")
			{
				// No parameters needed
				selfPlayer.Send(RpcEnum::rpc_server_poker_standup, [](NetPack& pack) {});
			}
			else if (tokens[0] == "SETBLINDS" && tokens.size() >= 3)
			{
				int smallBlind = 0;
				int bigBlind = 0;
				try
				{
					smallBlind = std::stoi(tokens[1]);
					bigBlind = std::stoi(tokens[2]);
				}
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
					return;
				}
				selfPlayer.Send(RpcEnum::rpc_server_poker_set_blinds, [smallBlind, bigBlind](NetPack& pack) {
					pack.WriteInt32(smallBlind);
					pack.WriteInt32(bigBlind);
					});
			}
			// CHECK or CALL
			else if (tokens[0] == "CHECK" || tokens[0] == "CALL")
			{
				uint8_t action = 0;  // CheckCall
				selfPlayer.Send(RpcEnum::rpc_server_poker_action, [action](NetPack& pack) {
					pack.WriteUInt8(action);
					pack.WriteInt32(0);  // amount not used for check/call
					});
			}
			// BET or RAISE
			else if ((tokens[0] == "BET" || tokens[0] == "RAISE") && tokens.size() >= 2)
			{
				int amount = 0;
				try { amount = std::stoi(tokens[1]); }
				catch (std::exception const& e)
				{
					std::cout << e.what() << std::endl;
					return;
				}
				uint8_t action = 1;  // BetRaise
				selfPlayer.Send(RpcEnum::rpc_server_poker_action, [action, amount](NetPack& pack) {
					pack.WriteUInt8(action);
					pack.WriteInt32(amount);
					});
			}
			// FOLD
			else if (tokens[0] == "FOLD")
			{
				uint8_t action = 2;  // Fold
				selfPlayer.Send(RpcEnum::rpc_server_poker_action, [action](NetPack& pack) {
					pack.WriteUInt8(action);
					pack.WriteInt32(0);
					});
			}
			// help and error section
			else if (tokens[0] == "HELP")
			{
				if (tokens.size() == 2)
				{
					if (tokens[1] == "-HOLDEM")
						std::cout << HelpStrings::HOLDEM << std::endl;
					else if (tokens[1] == "-GENERIC")
						std::cout << HelpStrings::GENERIC << std::endl;
				}
				else
				{
					std::cout << "Available help topics:" << std::endl;
					std::cout << "		-GENERIC" << std::endl;
					std::cout << "		-HOLDEM" << std::endl;
				}
			}
			else if (tokens[0] == "LOGIN" || tokens[0] == "REGISTER" || tokens[0] == "SETNAME" ||
				tokens[0] == "SETLANG" || tokens[0] == "TOROOM")
			{
				std::cout << "ERROR INPUT: " << input << std::endl;
			}
			else
			{
				std::cout << "SENDING MSG: " << input << std::endl;
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
				selfPlayer.Send(RpcEnum::rpc_server_send_text, [msg](NetPack& pack) { pack.WriteString(msg); });
				waitingReturn = true;
			}

		}
	};

	auto inputThread = std::thread(inputFunc);
	while (!selfPlayer.Expired())
	{
		auto er = NetPackHandler::DoOneTask();
		while (er != 1)
		{
			if(er > 1) std::cout << "NetPackHandler::DoOneTask WARNING: " << er << std::endl;
			er = NetPackHandler::DoOneTask();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	inputThread.join();
}