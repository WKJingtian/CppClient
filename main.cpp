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
			else if (input == "HELP\0")
			{
				std::cout << "========== WKR's chatting room user guide ==========" << std::endl;
				std::cout << "SETNAME {yourname}: change your name " << std::endl;
				std::cout << "SETLANG {language}: change the language of your text. currently, there are only en (English) and cn (Chinese)" << std::endl;
				std::cout << "MUTE: to mute the chat room" << std::endl;
				std::cout << "UNMUTE: unmute the chat room" << std::endl;
				std::cout << "USERLIST: check all online users" << std::endl;
				std::cout << "MAKEROOM {roomId}: room id has to be a number" << std::endl;
				std::cout << "TOROOM {roomId}: switch to a different room, room id has to be a number" << std::endl;
				std::cout << "[{language}] {thingsToSay}: say something to other users in the same room with given language" << std::endl;
				std::cout << "    for example: \"[en] hello\" is to say hello in english" << std::endl;
				std::cout << "{thingsToSay}: say something with previously set language" << std::endl;
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
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	inputThread.join();
}