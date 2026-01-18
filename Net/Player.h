#pragma once
#include "RpcEnum.h"

class NetPack;
class Player
{
	SOCKET m_socket;
	std::thread m_recvThread;
	bool m_deleted = false;
	void RecvJob();
	void OnRecv(NetPack&& pack);
public:
	//static std::vector<std::shared_ptr<Player>> AllConnectedPlayers;
	//static void InitPlayer(SOCKET&& socket);
	Player() = delete;
	Player(SOCKET&& socket);
	~Player() { if (m_deleted) return; Delete(); }
	void Send(NetPack& pack);
	void Send(RpcEnum msgType, std::function<void(NetPack&)> func);
	void Delete(int errCode = 0);
	bool Expired() { return m_deleted || !m_recvThread.joinable(); }
};

