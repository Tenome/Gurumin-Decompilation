// The script value commands (value_command.cpp): the command numbers 0x2716..0x4e69 of the script system's
// "value" calls. They work on the three text windows, the window panel lines, the costume and furniture bits, the
// character slots, the item stock and the map objects; their arguments and results travel in the registers
// DAT_10c6356c.. (camera_vars.h, scene_command.h, script_system.h).
#ifndef GURUMIN_VALUE_COMMAND_H
#define GURUMIN_VALUE_COMMAND_H

#include "vec4.h"

// Command numbers of Script_ValueCommand. Numbers from 30000 on go to the scene commands (0x1009e380), from 40000
// on also to Script_SystemCommand (script_system.h).
enum ValueCommand
{
    VALUE_WINDOW_CLOSE = 0x2716,            // closes text window `value` and drops its picture
    VALUE_WINDOW_SET_STYLE = 0x2717,        // style of the current window from the layout number `value` (0..8)
    VALUE_WINDOW_OPEN = 0x271a,             // sizes the current window to its panel lines (or picture) and shows it
    VALUE_WINDOW_SET_FADE = 0x271b,         // frames the current window fades in (-1 does the same)
    VALUE_MESSAGE_OPEN_PANEL = 0x271c,      // opens a message box with the first four panel lines
    VALUE_MESSAGE_OPEN_LINE = 0x271f,       // opens message `value` with the panel line named by register 0
    VALUE_NOP_2774 = 0x2774,
    VALUE_KAGU_CLEAR = 0x27d7,              // empties the furniture node "kagunode" of the map
    VALUE_KAGU_PLACE = 0x27d8,              // puts the furniture of spot `value` into "kagunode" (when not in stock)
    VALUE_KAGU_PLACE_BOUGHT = 0x27d9,       // puts every furniture whose bit is set in DAT_10c64adc
    VALUE_HALVE_COUNT = 0x27da,             // DAT_10c64a44 = DAT_10c64a48 (1 at most unless DAT_10c65658) / 2
    VALUE_COSTUME_SET_BIT = 0x27db,         // sets the bit of the costume item `value` in DAT_10c64adc
    VALUE_COSTUME_COUNT = 0x27dc,           // register 0 = costumes owned in group `value` (4 = all groups)
    VALUE_COSTUME_CLEAR_ALL = 0x27dd,       // takes every costume item out of the stock and clears its bit
    VALUE_SHOW_RESULT = 0x2904,             // builds and shows the result card of a time run and records the best
    VALUE_CLAMP_63 = 0x2968,                // register 0 = `value` limited to 0..63
    VALUE_GET_STATE_CODE = 11000,           // register 0 = the number of the first state flag that is set (0..6)
    VALUE_CHAR_RESET_KIND = 12000,          // character slot `value` becomes kind -1 (taken marker 2)
    VALUE_CHAR_CLEAR_STATE = 0x2ee1,        // clears the behaviour state of character `value`
    VALUE_CHAR_SET_KIND = 0x2ee2,           // kind of character `value` = register 0
    VALUE_CHAR_CARRY_MOTION = 0x2ee3,       // starts a motion on character `value` and raises its carried marks
    VALUE_CHAR_GET_KIND = 0x2ee4,           // register 0 = kind of character `value`
    VALUE_CHAR_SET_SLOT_KIND = 0x2ee5,      // slot kind marker of character `value` = register 0
    VALUE_CHAR_UPDATE = 0x2ee6,             // runs the per-slot update of character `value`
    VALUE_CARRY_SET = 0x2ee7,               // puts item `value` into a carrier compartment (0 takes it out)
    VALUE_ROUTE_SET_KIND = 0x2ee8,          // every character walking route record `value` gets kind register 0
    VALUE_CHAR_SET_TABLE_9 = 0x2ee9,        // byte table DAT_10254090 of character `value` = register 0
    VALUE_CHAR_SET_VARIANT = 0x2eea,        // kind variant of character `value` = register 0
    VALUE_CHAR_PLACE_SHADOW = 0x2eeb,       // puts the shadow of character `value` on the ground under the light
    VALUE_CHAR_SET_MARK = 0x2eec,           // byte at 0x12 of character `value`'s model = register 0
    VALUE_SET_INVULNERABLE = 0x2eed,        // DAT_104c00b8 = `value`
    VALUE_PANEL_CHECKSUM = 13000,           // register 0 = character sum of the first panel line (twice)
    VALUE_MESSAGE_SHOW = 20000,             // message window with the first two panel lines for `value` frames
    VALUE_GET_STOCK = 0x4e21,               // register 10 = stock of item `value`
    VALUE_REMOVE_ITEM_100 = 0x4e22,         // takes 100 of item `value`
    VALUE_NOP_4E23 = 0x4e23,
    VALUE_CHECK_COLLECTION = 0x4e25,        // sets DAT_10c647d8 when every collection item is owned or flagged
    VALUE_REMOVE_ITEM = 0x4e26,             // takes register 0 of item `value`
    VALUE_GET_HELMET = 0x4e28,              // register 10 = the helmet worn
    VALUE_GET_MAP_OR_MIRROR = 0x4e2a,       // 0: register 0 = map; 1: mirrors the player's x
    VALUE_NOP_4E2B = 0x4e2b,
    VALUE_NOP_4E2C = 0x4e2c,
    VALUE_SET_HEALTH = 0x4e2d,              // the player's health = `value`
    VALUE_NEAREST_ZONE = 0x4e2e,            // register 10 = the zone of roamer `value` that is nearer to it
    VALUE_SET_SPEAKER_NODE_ID = 0x4e34,     // register 5 = 0x3a (value 0) or 0x5c (value 1)
    VALUE_FACE_CAMERA_AXIS = 0x4e35,        // turns the active character to the camera's axis
    VALUE_GAME_OVER = 0x4e36,               // starts the game over sequence
    VALUE_PLAYER_FACE_POINT = 0x4e37,       // player faces the shot point and plays motion 0x58
    VALUE_PLAYER_TURN_RESTORE = 0x4e38,     // player's target heading = register 9 / 1000
    VALUE_PLAYER_TURN_RELATIVE = 0x4e39,    // player's heading = `value` degrees relative to the shot point
    VALUE_PLAYER_MOTION = 0x4e3a,           // player plays motion `value`
    VALUE_PLAYER_ATTACK = 0x4e3b,           // starts attack `value`
    VALUE_GREETINGS = 0x4e3c,               // register 0 = greetings on; sets the greeting balloons
    VALUE_CLEAR_FLAGS = 0x4e3d,             // clears every script flag but three and refreshes the shadows
    VALUE_CHAR_GET_SWITCHES = 0x4e3e,       // register 0 = switches held by character `value`
    VALUE_FIND_MARKED_SLOT = 0x4e3f,        // register 1 = the `value`-th slot (0x5c..0x7b) of kind '`'
    VALUE_RAISE_MAX_HEALTH = 0x4e40,        // `value` 0: maximum health + 10
    VALUE_NOP_4E41 = 0x4e41,
    VALUE_PAY_ITEM = 0x4e42,                // takes the price of item `value` (with the shop percentage) from the coins
    VALUE_GET_PRICE = 0x4e43,               // register 0 = price `value` with the shop percentage and discounts
    VALUE_KAGU_ITEM_PLACE = 0x4e44,         // puts furniture `value` onto the stand
    VALUE_KAGU_STAND_HIDE = 0x4e45,         // hides the stand's parent node
    VALUE_PLAYER_ATTACK_NEAR = 0x4e46,      // the player attacks the nearest of the characters 58..123
    VALUE_CHAR_FACE_POINT = 0x4e47,         // character `value` faces the player's point
    VALUE_REMOVE_EFFECT_HOLD = 0x4e48,      // moves the held effects `value` out of sight
    VALUE_CLEAR_104C0110 = 0x4e49,          // DAT_104c0110 = 0
    VALUE_PLAYER_TURN_TO_REGISTERS = 0x4e4a,   // player turns by `value` degrees towards the point of registers 0, 1
    VALUE_AND_REGISTER = 0x4e4b,            // register 0 &= `value`
    VALUE_SET_STATE_WORD = 0x4e4c,          // state word register 1 of character register 0 = `value`
    VALUE_SWITCH_HOLD = 0x4e4d,             // character `value` holds a switch of its node (@Thunder or @Fire)
    VALUE_KNOCK_BACK = 0x4e4e,              // knocks the player back
    VALUE_WATER_FREEZE = 0x4e4f,            // register 0 = water frozen; `value` 1 freezes, 0 thaws (with the music)
    VALUE_GET_STATE_WORD = 0x4e50,          // register 0 = state word register 1 of character `value`
    VALUE_SET_SCALE_FROM_REGISTERS = 0x4e51,   // scale of character `value` = registers 0..2 / 100
    VALUE_SET_SCALE_SMALL = 0x4e52,         // scale of character 93 = 0.3
    VALUE_STAMINA_UP = 0x4e53,              // stamina gauge + 100
    VALUE_OR_REGISTER = 0x4e54,             // register 0 |= `value`
    VALUE_GET_104BFD0C = 0x4e55,            // register 0 = DAT_104bfd0c
    VALUE_SET_VECTOR = 0x4e56,              // DAT_1083631c = registers 0..3
    VALUE_NEAREST_CHAR = 0x4e57,            // register 0 = nearest character (1..91) to the player, but not `value`
    VALUE_CHECK_SAVE_ALLOWED = 0x4e58,      // register 0 = 1 when the current map allows the action
    VALUE_SET_OVERLAY_FLAG = 0x4e59,        // DAT_104bfe1c = `value`
    VALUE_STAMINA_DOWN = 0x4e5a,            // stamina gauge - 100
    VALUE_GET_PICKUPS = 0x4e5b,             // register 0 = pickups of the player
    VALUE_SWITCH_RELEASE = 0x4e5c,          // releases the switch of object `value` (-1 = every object)
    VALUE_SET_104C0110 = 0x4e5d,            // DAT_104c0110 = `value`
    VALUE_COPY_STATE_WORD = 0x4e5e,         // register 2 = state word register 1 of character register 0
    VALUE_KAGU_SWAP_NODES = 0x4e5f,         // swaps the visible furniture nodes of the stand
    VALUE_GET_OBJECT_FIELD = 0x4e60,        // register 0 = first field of map object `value`
    VALUE_SET_104C012C = 0x4e61,            // DAT_104c012c = `value`
    VALUE_NEGATE_104C012C = 0x4e62,         // DAT_104c012c = -DAT_104c012c
    VALUE_GET_RESTRICTED = 0x4e63,          // register 0 = the player is restricted
    VALUE_PLAYER_SHOUT = 0x4e64,            // lowers the music and plays motion 0x59
    VALUE_SET_RECORD_FIELD = 0x4e65,        // map record register 0, row register 1, column register 2 = `value`
    VALUE_SET_COSTUME = 0x4e66,             // the current costume = `value`
    VALUE_SET_104C635F8 = 0x4e67,           // DAT_10c635f8 = `value`
    VALUE_PUSH_AWAY = 0x4e68,               // pushes the player away from the point of registers 0..2
    VALUE_CLEAR_POST_EFFECT = 0x4e69        // post effect strength 0
};

