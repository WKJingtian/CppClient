#include "pch.h"
#include "Player.h"

Player::Player(SOCKET&& socket) :
	m_socket(socket)
{
	m_recvThread = std::thread(&Player::RecvJob, this);
}
void Player::RecvJob()
{
	char recvbuf[NET_PACK_MAX_LEN];
	int recvbuflen = NET_PACK_MAX_LEN;
	int iResult;
	// Receive until the peer shuts down the connection
	do {
		iResult = recv(m_socket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			NetPack pack = NetPack((uint8_t*)recvbuf);
			OnRecv(std::move(pack));
		}
		else
			break;
	} while (iResult > 0 && !m_deleted);
	m_recvThread.detach();
}
void Player::OnRecv(NetPack&& pack)
{
	NetPackHandler::AddTask(std::move(pack));
}
void Player::Send(NetPack& pack)
{
	if (Expired()) return;
	auto iSendResult = send(m_socket, pack.GetContent(), (int)pack.Length(), 0);
	if (iSendResult == SOCKET_ERROR)
		Delete(iSendResult * 100);
}
void Player::Send(RpcEnum msgType, std::function<void(NetPack&)> func)
{
	if (Expired()) return;
	NetPack pack(msgType);
	func(pack);
	Send(pack);
}
void Player::Delete(int errCode)
{
	if (m_deleted) return;
	m_deleted = true;
	std::cout << "delete player(err " << errCode << ")" << std::endl;
	if (m_recvThread.joinable()) m_recvThread.detach();
	shutdown(m_socket, SD_SEND);
	closesocket(m_socket);
}