#include "pch.h"
#include "Utils//CommandProcessor.h"

#include <chrono>
#include <cctype>
#include <sstream>

#include "Helper/HelpString.h"
#include "Audio/AudioCenter.h"

CommandProcessor::CommandProcessor(Player& player)
	: m_player(player)
{
	RegisterCommands();
}

void CommandProcessor::Run()
{
	while (!m_player.Expired())
	{
		std::string input;
		if (!Console::ReadLine(input))
			break;
		HandleLine(input);
	}
}

bool CommandProcessor::HandleLine(const std::string& input)
{
	if (input.empty())
		return true;

	PrefixResult prefix;
	size_t remainderPos = 0;
	std::string error;
	if (!ParsePrefixes(input, prefix, remainderPos, error))
	{
		Console::Out() << error << std::endl;
		return true;
	}

	std::string remainder = input.substr(remainderPos);
	int room = prefix.room.has_value() ? *prefix.room : m_currentRoom;

	if (prefix.mode == Mode::ForceText)
	{
		SendTextMessage(input, remainder, prefix, room);
		return true;
	}

	std::string commandText = LTrimCopy(remainder);
	std::vector<std::string> tokens;
	if (!commandText.empty())
		tokens = Split(commandText);

	if (!tokens.empty() && !tokens[0].empty())
	{
		auto it = m_commands.find(tokens[0]);
		if (it != m_commands.end())
		{
			const CommandSpec& spec = it->second;
			bool sizeOk = spec.exactTokens ? (tokens.size() == spec.minTokens) : (tokens.size() >= spec.minTokens);
			if (sizeOk)
			{
				spec.handler(tokens, room);
				return true;
			}

			if (prefix.mode == Mode::ForceCommand || spec.missingArgsIsError)
			{
				Console::Out() << "ERROR: Missing arguments for " << tokens[0] << std::endl;
				return true;
			}
		}
	}

	if (prefix.mode == Mode::ForceCommand)
	{
		if (tokens.empty() || tokens[0].empty())
			Console::Out() << "ERROR: Missing command" << std::endl;
		else
			Console::Out() << "ERROR: Unknown command: " << tokens[0] << std::endl;
		return true;
	}

	SendTextMessage(input, remainder, prefix, room);
	return true;
}

