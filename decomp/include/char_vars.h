// Globals of characters, enemies, NPCs and the objects they use.
// Generated from the per-file extern declarations by tools/globals_driver.py; this is the one
// declaration of each of them: include this header, never redeclare one. Comments say what is known.
#ifndef GURUMIN_CHAR_VARS_H
#define GURUMIN_CHAR_VARS_H

#ifndef DEBRIS_SLOTS
#define DEBRIS_SLOTS 0x100
#endif

#include "vec3.h"
#include "vec4.h"
#include "model_path.h"

struct Node;   // node.h
struct Playback;   // anim_model.h

extern "C" const char DAT_101d67f0[5];                    // character slots checked by FUN_10034e80: 0, 13, 15, 58, 20; the party's character slots: 0, 13, 15, 58 and 20
extern "C" const char DAT_101d76d0[];                     // "オブジェ数オーバーだぞよ"
extern "C" int DAT_102022b8;                              // non-zero = always place characters on the ground
extern "C" unsigned char DAT_1020d4c2[];                  // per-character count compared with 4
extern "C" char DAT_1020da24[];
extern "C" int DAT_10210fb8;                              // next slot of the pickup effect ring
extern "C" char DAT_102194c8[];
extern "C" char DAT_102195f0[0x100];                      // cleared on spawn
extern "C" int DAT_102196f0[0x20];
extern "C" int DAT_10219778[];                            // a per-character level, at most 5
extern "C" char DAT_1021b628[0x100];                      // cleared on spawn
extern "C" char DAT_1021bbc0[0x30];                      // a flag per camera position (DAT_1032d5b8)
extern "C" int DAT_1021f338;
extern "C" int DAT_10254798[];
extern "C" unsigned char DAT_102550e2[];                  // a per-character value; the larger of it and DAT_102550e0 is kept
extern "C" int DAT_1030bdf8[0x100];                       // piece type, 0 = free slot
extern "C" float DAT_1030cd98[];                          // their radius
extern "C" unsigned char DAT_10315350[0x80];             // a per-character flag (0x80 character slots)
extern "C" unsigned char DAT_10315352[];                  // a per-character flag, cleared every frame
extern "C" short DAT_103153b4[];                          // a per-character counter, at most 15
extern "C" Vec3 *DAT_1032d5b8[0x30];                     // positions the camera keeps in view (characters moved by routes)
extern "C" int DAT_1032d69c;                              // 1 once the game built-ins are registered; 0 while a queued script runs
extern "C" int DAT_1042db08[];
extern "C" int DAT_104465f8[DEBRIS_SLOTS];                // spin: rotation in tenths of a radian
extern "C" int DAT_104469f8[0x100];                       // frames left (0 for the simple pieces)
extern "C" Node *DAT_10446e00[0x28];                      // the shaga models (shaga_spawn.cpp)
extern "C" int DAT_10446eb0[0x100];                       // colour of a thrown piece
extern "C" unsigned char DAT_104677a0[DEBRIS_SLOTS];      // bounces in a row, cleared on spawn
extern "C" char DAT_104686b2[];                           // per-character mode (7 = the mode handled here)
extern "C" char DAT_1049efb0[10][10];                     // the wave groups: character of each slot (0 = empty)
extern "C" int DAT_1049f0f0;                              // number of entries in the two lists below; number of camera positions
extern "C" unsigned int DAT_104bf890;                     // frame counter
extern "C" int DAT_104bfd08;                              // non-zero = enemies do not move
extern "C" int DAT_104bfdd8;                              // the carried object's slot (below 0x5c: nothing carried)
extern "C" int DAT_104bfe28;                              // non-zero when the map has a pit
extern "C" int DAT_104bfe6c;                              // non-zero in the mode where only positional sounds are turned down; 1 while sounds are muted (cut-scenes)
extern "C" int DAT_104bfe78;                              // non-zero while character updates are suspended; 1 once the game over has started
extern "C" char DAT_104c0093;                             // non-zero = damage reduced to two thirds
extern "C" char DAT_104c0094;                             // non-zero = only a few behaviours run (an event is playing)
extern "C" int DAT_104c00c8;
extern "C" char DAT_104c00cc;                             // 2 = the extra life was used up this hit
extern "C" int DAT_104c00d0;                              // when non-zero, the player (character 0) cannot be pushed
extern "C" unsigned int DAT_104c00e0;                              // non-zero = greetings are on
extern "C" int DAT_104c00e8;                              // the map area (greetings in areas 0x10, 0x14-0x19 and 0x28)
extern "C" Node *DAT_104c00f8;                            // non-zero = keep the offsets up to date; the player's "scop-pt" node (0 = none)
extern "C" Node *DAT_104c00fc;                            // the player's "charbase0" node
extern "C" Node *DAT_104c0100;                            // "scop-2pt" (costume 6 only)
extern "C" Node *DAT_104c0104;                            // "scop-3pt" (costume 6 only)
extern "C" char DAT_104c0119;                             // HUD counter display timer
extern "C" char DAT_104c0135;
extern "C" int DAT_104c013c;
extern "C" int DAT_104c02c8;                              // face settings of the worn helmet (Helmet_FaceSettings)
extern "C" char DAT_104c0319;                             // set to 1 when a helmet is attached
extern "C" char DAT_104c0332;                             // when set, sounds requested with `optional` are skipped; non-zero = voices off
extern "C" unsigned char DAT_104c033a;                             // face variant: from the costume and the helmet worn with it; face variant (char_attach.cpp)
extern "C" int DAT_104c0378;                              // frames before the voice is cleared after walking away
extern "C" unsigned short DAT_104c0380;                   // frame counter
extern "C" int DAT_104c0384;                              // the object it rests on (0 = none yet)
extern "C" int DAT_104c0388;                              // the carried slot DAT_104c0384 was found for
extern "C" Vec4 DAT_104c0438[0x400];                      // drop positions
extern "C" Vec4 DAT_10802ac8;                             // where the player's head looks
extern "C" Vec4 DAT_10803118;                             // character 20's resting place
extern "C" Vec4 DAT_10836c74;                             // reference position (in the engine object at 0x10836c70)
extern "C" Vec4 DAT_10866df8[0x100];                      // scale
extern "C" Vec4 DAT_10867df8;                             // top of the object below; z becomes the height above it
extern "C" char DAT_10894698[][0x124];                    // model paths
extern "C" char DAT_108f678c[][0x240];                    // item model paths
extern "C" short DAT_10b314c0;                            // last face frame set
extern "C" ModelPath DAT_10b314c8[];                     // the model file of each model number
extern "C" int DAT_10c62ed0;                              // current map
extern "C" int DAT_10c63520;                              // 1 while a rider stands on the map cursor
extern "C" int DAT_10c6351c;                              // the character being talked to
extern "C" int DAT_10c6358c;                              // look-around step: 0..9 = one side, -1..-9 = the other side
extern "C" int DAT_10c635a8;                              // wave enemies still alive
extern "C" int DAT_10c635f8;
extern "C" int DAT_10c64ad0;                              // non-zero = the bank holds 400
extern "C" int DAT_10c64ad4;                              // drill level 0..3
extern "C" int DAT_10c64ad8;                              // the player's stamina gauge; banked hundreds
extern "C" int DAT_10c655e0;
extern "C" int DAT_10c655f0;                              // current costume (6 = the one with three pointer nodes); 6 = the player has no name yet
extern "C" int DAT_10c65648;
extern "C" int DAT_10c6564c;
extern "C" int DAT_10c6566c;                              // 1 = drill 5
extern "C" int DAT_10c66710;                              // non-zero: wake-up radius forced to 300
extern "C" int DAT_10c66cb0;                              // cleared when costume 6 is revoked (its unlock bits are incomplete)
extern "C" int DAT_10c66cc0;                              // non-zero = message words are replaced; non-zero = voices are not limited
extern "C" int DAT_10c66d00;
extern "C" Vec4 DAT_10c66ef0[0x100];                      // velocity
extern "C" Vec4 DAT_10da9160[];                           // their velocity
extern "C" Vec4 DAT_10da9400[DEBRIS_SLOTS];               // position
extern "C" int DAT_10daabb8;                              // the player's costume
extern "C" Node *DAT_10db91dc;                            // the player's head node (character 0, field 0x2304)
extern "C" float DAT_10db9364;                            // the player's heading (character 0)
extern "C" int DAT_10de6e80;                              // character 20's frame counter (CharState of character 0x5d)
extern "C" int DAT_10f654b0;
extern "C" float DAT_10f668c0;                            // its depth limit
extern "C" char DAT_10fc6a90[];
extern "C" Playback *DAT_1101d880;                        // motion playback of the character being moved
extern "C" char DAT_1101de68[0x114450];                   // effect definitions with the data before them (cleared by Init)
extern "C" Vec4 DAT_118e69e8;                             // player position; camera position
extern "C" Vec4 DAT_118e6cb8;                             // the player's position here (player_reset.cpp calls it the player velocity); camera target
extern "C" float DAT_118e6cc0;                            // the height the drops land at
extern "C" int DAT_11b002c0;                              // collision filter of the current test (a character's collisionFilter); collision filter of the current test (the model's collisionFilter); collision filter of the current test (a character's nearCells)

#endif
