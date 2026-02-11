#pragma once

class HoldemPokerGame;
struct HandResult;
class Card;
class Room;
class PlayerInfo;
class GameElementPrinter
{
public:
    static void Print(const HoldemPokerGame& target, int roomId);
    static void Print(const HandResult& target, int roomId);
    static void Print(const Card& target);
    static void Print(const Room& target);
    static void Print(const PlayerInfo& target);
};
