#include "pch.h"
#include "Utils//CommandProcessor.h"
#include "Audio/AudioCenter.h"

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
	CommandProcessor processor(selfPlayer);
	auto inputThread = std::thread([&processor]() { processor.Run(); });
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
