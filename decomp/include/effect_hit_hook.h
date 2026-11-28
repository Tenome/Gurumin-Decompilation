// The host's hit callback for effects (effect_hit_hook.cpp, 0x10093cc0, host callback slot 0 of DAT_1101de40) and the
// names of the values it uses.
#ifndef GURUMIN_EFFECT_HIT_HOOK_H
#define GURUMIN_EFFECT_HIT_HOOK_H

#include "vec4.h"
#include "matrix4.h"

struct EffectOptions;   // effect_manager.h
struct Node;            // node.h

// 0x10093cc0. Tests the sphere (x, y, z, size) or, when options->settings[3] is set, the capsule from options->m_0034 to
// options->m_0038 against the characters the effect can hit and applies the hit. An effect that belongs to the player
// (options->m_0000 != 0) is tested against the enemies (slots 0x3a..0x7b), the breakable objects and then the water, the
// kskon props and the fire/candle lights; any other effect is tested against the player. The first character hit
// takes damage, scoring, effects and its death. Returns 0 for no hit, 1 or the hit result when something was hit, and
// -1 when the effect is blocked by the map (the hit normal then goes to *options->m_0030).
int FUN_10093cc0(float x, float y, float z, float size, EffectOptions *options);

// Helpers of other files that have no shared declaration yet.
extern "C" int FUN_10011b70(int strength, char flag, int index, const Vec4 *point);   // char_turn.cpp: pushes character `index` (0 = the player) away from `point`
extern "C" void FUN_1003ab40(int id, int quiet);                                     // pickups.cpp: collects pickup `id`
extern "C" int FUN_10057c40(int holder, Node *light, int kind);                      // switches: a strike holds the switch of `light`
extern "C" int FUN_10057b90(int holder);                                             // switches: releases the switch of `holder`
extern "C" int FUN_10017b10(int index, int *high, int *low);                         // splits script variable `index` into item and count
extern "C" int FUN_10062750(const Vec4 *position, const char *model, int variant, float scale, float spin);   // shaga_spawn.cpp

// Character slots the hook walks (the enemies, then the first props).
#define HIT_FIRST_ENEMY_SLOT 0x3a
#define HIT_END_ENEMY_SLOT 0x7c
#define HIT_END_OBJECT_SLOT 0x5c   // the second loop walks the slots 0x3a..0x5b
#define HIT_NODE_MAX 129           // nodes one test can report
#define HIT_PROP_COUNT 0x40        // "@kskNN_" props
#define HIT_BONUS_PROP_SLOT 0x7d   // first of the three slots a hit prop copies its mesh into

// DAT_1021f3c0 (slot kind) values the hook reacts to.
#define HIT_KIND_ENEMY 3           // a live enemy (also needs DAT_10210d30 == 0)
#define HIT_KIND_CRATE 5           // breakable crate: breaks into "O_001br.it3"
#define HIT_KIND_GUARD_Q 0x51      // 'Q': armoured enemy that parries and takes counted hits
#define HIT_KIND_GUARD_COMMA 0x2c  // ',': like 'Q', leaves "model[leftModel]" behind
#define HIT_KIND_TREASURE 0x50     // 'P': breakable treasure that drops items by its script variable
#define HIT_KIND_POT 0x7a          // 'z': pot that breaks into "O_061br.it3" (only while DAT_104c01e0 is 0)
#define HIT_KIND_SPARE_63 0x63     // 'c', '`' (0x60) and 'M' (0x4d): pass the filter, no special handling
#define HIT_KIND_SPARE_60 0x60
#define HIT_KIND_SPARE_4D 0x4d
#define HIT_KIND_CRATE_BROKEN ((char)0xd4)   // what a broken crate becomes
#define HIT_KIND_GUARD_DONE ((char)0xaf)
#define HIT_KIND_TREASURE_DONE ((char)0xb0)
#define HIT_KIND_POT_DONE ((char)0x86)
#define HIT_KIND_SPARE_FB ((char)0xfb)

// Bits of options->settings[4] (the kind of strike).
#define STRIKE_SLASH 1
#define STRIKE_PIERCE 2
#define STRIKE_SWORD 8             // the player's sword (parried by the armoured kinds)
#define STRIKE_HEAVY 0x10
#define STRIKE_DOUBLE 0x20         // counts two hits

// options->m_000e: the element of the strike.
#define ELEMENT_FIRE 1
#define ELEMENT_THUNDER 2
#define ELEMENT_ICE 3
#define ELEMENT_BLAST 5
#define ELEMENT_PLAYER_HIT 6

// CharSlot fields the hook reads for enemies (see char_slot.h for the layout).
#define STAT_NEGATIVE_HIT 12       // CharSlot::stats[12] (0x34): 1 while the hit that is applied has a negative damage
#define STAT_DEFENCE STAT_VALUE_4  // CharSlot::stats[4] (0x14): subtracted from the damage of every hit

#endif
