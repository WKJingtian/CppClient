#pragma once
#include <cstdint>
#include <string>

class NetPack;

class Card
{
public:
	static constexpr uint8_t RANK_MIN = 2;
	static constexpr uint8_t RANK_MAX = 14; // Ace high
	static constexpr uint8_t SUIT_COUNT = 4;

	Card() = default;
	Card(uint8_t rank, uint8_t suit) : _rank(rank), _suit(suit) {}

	uint8_t Rank() const { return _rank; }
	uint8_t Suit() const { return _suit; }
	bool IsValid() const { return _rank >= RANK_MIN && _rank <= RANK_MAX && _suit < SUIT_COUNT; }

	std::wstring ToString() const
	{
		static const wchar_t* ranks[] = { L"",L"",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"T",L"J",L"Q",L"K",L"A" };
		static const wchar_t* suits[] = { L"\x2660",L"\x2665",L"\x2666",L"\x2663" };
		if (!IsValid()) return L"??";
		return std::wstring(ranks[_rank]) + suits[_suit];
	}

	bool operator==(const Card& other) const { return _rank == other._rank && _suit == other._suit; }
	bool operator<(const Card& other) const
	{
		if (_rank != other._rank) return _rank < other._rank;
		return _suit < other._suit;
	}

	void Write(NetPack& pack) const;
	void Read(NetPack& pack);

private:
	uint8_t _rank = 0;
	uint8_t _suit = 0;
};