// One furniture spot of the item stand (0x20 bytes; DAT_1090865c, indexed by the spot number).
struct KaguSpot
{
    int item;           // 0x00: item number of the furniture (-1 = none); the furniture is placed when its stock is 0
    int x;              // 0x04
    int y;              // 0x08
    int z;              // 0x0c
    int angle;          // 0x10: turn about z in degrees
    int pad_0014[3];
};

// The thresholds the result card of a time run compares with (0x102169c0).
struct ResultThresholds
{
    int enemies;        // 0x00: enemies to defeat (90 % of it earns a point)
    int chestsA;        // 0x04: chests (with chestsB; 90 % earns a point)
    int chestsB;        // 0x08
    int pad_000c;
    int deaths;         // 0x10: game overs allowed
    int time;           // 0x14: best time as minutes * 100 + seconds
    int pad_0018;
    int accuracy;       // 0x1c: needed value of the accuracy score
};

extern "C" KaguSpot DAT_1090865c[];             // the furniture spots of the stand
extern "C" ResultThresholds DAT_102169c0;       // the rank thresholds of the result card
extern "C" int DAT_1090a700[][0x3a];            // map object records of 0xe8 bytes; the first field is read
extern "C" unsigned int DAT_10c64adc;           // bits of the furniture and costumes bought (script flag 0x700)
extern "C" int DAT_10c64a44;                    // script flag
extern "C" int DAT_10c64a48;                    // script flag
extern "C" int DAT_10c65658;
extern "C" int DAT_10c64680;                    // state flags read by VALUE_GET_STATE_CODE
extern "C" int DAT_10c64764;
extern "C" int DAT_10c65240;
extern "C" int DAT_10c65238;
extern "C" int DAT_10c65230;
extern "C" int DAT_10c65294;
extern "C" int DAT_10c64768;                    // non-zero = the result card keeps the music
extern "C" int DAT_10c647d8;                    // 1 while every collection item is owned
extern "C" int DAT_10c64878;                    // non-zero once item 0x27 was owned
extern "C" int DAT_10c635cc;                    // set to 1 by VALUE_GREETINGS
extern "C" int DAT_10c63edc;                    // enemies of the run
extern "C" int DAT_10c63f3c;                    // chests of the run
extern "C" int DAT_10c63fcc;                    // jars of the run
extern "C" int DAT_10c64000;                    // game overs of the run
extern "C" int DAT_10c64008;                    // hits the player took in the run
extern "C" int DAT_10c6400c;                    // time offset added to the tick count of the run
extern "C" int DAT_10c64020;                    // successful hits of the run
extern "C" int DAT_10c64024;                    // accuracy score of the run
extern "C" int DAT_10c6402c;                    // first record flag of the best result of the run's map

// 0x101011b0 (Script_ValueCommand; C++ linkage). Runs value command `id` with `value`; numbers from 30000 on are
// passed on to the scene commands and from 40000 on also to Script_SystemCommand. Returns 0.
int Script_ValueCommand(int id, int value);

#endif
