#pragma once
#include <queue>
#include <thread>
#include <mutex>

enum Language : byte
{
	English = 0,
	Chinese = 1,
	Italian = 2,
	Japanese = 3,
	Franch = 4,
	Spanish = 5,
};

class AudioCenter
{
public:
	static AudioCenter& Inst();

	struct voiceMsg
	{
		std::string msg;
		Language lang;
	};

public:
	bool m_mute;
	std::queue<voiceMsg> m_voiceMsgQueue;

	AudioCenter();
	void AddVoiceMsg(std::string msg, Language lang);
private:
	std::thread m_audioPlayThread;
	std::mutex _mutex;

	void PlayVoiceMsg();
};