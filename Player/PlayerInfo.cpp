#include "pch.h"
#include "PlayerInfo.h"

PlayerInfo::PlayerInfo()
	: m_id(-1), m_name(""), m_language(Language::English), m_chipCount(0)
{
}

PlayerInfo::PlayerInfo(const PlayerInfo& other)
{
	m_id = other.m_id;
	m_name = other.m_name;
	m_language = other.m_language;
	m_chipCount.store(other.m_chipCount.load());
}

PlayerInfo& PlayerInfo::operator=(const PlayerInfo& other)
{
	if (this != &other)
	{
		m_id = other.m_id;
		m_name = other.m_name;
		m_language = other.m_language;
		m_chipCount.store(other.m_chipCount.load());
	}
	return *this;
}

PlayerInfo::PlayerInfo(NetPack& src)
	: m_chipCount(0)
{
	ReadInfo(src);
}

PlayerInfo::~PlayerInfo()
{
}

void PlayerInfo::SetName(std::string n)
{
	m_name = n;
}

void PlayerInfo::SetLanguage(Language l)
{
	m_language = l;
}

void PlayerInfo::AddChipsMemoryOnly(int delta)
{
	m_chipCount.fetch_add(delta);
}

void PlayerInfo::SetChipsMemoryOnly(int newCount)
{
	m_chipCount.store(newCount);
}

void PlayerInfo::AddChips(int addCount)
{
	m_chipCount.fetch_add(addCount);
#ifdef IS_CPP_SERVER
	WriteAssetToDatabase();
#endif
}

void PlayerInfo::SetChips(int newCount)
{
	m_chipCount.store(newCount);
#ifdef IS_CPP_SERVER
	WriteAssetToDatabase();
#endif
}

int PlayerInfo::GetID() const
{
	return m_id;
}

int PlayerInfo::GetChip() const
{
	return m_chipCount.load();
}

std::string PlayerInfo::GetName() const
{
	return m_name;
}

Language PlayerInfo::GetLanguage() const
{
	return m_language;
}

void PlayerInfo::WriteInfo(NetPack& dst)
{
	dst.WriteUInt32(m_id);
	dst.WriteString(m_name);
	dst.WriteUInt8(m_language);
	dst.WriteInt32(m_chipCount.load());
}

void PlayerInfo::ReadInfo(NetPack& src)
{
	m_id = src.ReadUInt32();
	m_name = src.ReadString();
	m_language = (Language)src.ReadUInt8();
	m_chipCount.store(src.ReadInt32());
}

void PlayerInfo::Print()
{
	Console::Out() << "player info with _id=" << GetID() << std::endl;
	Console::Out() << "\t nickname: " << GetName() << std::endl;
	Console::Out() << "\t language: " << (int)GetLanguage() << std::endl;
	Console::Out() << "\t chip: " << GetChip() << std::endl;
}
