// The callback the game runs for every queued event trigger (Game::RunQueuedEvents): the map's "JUMP" family moves
// the player to a point or another map, "SCRIPT" and "NSSCRIPT" start or queue script texts, "ANIME" restarts a node's
// colour animation, "FLAG" sets a script flag, "HIDE" and "GROUND" switch node flags, "LIFTON" starts a lift and
// "DROPEN"/"DRCLOSE" open and close a door.

#include <stdlib.h>
#include <string.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "char_tables.h"
#include "char_vars.h"
#include "camera_vars.h"
#include "field_item_menu.h"
#include "game.h"
#include "game_main.h"
#include "game_vars.h"
#include "kind_counter.h"
#include "map_setup.h"
#include "map_vars.h"
#include "menu_main.h"
#include "partner_follow.h"
#include "render_vars.h"
#include "script_vars.h"
#include "title_screen.h"

extern "C" int FUN_10017ad0(int id);         // Enemy_FindById: character slot of the lift with that id (0 = none)
extern "C" int FUN_10017aa0(int id);         // Enemy_FindByKind: character slot of the door with that id (0 = none)
extern "C" int FUN_1005f530(int index);      // Switch_TurnOn
extern "C" int FUN_1005f640(int index);      // Switch_TurnOff
extern "C" int FUN_1015c9e0(int mode);       // Screen_StartTransition

// The player's motion while a jump or script takes over: idle, blended over a few frames, at normal speed.
#define TRIGGER_PLAYER_MOTION_IDLE 2
#define TRIGGER_PLAYER_MOTION_BLEND 5
#define TRIGGER_MOTION_NORMAL_SPEED 1.0f
// The screen transition after a jump, in frames; DAT_1101d8a8 is set to the transition's mode.
#define TRIGGER_JUMP_TRANSITION_FRAMES 0x1e
#define TRIGGER_JUMP_TRANSITION_MODE 0xf
// SCRIPT: a combo gauge at or above this value is flashed away; a "danger" counter at or above this one resets the
// player's follow-position history (a ring of 16 positions).
#define TRIGGER_COMBO_FLASH_FROM 0x45
#define TRIGGER_COMBO_FLASH_FRAMES 5
#define TRIGGER_DANGER_RESET_FROM 10000
#define TRIGGER_POSITION_HISTORY_LENGTH 16
// Highest script flag index a trigger may set (index 0 means no flag).
#define TRIGGER_LAST_FLAG 0xf9f
// Node flags the "HIDE" and "GROUND" triggers switch.
#define TRIGGER_NODE_FLAG_HIDDEN 0x20
#define TRIGGER_NODE_FLAG_GROUND 4
// The script queue is a ring of eight numbers (DAT_1020247c).
#define TRIGGER_SCRIPT_QUEUE_MASK 7
// Message shown when a lift or door trigger names an id no character has (Shift-JIS).
#define TRIGGER_MESSAGE_FRAMES 0x28
#define TRIGGER_TEXT_BAD_DOOR "\x95s\x90\xb3\x82\xc8\x83h\x83" "A\x82h\x82" "c\x82\xc5\x82\xb7\x81" "B"
#define TRIGGER_TEXT_BAD_LIFT "\x95s\x90\xb3\x82\xc8\x83\x8a\x83t\x83g\x82h\x82" "c\x82\xc5\x82\xb7\x81" "B"

