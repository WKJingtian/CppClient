#pragma once
#include <string>

namespace HelpStrings
{
    static const std::string GENERIC = R"(
========== WKR's chatting room user guide ==========
SETNAME {yourname}: change your name 
SETLANG {language}: change the language of your text. currently, there are only en (English) and cn (Chinese)
MUTE: to mute the chat room
UNMUTE: unmute the chat room
USERLIST: check all online users
MAKEROOM {roomId}: room id has to be a number
TOROOM {roomId}: switch to a different room, room id has to be a number
[{language}] {thingsToSay}: say something to other users in the same room with given language
    for example: \"[en] hello\" is to say hello in english
{thingsToSay}: say something with previously set language
)";

    static const std::string HOLDEM = R"(
================================================================================
                        TEXAS HOLD'EM POKER - HELP
================================================================================

[GAME FLOW]
  1. MAKEROOM 2        - Create a poker room (type 2 = POKER_ROOM)
  2. TOROOM <id>       - Join the room
  3. SETBLINDS <sb> <bb> - Set blinds (e.g., SETBLINDS 5 10)
  4. SIT <seat>        - Sit at a seat (e.g., SIT 0)
  5. BUYIN <amount>    - Buy chips (min = big blind x 100)
  6. Wait for players  - Game starts when 2+ players are ready
  7. Play the hand     - Use CHECK/CALL/BET/RAISE/FOLD
  8. Receive results   - Hand result is broadcast automatically
  9. STANDUP           - Sit out (optional)
 10. TOROOM -1         - Leave room, chips return to wallet

--------------------------------------------------------------------------------
[COMMAND REFERENCE]
--------------------------------------------------------------------------------
  ROOM COMMANDS:
    MAKEROOM <type>      Create room (0=Hall, 1=Chat, 2=Poker)
    TOROOM <id>          Join room (-1 = return to hall)

  TABLE SETUP:
    SETBLINDS <sb> <bb>  Set small/big blind (before game starts)
    SIT <seat>           Sit at seat number
    BUYIN <amount>       Buy chips from wallet to table
    STANDUP              Sit out from table
    TABLEINFO            Request current table state

  GAME ACTIONS:
    CHECK                Check (when no bet to call)
    CALL                 Call current bet
    BET <amount>         Place a bet
    RAISE <amount>       Raise to specified amount
    FOLD                 Fold your hand

--------------------------------------------------------------------------------
[GAME STAGES]
--------------------------------------------------------------------------------
  0 = Waiting    - Waiting for players / between hands
  1 = PreFlop    - Hole cards dealt, betting round
  2 = Flop       - 3 community cards, betting round
  3 = Turn       - 4th community card, betting round
  4 = River      - 5th community card, betting round
  5 = Showdown   - Reveal hands, determine winner

--------------------------------------------------------------------------------
[RULES]
--------------------------------------------------------------------------------
  * Minimum buy-in = Big Blind x 100
  * Blinds must be set before game can start
  * At least 2 players with chips >= big blind to start
  * Cannot buy-in during a hand
  * Leaving room auto-returns table chips to wallet
  * Leaving during hand = auto-mode (check if free, else fold)

--------------------------------------------------------------------------------
[ERROR CODES]
--------------------------------------------------------------------------------
  300 = Insufficient chips in wallet
  301 = Buy-in failed (database error)
  302 = Blinds not set
  303 = Not seated at table
  304 = Already in a hand

================================================================================
)";
}