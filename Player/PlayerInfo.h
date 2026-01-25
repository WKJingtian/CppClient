#pragma once
#include <atomic>
#include "Utils/enum.h"

class NetPack;
class Player;
class PlayerMgr;

// Thread-safe player information container
// Uses atomic for chip count (frequently modified) and ReadWriteLock for other fields
class PlayerInfo
{
	int m_id;
	std::string m_name;
	Language m_language;
	std::atomic<int> m_chipCount;

public:
	PlayerInfo();
	PlayerInfo(const PlayerInfo& other);
	PlayerInfo& operator=(const PlayerInfo& other);
	PlayerInfo(NetPack& src);
	~PlayerInfo();

	void SetName(std::string n);
	void SetLanguage(Language l);
	
	// Memory-only chip operations (atomic, no database write)
	void AddChipsMemoryOnly(int delta);
	void SetChipsMemoryOnly(int newCount);
	
	// Chip operations with database sync
	void AddChips(int addCount);
	void SetChips(int newCount);

	int GetID() const;
	int GetChip() const;
	std::string GetName() const;
	Language GetLanguage() const;

	void WriteInfo(NetPack& dst);
	void ReadInfo(NetPack& src);

	void Print();

	friend Player;
	friend PlayerMgr;
};