// 0x101008e0 (a C++ function: the Vec4 and Matrix4 locals give it an EH frame). Runs one queued trigger by its name:
// the "JUMP" family moves the player (JUMP: to the coordinates ids[0..2] of map `argument`; JUMP4: to the coordinates
// in the current map; JUMP24: to the node named labels[1]; JUMP2/JUMP3: to the marker labels[1] of map `argument`,
// JUMP3 raised by ids[2]) and starts the screen transition; ids[4] asks the camera to cut instead of follow. Then
// NSSCRIPT shows a script text, SCRIPT resets the player and queues the script, ANIME restarts a node's colour
// animation, FLAG/HIDE/GROUND/LIFTON/DROPEN/DRCLOSE set flags, node flags, lift counters and doors. Returns 0.
int Event_RunTrigger(GameTrigger *trigger)
{
    if (strcmp(trigger->name, "JUMP") == 0)
    {
        DAT_10c62ed0 = trigger->argument;
        DAT_10db6ee8[DAT_104bf8b4].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 1, TRIGGER_PLAYER_MOTION_BLEND,
                                              TRIGGER_MOTION_NORMAL_SPEED);
        Vec4 playerPosition;
        Vec4 cameraPosition;
        Vec4 destination;
        playerPosition.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        cameraPosition.Copy(DAT_118e69e8);
        destination.Set((float)trigger->ids[0], (float)trigger->ids[1], (float)trigger->ids[2], 0.0f);
        DAT_11af3e28.Copy(destination + (cameraPosition - playerPosition));
        DAT_118e6c78.Copy(destination);
        DAT_10db6ee8[0].Teleport(destination.x, destination.y, destination.z);
        FUN_100fc4a0(DAT_10c62ed0, 0, 0);
        DAT_1101d8a8 = TRIGGER_JUMP_TRANSITION_MODE;
        DAT_104c0092 = 0;
        DAT_104c0093 = 0;
        FUN_1015c9e0(TRIGGER_JUMP_TRANSITION_FRAMES);
        if (trigger->ids[4] != 0)
        {
            DAT_11018734 = 1;
            DAT_1101de30 = 1;
        }
    }
    else if (strcmp(trigger->name, "JUMP4") == 0)
    {
        DAT_10c62ed0 = trigger->argument;
        DAT_10db6ee8[DAT_104bf8b4].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 1, TRIGGER_PLAYER_MOTION_BLEND,
                                              TRIGGER_MOTION_NORMAL_SPEED);
        if (trigger->ids[4] != 0)
        {
            DAT_11018734 = 1;
            DAT_1101de30 = 1;
        }
        Vec4 playerPosition;
        Vec4 cameraPosition;
        Vec4 destination;
        playerPosition.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        cameraPosition.Copy(DAT_118e69e8);
        destination.Set((float)trigger->ids[0], (float)trigger->ids[1], (float)trigger->ids[2], 0.0f);
        DAT_11af3e28.Copy(destination + (cameraPosition - playerPosition));
        DAT_118e6c78.Copy(destination);
        DAT_10db6ee8[0].Teleport(destination.x, destination.y, destination.z);
        DAT_1101d8a8 = TRIGGER_JUMP_TRANSITION_MODE;
        DAT_104c0092 = 0;
        DAT_104c0093 = 0;
        FUN_1015c9e0(TRIGGER_JUMP_TRANSITION_FRAMES);
        if (trigger->ids[4] != 0)
        {
            DAT_11018734 = 1;
            DAT_1101de30 = 1;
        }
    }
    else if (strcmp(trigger->name, "JUMP24") == 0)
    {
        DAT_10c62ed0 = trigger->argument;
        DAT_10db6ee8[DAT_104bf8b4].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 1, TRIGGER_PLAYER_MOTION_BLEND,
                                              TRIGGER_MOTION_NORMAL_SPEED);
        if (trigger->ids[4] != 0)
        {
            DAT_11018734 = 1;
            DAT_1101de30 = 1;
        }
        const char *markerName = trigger->labels[1];
        if (markerName != 0)
        {
            Vec4 playerPosition;
            Vec4 cameraPosition;
            Vec4 destination;
            playerPosition.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
            cameraPosition.Copy(DAT_11af3e28);
            Node *marker = DAT_10255f08->Find(markerName, 0);
            if (marker != 0)
            {
                Matrix4 markerMatrix;
                markerMatrix = DAT_10255f08->BoneMatrix((int)marker, 0);
                destination.Copy(markerMatrix.GetTranslation());
            }
            else
                destination.Set(0.0f, 0.0f, 0.0f, 0.0f);
            DAT_11af3e28.Copy(destination + (cameraPosition - playerPosition));
            DAT_118e6c78.Copy(destination);
            DAT_10db6ee8[0].Teleport(destination.x, destination.y, destination.z);
        }
        DAT_1101d8a8 = TRIGGER_JUMP_TRANSITION_MODE;
        FUN_1015c9e0(TRIGGER_JUMP_TRANSITION_FRAMES);
        if (trigger->ids[4] != 0)
        {
            DAT_11018734 = 1;
            DAT_1101de30 = 1;
        }
        DAT_104c0092 = 0;
        DAT_104c0093 = 0;
    }
    else if (strcmp(trigger->name, "JUMP2") == 0)
    {
        if (trigger->ids[4] != 0)
        {
            DAT_1101de30 = 1;
            DAT_11018734 = 1;
        }
        DAT_10c62ed0 = trigger->argument;
        DAT_10db6ee8[DAT_104bf8b4].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 1, TRIGGER_PLAYER_MOTION_BLEND,
                                              TRIGGER_MOTION_NORMAL_SPEED);
        FUN_100fc4a0(DAT_10c62ed0, trigger->labels[1], 0);
        DAT_1101d8a8 = TRIGGER_JUMP_TRANSITION_MODE;
        FUN_1015c9e0(TRIGGER_JUMP_TRANSITION_FRAMES);
        if (trigger->ids[4] != 0)
        {
            DAT_1101de30 = 1;
            DAT_11018734 = 1;
        }
        DAT_104c0092 = 0;
        DAT_104c0093 = 0;
    }
    else if (strcmp(trigger->name, "JUMP3") == 0)
    {
        if (trigger->ids[4] != 0)
        {
            DAT_1101de30 = 1;
            DAT_11018734 = 1;
        }
        DAT_10c62ed0 = trigger->argument;
        DAT_10db6ee8[DAT_104bf8b4].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 1, TRIGGER_PLAYER_MOTION_BLEND,
                                              TRIGGER_MOTION_NORMAL_SPEED);
        FUN_100fc4a0(DAT_10c62ed0, trigger->labels[1], trigger->ids[2]);
        DAT_1101d8a8 = TRIGGER_JUMP_TRANSITION_MODE;
        FUN_1015c9e0(TRIGGER_JUMP_TRANSITION_FRAMES);
        if (trigger->ids[4] != 0)
        {
            DAT_1101de30 = 1;
            DAT_11018734 = 1;
        }
        DAT_104c0092 = 0;
        DAT_104c0093 = 0;
    }
    else if (strcmp(trigger->name, "NSSCRIPT") == 0)
    {
        if (DAT_104bfe6c == 0 && DAT_10c5e1c4[trigger->argument] != 0)
            FUN_1000da20(trigger->argument);
    }
    if (strcmp(trigger->name, "SCRIPT") == 0)
    {
        if (DAT_104bfe6c == 0)
        {
            if (DAT_104c00d0 == 0)
            {
                DAT_10db6ee8[DAT_104bf8b4].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 1, TRIGGER_PLAYER_MOTION_BLEND,
                                                      TRIGGER_MOTION_NORMAL_SPEED);
                DAT_104c0190 = 0;
                if (DAT_104c0108 >= TRIGGER_COMBO_FLASH_FROM)
                    FUN_1000ec70(TRIGGER_COMBO_FLASH_FRAMES, 1.0f, 1.0f, 1.0f, 1.0f);
                DAT_104c0108 = 0;
                if (DAT_104c0228 >= TRIGGER_DANGER_RESET_FROM)
                {
                    DAT_10db6ee8[0].PlayMotion(TRIGGER_PLAYER_MOTION_IDLE, 0, TRIGGER_PLAYER_MOTION_BLEND,
                                               TRIGGER_MOTION_NORMAL_SPEED);
                    DAT_104c0228 = 0;
                    for (int step = 0; step < TRIGGER_POSITION_HISTORY_LENGTH; step++)
                    {
                        DAT_10802ad8[DAT_104c022c].Copy(DAT_10db6ee8[0].matrix.GetTranslation());
                        DAT_104c022c = (DAT_104c022c + 1) & (TRIGGER_POSITION_HISTORY_LENGTH - 1);
                    }
                }
            }
            if (DAT_10c5e1c4[trigger->argument] != 0)
            {
                DAT_1020247c[DAT_104bfe88] = trigger->argument;
                DAT_104bfe88 = (DAT_104bfe88 + 1) & TRIGGER_SCRIPT_QUEUE_MASK;
            }
        }
    }
    else if (strcmp(trigger->name, "ANIME") == 0)
    {
        Node *node = DAT_10255f08->Find(trigger->labels[0], 0);
        if (node != 0)
        {
            node->RestartColourPlugs(trigger->labels[1], DAT_104bf8a4);
            if (trigger->ids[1] <= TRIGGER_LAST_FLAG && trigger->ids[1] != 0)
                DAT_10c62edc[trigger->ids[1]] = trigger->ids[2];
        }
    }
    else if (strcmp(trigger->name, "FLAG") == 0)
    {
        if (trigger->argument <= TRIGGER_LAST_FLAG && trigger->argument != 0)
            DAT_10c62edc[trigger->argument] = trigger->ids[0];
    }
    else if (strcmp(trigger->name, "HIDE") == 0)
    {
        Node *node = DAT_10255f08->Find(trigger->labels[0], 0);
        if (node != 0)
        {
            if (trigger->ids[0] != 0)
                node->SetFlags(TRIGGER_NODE_FLAG_HIDDEN);
            else
                node->ClearFlags(TRIGGER_NODE_FLAG_HIDDEN);
        }
    }
    else if (strcmp(trigger->name, "GROUND") == 0)
    {
        Node *node = DAT_10255f08->Find(trigger->labels[0], 0);
        if (node != 0)
        {
            if (trigger->ids[0] != 0)
                node->SetFlags(TRIGGER_NODE_FLAG_GROUND);
            else
                node->ClearFlags(TRIGGER_NODE_FLAG_GROUND);
        }
    }
    else if (strcmp(trigger->name, "LIFTON") == 0)
    {
        int slot = FUN_10017ad0(trigger->argument);
        if (slot > 0)
            DAT_1020b46c[slot].value = trigger->ids[0];
        else
            FUN_1000ed70(TRIGGER_TEXT_BAD_LIFT, 0, TRIGGER_MESSAGE_FRAMES);
    }
    else if (strcmp(trigger->name, "DROPEN") == 0)
    {
        int slot = FUN_10017aa0(trigger->argument);
        if (slot > 0)
            FUN_1005f530(slot);
        else
            FUN_1000ed70(TRIGGER_TEXT_BAD_DOOR, 0, TRIGGER_MESSAGE_FRAMES);
    }
    else if (strcmp(trigger->name, "DRCLOSE") == 0)
    {
        int slot = FUN_10017aa0(trigger->argument);
        if (slot > 0)
            FUN_1005f640(slot);
        else
            FUN_1000ed70(TRIGGER_TEXT_BAD_DOOR, 0, TRIGGER_MESSAGE_FRAMES);
    }
    return 0;
}
