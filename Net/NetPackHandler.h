#pragma once
#include "Player/Player.h"

class NetPackHandler
{
	static std::queue<NetPack> _taskList;
	static std::mutex _mutex;
public:
	static void AddTask(NetPack&& pack);
	static int DoOneTask();
};

