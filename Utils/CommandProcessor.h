#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "Game/HoldemPokerGame.h"

class Player;

class CommandProcessor
{
public:
	explicit CommandProcessor(Player& player);
	void Run();
	bool HandleLine(const std::string& input);

private:
	enum class Mode
	{
		Default,
		ForceText,
		ForceCommand
	};

	struct PrefixResult
	{
		Mode mode = Mode::Default;
		std::optional<std::string> lang;
		std::optional<int> room;
	};

	struct CommandSpec
	{
		size_t minTokens = 1;
		bool exactTokens = false;
		bool missingArgsIsError = false;
		std::function<void(const std::vector<std::string>&, int)> handler;
	};

	Player& m_player;
	int m_currentRoom = -1;
	std::unordered_map<std::string, CommandSpec> m_commands;

	void RegisterCommands();
	static std::vector<std::string> Split(const std::string& str);
	static size_t SkipSpace(const std::string& text, size_t pos);
	static std::string LTrimCopy(const std::string& text);

	bool ParsePrefixes(const std::string& input, PrefixResult& out, size_t& remainderPos, std::string& error);
	void SendTextMessage(const std::string& rawInput, const std::string& message, const PrefixResult& prefix, int room);

	bool RequireRoom(int room) const;
	bool TryParseInt(const std::string& s, int& out) const;

	static std::function<void(const std::vector<std::string>& strVec, int room)> MakePokerActionCall(
		CommandProcessor* processor,
		HoldemPokerGame::Action actionType,
		bool needAmount);
};
