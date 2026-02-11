#include "GameElementPrinter.h"
#include "Game/HoldemPokerGame.h"
#include "Utils/Console.h"
#include "Game/GameItem/HandEvaluator.h"
#include "ServerClass/Room.h"
#include "Player/PlayerInfo.h"

void GameElementPrinter::Print(const HoldemPokerGame& target, int roomId)
{
    auto stage = target.GetStage();
    int pot = target.GetTotalPot();
    int actingPlayer = target.ActingPlayerId();

    const auto& community = target.GetCommunity();
    const auto& seats = target.GetSeats();

    Console::Out() << "Room " << roomId << " - Stage: " << (int)stage
        << ", Pot: " << pot
        << ", Acting: " << actingPlayer << std::endl;

    Console::Out() << "  Community: ";
    for (const auto& c : community)
        if (c.IsValid()) Console::OutW() << c.ToString() << " ";
    Console::Out() << std::endl;

    for (const Seat& seat : seats)
    {
        if (seat.playerId < 0) continue;
        Console::Out() << "  Seat " << seat.seatIndex
            << ": Player " << seat.playerId
            << ", Chips: " << seat.chips
            << (seat.inHand ? " [IN HAND]" : "")
            << (seat.folded ? " [FOLDED]" : "")
            << (seat.sittingOut ? " [SITTING OUT]" : "")
            << (seat.seatIndex == target.GetBigBlindSeatIndex() ? " [BB]" : "")
            << (seat.seatIndex == target.GetSmallBlindSeatIndex() ? " [SB]" : "")
            << (seat.seatIndex == target.GetDealerSeatIndex() ? " [D]" : "")
            << (seat.folded ? " [FOLDED]" : "")
            << std::endl;

        if (seat.hole[0].IsValid())
        {
            Console::OutW() << L"    Hole: " << seat.hole[0].ToString()
                << " " << seat.hole[1].ToString() << std::endl;
        }
    }
}

void GameElementPrinter::Print(const HandResult& target, int roomId)
{
    Console::Out() << "=== Hand Result (Room " << roomId << ") ===" << std::endl;
    Console::Out() << "Total pot: " << target.totalPot << std::endl;
    Console::Out() << "Community: ";
    for (const auto& c : target.communityCards)
        if (c.IsValid()) Console::OutW() << c.ToString() << " ";
    Console::Out() << std::endl;

    for (const auto& pr : target.playerResults)
    {
        Console::Out() << "  Player " << pr.playerId << ": ";
        if (pr.folded)
            Console::Out() << "FOLDED";
        else
        {
            Console::OutW() << pr.holeCards[0].ToString() << " " << pr.holeCards[1].ToString();
            Console::Out() << " (" << HandEvaluator::ParseScore(pr.handRank) << ")";
        }
        if (pr.chipsWon > 0)
            Console::Out() << " WON " << pr.chipsWon;
        Console::Out() << std::endl;
    }
    Console::Out() << "=== === === === === === === ===" << std::endl;
}

void GameElementPrinter::Print(const Card& target)
{
    Console::OutW() << target.ToString() << std::endl;
}

void GameElementPrinter::Print(const Room& target)
{
    Console::Out() << "Room " << target.GetRoomId() << " (Type: " << target.GetTypeName() << "): " << std::endl;
    for (const auto& member : target.GetMembers())
        Print(member);
}

void GameElementPrinter::Print(const PlayerInfo& target)
{
    Console::Out() << "player info with _id=" << target.GetID() << std::endl;
    Console::Out() << "\t nickname: " << target.GetName() << std::endl;
    Console::Out() << "\t language: " << (int)target.GetLanguage() << std::endl;
    Console::Out() << "\t chip: " << target.GetChip() << std::endl;
}