void CommandProcessor::RegisterCommands()
{
	m_commands["QUIT"] = CommandSpec{ 1, true, false, [this](const std::vector<std::string>&, int) {
		m_player.Delete();
	} };

	m_commands["MUTE"] = CommandSpec{ 1, true, false, [](const std::vector<std::string>&, int) {
		AudioCenter::Inst().m_mute = true;
	} };

	m_commands["UNMUTE"] = CommandSpec{ 1, true, false, [](const std::vector<std::string>&, int) {
		AudioCenter::Inst().m_mute = false;
	} };

	m_commands["PING"] = CommandSpec{ 1, true, false, [this](const std::vector<std::string>&, int) {
		const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		m_player.Send(RpcEnum::rpc_server_ping, [nowMs](NetPack& pack) { pack.WriteInt64(nowMs); });
	} };

	m_commands["LOGIN"] = CommandSpec{ 3, true, true, [this](const std::vector<std::string>& tokens, int) {
		int id = 0;
		try { id = std::stoi(tokens[1]); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		auto pwdStr = tokens[2];
		m_player.Send(RpcEnum::rpc_server_log_in, [id, pwdStr](NetPack& pack) {
			pack.WriteUInt32(id);
			pack.WriteString(pwdStr);
		});
	} };

	m_commands["REGISTER"] = CommandSpec{ 3, true, true, [this](const std::vector<std::string>& tokens, int) {
		auto nameStr = tokens[1];
		auto pwdStr = tokens[2];
		m_player.Send(RpcEnum::rpc_server_register, [nameStr, pwdStr](NetPack& pack) {
			pack.WriteString(nameStr);
			pack.WriteString(pwdStr);
		});
	} };

	m_commands["SETNAME"] = CommandSpec{ 2, true, true, [this](const std::vector<std::string>& tokens, int) {
		auto setTo = tokens[1];
		m_player.Send(RpcEnum::rpc_server_set_name, [setTo](NetPack& pack) { pack.WriteString(setTo); });
	} };

	m_commands["SETLANG"] = CommandSpec{ 2, true, true, [this](const std::vector<std::string>& tokens, int) {
		auto setTo = tokens[1];
		m_player.Send(RpcEnum::rpc_server_set_language, [setTo](NetPack& pack) { pack.WriteString(setTo); });
	} };

	m_commands["MAKEROOM"] = CommandSpec{ 2, true, true, [this](const std::vector<std::string>& tokens, int) {
		uint16_t type = 0;
		try { type = static_cast<uint16_t>(std::stoi(tokens[1])); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_player.Send(RpcEnum::rpc_server_create_room, [type](NetPack& pack) { pack.WriteUInt16(type); });
	} };

	m_commands["TOROOM"] = CommandSpec{ 2, true, true, [this](const std::vector<std::string>& tokens, int) {
		int roomIdx = 0;
		try { roomIdx = std::stoi(tokens[1]); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_currentRoom = roomIdx;
		m_player.Send(RpcEnum::rpc_server_goto_room, [roomIdx](NetPack& pack) { pack.WriteInt32(roomIdx); });
	} };

	m_commands["LEAVEROOM"] = CommandSpec{ 2, true, true, [this](const std::vector<std::string>& tokens, int) {
		int roomIdx = 0;
		try { roomIdx = std::stoi(tokens[1]); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_player.Send(RpcEnum::rpc_server_leave_room, [roomIdx](NetPack& pack) { pack.WriteInt32(roomIdx); });
	} };

	m_commands["MYROOMS"] = CommandSpec{ 1, true, false, [this](const std::vector<std::string>&, int) {
		m_player.Send(RpcEnum::rpc_server_get_my_rooms, [](NetPack& pack) {});
	} };

	m_commands["USEROOM"] = CommandSpec{ 2, true, true, [this](const std::vector<std::string>& tokens, int) {
		int roomIdx = 0;
		try { roomIdx = std::stoi(tokens[1]); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_currentRoom = roomIdx;
		Console::Out() << "Current room set to " << m_currentRoom << std::endl;
	} };

	m_commands["ROOMLIST"] = CommandSpec{ 1, true, false, [this](const std::vector<std::string>&, int) {
		m_player.Send(RpcEnum::rpc_server_print_room, [](NetPack& pack) {});
	} };

	m_commands["USERLIST"] = CommandSpec{ 1, true, false, [this](const std::vector<std::string>&, int) {
		m_player.Send(RpcEnum::rpc_server_print_user, [](NetPack& pack) {});
	} };

	m_commands["TABLEINFO"] = CommandSpec{ 1, false, false, [this](const std::vector<std::string>&, int room) {
		if (!RequireRoom(room))
			return;
		m_player.Send(RpcEnum::rpc_server_get_poker_table_info, [room](NetPack& pack) {
			pack.WriteInt32(room);
		});
	} };

	m_commands["SIT"] = CommandSpec{ 1, false, false, [this](const std::vector<std::string>& tokens, int room) {
		if (!RequireRoom(room))
			return;
		m_player.Send(RpcEnum::rpc_server_sit_down, [room](NetPack& pack) {
			pack.WriteInt32(room);
		});
	} };

	m_commands["BUYIN"] = CommandSpec{ 2, false, false, [this](const std::vector<std::string>& tokens, int room) {
		if (!RequireRoom(room))
			return;
		int amount = 0;
		try { amount = std::stoi(tokens[1]); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_player.Send(RpcEnum::rpc_server_poker_buyin, [room, amount](NetPack& pack) {
			pack.WriteInt32(room);
			pack.WriteInt32(amount);
		});
	} };

	m_commands["STANDUP"] = CommandSpec{ 1, false, false, [this](const std::vector<std::string>&, int room) {
		if (!RequireRoom(room))
			return;
		m_player.Send(RpcEnum::rpc_server_poker_standup, [room](NetPack& pack) {
			pack.WriteInt32(room);
		});
	} };

	m_commands["SETBLINDS"] = CommandSpec{ 3, false, false, [this](const std::vector<std::string>& tokens, int room) {
		if (!RequireRoom(room))
			return;
		int smallBlind = 0;
		int bigBlind = 0;
		try
		{
			smallBlind = std::stoi(tokens[1]);
			bigBlind = std::stoi(tokens[2]);
		}
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_player.Send(RpcEnum::rpc_server_poker_set_blinds, [room, smallBlind, bigBlind](NetPack& pack) {
			pack.WriteInt32(room);
			pack.WriteInt32(smallBlind);
			pack.WriteInt32(bigBlind);
		});
	} };

	m_commands["CHECK"] = CommandSpec{ 1, false, false,
		MakePokerActionCall(this, HoldemPokerGame::Action::CheckCall, false) };
	m_commands["CALL"] = CommandSpec{ 1, false, false,
		MakePokerActionCall(this, HoldemPokerGame::Action::CheckCall, false) };
	m_commands["BET"] = CommandSpec{ 2, false, false,
		MakePokerActionCall(this, HoldemPokerGame::Action::Bet, true) };
	m_commands["RAISE"] = CommandSpec{ 2, false, false,
		MakePokerActionCall(this, HoldemPokerGame::Action::Raise, true) };
	m_commands["ALLIN"] = CommandSpec{ 1, false, false,
		MakePokerActionCall(this, HoldemPokerGame::Action::AllIn, false) };
	m_commands["FOLD"] = CommandSpec{ 1, false, false,
		MakePokerActionCall(this, HoldemPokerGame::Action::Fold, false) };

	m_commands["ADDHOLDEMBOT"] = CommandSpec{ 1, false, false, [this](const std::vector<std::string>&, int room) {
		if (!RequireRoom(room))
			return;
		m_player.Send(RpcEnum::rpc_server_poker_add_bot, [room](NetPack& pack) {
			pack.WriteInt32(room);
		});
	} };

	m_commands["RMHOLDEMBOT"] = CommandSpec{ 2, true, false, [this](const std::vector<std::string>& tokens, int room) {
		if (!RequireRoom(room))
			return;
		int seatID = 0;
		try { seatID = std::stoi(tokens[1]); }
		catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		m_player.Send(RpcEnum::rpc_server_poker_kick_bot, [room, seatID](NetPack& pack) {
			pack.WriteInt32(room);
			pack.WriteInt32(seatID);
		});
	} };

	m_commands["HELP"] = CommandSpec{ 1, false, false, [](const std::vector<std::string>& tokens, int) {
		if (tokens.size() == 2)
		{
			if (tokens[1] == "-HOLDEM")
				Console::Out() << HelpStrings::HOLDEM << std::endl;
			else if (tokens[1] == "-GENERIC")
				Console::Out() << HelpStrings::GENERIC << std::endl;
			else if (tokens[1] == "-ERROR")
				Console::Out() << HelpStrings::ERRORLIST << std::endl;
			else
				Console::Out() << HelpStrings::CONTENTS << std::endl;
		}
		else
		{
				Console::Out() << HelpStrings::CONTENTS << std::endl;
		}
	} };
}

std::vector<std::string> CommandProcessor::Split(const std::string& str)
{
	std::stringstream ss(str);
	std::string token;
	std::vector<std::string> result;
	while (std::getline(ss, token, ' '))
		result.push_back(token);
	return result;
}

size_t CommandProcessor::SkipSpace(const std::string& text, size_t pos)
{
	while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
		++pos;
	return pos;
}

std::string CommandProcessor::LTrimCopy(const std::string& text)
{
	size_t pos = SkipSpace(text, 0);
	return text.substr(pos);
}

bool CommandProcessor::ParsePrefixes(const std::string& input, PrefixResult& out, size_t& remainderPos, std::string& error)
{
	out = PrefixResult{};
	remainderPos = 0;

	size_t pos = 0;
	while (pos < input.size() && input[pos] == '[')
	{
		size_t end = input.find(']', pos + 1);
		if (end == std::string::npos)
			break;

		std::string token = input.substr(pos + 1, end - pos - 1);
		if (token == "cn" || token == "en")
		{
			out.lang = token;
		}
		else if (token == "text")
		{
			out.mode = Mode::ForceText;
		}
		else if (token == "cmd")
		{
			out.mode = Mode::ForceCommand;
		}
		else if (!token.empty() && token[0] == 'r')
		{
			std::string number = token.substr(1);
			int roomId = 0;
			if (!number.empty() && TryParseInt(number, roomId))
			{
				out.room = roomId;
			}
			else
			{
				error = "ERROR: Invalid room prefix [" + token + "]";
				return false;
			}
		}
		else
		{
			error = "ERROR: Unknown prefix [" + token + "]";
			return false;
		}

		pos = end + 1;
		size_t next = SkipSpace(input, pos);
		if (next < input.size() && input[next] == '[')
		{
			pos = next;
			continue;
		}
		break;
	}

	remainderPos = pos;
	return true;
}

void CommandProcessor::SendTextMessage(const std::string& rawInput, const std::string& message, const PrefixResult& prefix, int room)
{
	if (room < 0)
	{
		Console::Out() << "ERROR: Use USEROOM <id> first to send messages" << std::endl;
		return;
	}

	Console::Out() << "SENDING MSG: " << rawInput << std::endl;

	if (prefix.lang.has_value())
	{
		auto lang = *prefix.lang;
		m_player.Send(RpcEnum::rpc_server_set_language, [lang](NetPack& pack) { pack.WriteString(lang); });
	}

	auto msg = message;
	m_player.Send(RpcEnum::rpc_server_send_text, [room, msg](NetPack& pack) {
		pack.WriteInt32(room);
		pack.WriteString(msg);
		});
}

bool CommandProcessor::RequireRoom(int room) const
{
	if (room < 0)
	{
		Console::Out() << "ERROR: Use USEROOM <id> first" << std::endl;
		return false;
	}
	return true;
}

bool CommandProcessor::TryParseInt(const std::string& s, int& out) const
{
	try
	{
		size_t idx = 0;
		out = std::stoi(s, &idx);
		return idx == s.size();
	}
	catch (const std::exception&)
	{
		return false;
	}
}

std::function<void(const std::vector<std::string>& strVec, int room)> CommandProcessor::MakePokerActionCall(
	CommandProcessor* processor,
	HoldemPokerGame::Action actionType,
	bool needAmount)
{
	return [processor, actionType, needAmount](const std::vector<std::string>& args, int room) {
		if (!processor->RequireRoom(room))
			return;
		int amount = 0;
		if (needAmount)
		{
			try { amount = std::stoi(args[1]); }
			catch (const std::exception& e) { Console::Out() << e.what() << std::endl; return; }
		}
		processor->m_player.Send(RpcEnum::rpc_server_poker_action, [room, actionType, amount](NetPack& pack) {
			pack.WriteInt32(room);
			pack.WriteUInt8(static_cast<uint8_t>(actionType));
			pack.WriteInt32(amount);
			});
		};
}
