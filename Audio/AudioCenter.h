#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include "Utils/enum.h"

class AudioCenter
{
public:
	static AudioCenter& Inst();

	struct voiceMsg
	{
		std::string msg;
		Language lang;
	};

	~AudioCenter();

public:
	bool m_mute;
	std::queue<voiceMsg> m_voiceMsgQueue;

	AudioCenter();
	void AddVoiceMsg(std::string msg, Language lang);
private:
	std::thread m_audioPlayThread;
	std::mutex _mutex;

	bool _stopped = false;

	void PlayVoiceMsg();
};