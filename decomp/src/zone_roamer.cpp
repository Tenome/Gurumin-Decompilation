// The zone roamer: a party character (slots 0xd, 0xf and 0x3a) that wanders between the zones of the field maps 2, 3,
// 4, 5 and 0xc until the player talks to it (the per-kind behaviour 0x10049ec0 of 0x100f9670).

#include <math.h>
#include <string.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "effect_manager.h"

#include "anim_model.h"

#include "char_state.h"

#include "char_slot.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game.h"
#include "game_vars.h"
#include "last_position.h"
#include "map_vars.h"
#include "model_bank.h"
#include "world.h"

#include "zone_roamer.h"

extern "C" int FUN_1013f880(void);                                   // the game's random number
extern "C" float FUN_10001f80(float y, float x);                      // atan2f
extern "C" int FUN_10034cc0(int self, const Vec4 &step, int range);  // Party_BlocksMove
extern "C" int FUN_10034e80(int index, const Vec4 *offset, int range);  // 1 when a party character is obstructed
extern "C" int FUN_100242c0(int index, Vec4 *ground, float depth, int fromHead, int testOnly, const Vec4 *offset);  // Char_ProbeAhead

#define ROAM_STATE_WALK 0            // heads for the next zone (walk mode 0: to the zone it is in, 1: to the next one)
#define ROAM_STATE_REST 1            // stands in a resting zone and looks around
#define ROAM_STATE_LEAVE 2           // walks to the door and fades out
#define ROAM_STATE_ARRIVE 3          // fades in at the far side
#define ROAM_STATE_DOOR 4            // reappears at the door of the next map
#define ROAM_FLAG_BASE_SLOT_D 0xf96  // script flags: next zone, zone it is in, zone it came from
#define ROAM_FLAG_BASE_SLOT_F 0xf93
#define ROAM_FLAG_BASE_SLOT_3A 0xf90
#define ROAM_FLAG_BASE_DEFAULT 0xf9a
#define ROAM_SLOT_D 0xd
#define ROAM_SLOT_F 0xf
#define ROAM_SLOT_3A 0x3a
#define ROAM_SLOT_QUIET 0x14         // does not start voice lines
#define ROAM_OBSTACLE_KIND 0x3c      // character kind it keeps away from
#define ROAM_OBSTACLE_NEAR 75.0f
#define ROAM_OBSTACLE_PLACE 100.0f
#define ROAM_OBSTACLE_END_NEAR 0x5b  // slots scanned for obstacles while it is obstructed
#define ROAM_PLACE_ATTEMPTS 100
#define ROAM_STEP -5.0f
#define ROAM_PI 3.1415f
#define ROAM_DEGREES 180.0f
#define ROAM_SETUP_STEPS 2
#define ROAM_SETUP_DONE_MARK 9
#define ROAM_RAISE_UPPER 385.0f
#define ROAM_RAISE_LOWER (-490.0f)
#define ROAM_RAISE_MIDDLE 250.0f
#define ROAM_RAISE_OFFSET 15.0f
#define ROAM_PARTY_RANGE 100
#define ROAM_PARTY_NEAR 50
#define ROAM_STILL_DISTANCE 0.5f
#define ROAM_STILL_FRAMES 0x3c
#define ROAM_TURN_NUDGE 0.01f
#define ROAM_TURN_NUDGE_REST 0.05f
#define ROAM_ARRIVE_RADIUS 20.0f
#define ROAM_FADE_FRAMES 10
#define ROAM_WAIT_VOICE_A 0x5d
#define ROAM_WAIT_VOICE_B 0x92
#define ROAM_CROWD_ALERT 0xb
#define ROAM_NO_SHADOW_HEIGHT (-1000.0f)
#define ROAM_SHADOW_UNSET (-20000.0f)
#define ROAM_FAR_AWAY (-200000.0f)
#define ROAM_NEIGHBOURS 8
#define ROAM_MAP_HOUSE 2
#define ROAM_MAP_VILLAGE 3
#define ROAM_MAP_GATE_A 4
#define ROAM_MAP_GATE_B 5
#define ROAM_MAP_YARD 0xc

// Opens (closed = 0) the doors of the map: the nodes called "@020g1", "@gate", "@AtariBox1".. take the group 0.
static __forceinline void OpenGates()
{
    Node *node;
    switch (DAT_10c62ed0)
    {
    case ROAM_MAP_GATE_A:
    case ROAM_MAP_GATE_B:
        for (int i = 0;;)
        {
            node = DAT_10255f08->Find("@gate", i++);
            if (node == 0)
                break;
            if (strcmp(node->name, "gate") == 0)
            {
                node->group = 0;
                break;
            }
        }
        break;
    case ROAM_MAP_HOUSE:
        node = DAT_10255f08->Find("@020g1", 0);
        if (node != 0)
            node->group = 0;
        node = DAT_10255f08->Find("@020g2", 0);
        if (node != 0)
            node->group = 0;
        break;
    case ROAM_MAP_VILLAGE:
        node = DAT_10255f08->Find("@030g1", 0);
        if (node != 0)
            node->group = 0;
        node = DAT_10255f08->Find("@030g2", 0);
        if (node != 0)
            node->group = 0;
        break;
    case ROAM_MAP_YARD:
        node = DAT_10255f08->Find("@AtariBox1", 0);
        if (node != 0)
            node->group = 0;
        node = DAT_10255f08->Find("@AtariBox2", 0);
        if (node != 0)
            node->group = 0;
        node = DAT_10255f08->Find("@AtariBox3", 0);
        if (node != 0)
            node->group = 0;
        node = DAT_10255f08->Find("@AtariBox4", 0);
        if (node != 0)
            node->group = 0;
        node = DAT_10255f08->Find("@AtariBox5", 0);
        if (node != 0)
            node->group = 0;
        break;
    }
}

// Shuts the doors again: the same nodes take the groups 1, 2, .. that block the way.
static __forceinline void CloseGates()
{
    Node *node;
    switch (DAT_10c62ed0)
    {
    case ROAM_MAP_GATE_A:
    case ROAM_MAP_GATE_B:
        for (int i = 0;;)
        {
            node = DAT_10255f08->Find("@gate", i++);
            if (node == 0)
                break;
            if (strcmp(node->name, "gate") == 0)
            {
                node->group = 1;
                break;
            }
        }
        break;
    case ROAM_MAP_HOUSE:
        node = DAT_10255f08->Find("@020g1", 0);
        if (node != 0)
            node->group = 1;
        node = DAT_10255f08->Find("@020g2", 0);
        if (node != 0)
            node->group = 2;
        break;
    case ROAM_MAP_VILLAGE:
        node = DAT_10255f08->Find("@030g1", 0);
        if (node != 0)
            node->group = 1;
        node = DAT_10255f08->Find("@030g2", 0);
        if (node != 0)
            node->group = 2;
        break;
    case ROAM_MAP_YARD:
        node = DAT_10255f08->Find("@AtariBox1", 0);
        if (node != 0)
            node->group = 1;
        node = DAT_10255f08->Find("@AtariBox2", 0);
        if (node != 0)
            node->group = 2;
        node = DAT_10255f08->Find("@AtariBox3", 0);
        if (node != 0)
            node->group = 3;
        node = DAT_10255f08->Find("@AtariBox4", 0);
        if (node != 0)
            node->group = 4;
        node = DAT_10255f08->Find("@AtariBox5", 0);
        if (node != 0)
            node->group = 5;
        break;
    }
}

// 1 when `zone` is one of the three resting places in the way of the first roamer (slot 0xf): the roamer picks again.
#define IS_BLOCKED_ZONE(zone) ((zone) == 0x10 || (zone) == 0x15 || (zone) == 0x1a)

// Picks the next zone at random among the neighbours of the zone the roamer is in.
#define RANDOM_NEIGHBOUR(zone) ((int)DAT_101d6350[zone][FUN_1013f880() % ROAM_NEIGHBOURS])

// Walks the character forward: the step vector (5 units ahead of the heading) is turned by `heading`.
#define ROAM_MOTION(blocker) ((blocker) == 1 ? 2 : 0)

// 0x10049ec0 (Npc_RoamZones). Keeps the shadow on the ground, then, with the player talking to it, opens the doors of
// the map and turns to the player; otherwise it closes the doors again, sets itself up on its first two frames (the
// zones it starts in come from three script flags; it is put at a random free spot of the zone) and walks the zone
// graph (DAT_101d6350): state 0 heads for the zone it is in and then for the next one, state 1 rests in a resting
// zone and looks around, states 2, 3 and 4 take it through a door to the other map (fade out, fade in, appear at the
// door). Characters of kind 0x3c in the way make it wait; a party character obstructed it makes it stand still. Returns 0.
int Npc_RoamZones(int index)
{
    DAT_10db6ee8[index].shadowOpacity = 15;
    if (DAT_104bf8a4 % 15 == index % 15 && DAT_102022b8 != 0)
    {
        Vec4 from;
        Vec4 ground;
        Vec4 top = DAT_10db6ee8[index].matrix.GetTranslation();
        from.Copy(top);
        top.z += 10.0f;
        Vec4 direction = *DAT_10804bf8.LightDirection(&DAT_10db6ee8[index], &DAT_118e69e8, &DAT_118e6cb8);
        direction.Normalized3();
        float distance = 1000.0f;
        from.Copy(from - direction * distance);
        if (DAT_10255f08 != 0 && DAT_10255f08->CastSegment2(0, 0x4000000, &from, &top, &ground, 0) != 0)
        {
            ground.x = from.x;
            ground.y = from.y;
            DAT_10db6ee8[index].shadowMatrix.SetTranslation(ground);
        }
        else
        {
            ground.x = from.x;
            ground.y = from.y;
            ground.z = -3500.0f;
            DAT_10db6ee8[index].shadowMatrix.SetTranslation(ground);
        }
    }
    DAT_10db6ee8[index].m_0012 = 2;
    if (DAT_10c635c4 != 0)
        return 0;

    int zone = 0;
    int blocked = 0;
    Vec4 spot;
    Vec4 step;
    int crowded = 0;
    int flagBase = ROAM_FLAG_BASE_DEFAULT;
    Vec4 gateYard10;
    Vec4 gateYard1a;
    Vec4 gateYardC;
    Vec4 gateYard7;
    Vec4 gateVillageF;
    Vec4 gateHouse1c;
    Vec4 gateA;
    Vec4 gateB;
    Vec4 gateHouse12;
    Vec4 gateVillage17;
    Vec4 door;
    if (DAT_10db6ee8[index].IsClearAny(0) == 0)
        crowded = 1;
    ZonePoint from;
    from.x = DAT_10216dc0[DAT_101d6310[DAT_1020b430[index].currentZone]].x;
    from.y = DAT_10216dc0[DAT_101d6310[DAT_1020b430[index].currentZone]].y;
    from.radius = DAT_10216dc0[DAT_101d6310[DAT_1020b430[index].currentZone]].radius;
    ZonePoint to = DAT_10216dc0[DAT_101d6310[DAT_1020b430[index].nextZone]];

    if (DAT_10c6351c == index && DAT_1020b430[index].setupStep > 1)
    {
        if (DAT_1020b430[index].gatesOpen == 0)
        {
            if (DAT_10255f08 != 0)
                OpenGates();
            DAT_1020b430[index].gatesOpen = 1;
        }
        Vec4 toPlayer = DAT_10db6ee8[0].matrix.GetTranslation() - DAT_10db6ee8[index].matrix.GetTranslation();
        spot.Copy(toPlayer);
        if (DAT_10255160[index][0] < ROAM_NO_SHADOW_HEIGHT)
            DAT_10255160[index][0] = DAT_10db6ee8[index].targetHeading;
        DAT_10db6ee8[index].alpha = 1.0f;
        if (DAT_1020b430[index].talkWait > 0)
            DAT_1020b430[index].talkWait = -1;
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_10db6ee8[index].targetHeading = (float)atan2(spot.x, spot.y) + ROAM_PI;
        return 0;
    }
    if (DAT_10c6351c - 100 == index)
        return 0;
    if (DAT_10255160[index][0] > ROAM_NO_SHADOW_HEIGHT)
    {
        float turned = (float)fabs(DAT_10db6ee8[index].heading - DAT_10255160[index][0]);
        if (turned > 0.017f && DAT_1020b430[index].setupStep > 1)
        {
            DAT_10db6ee8[index].targetHeading = DAT_10255160[index][0];
            return 0;
        }
    }
    DAT_10255160[index][0] = ROAM_SHADOW_UNSET;
    if (DAT_1020b430[index].gatesOpen != 0)
    {
        if (DAT_10255f08 != 0)
            CloseGates();
        DAT_1020b430[index].gatesOpen = 0;
    }
    if (index == ROAM_SLOT_D)
        flagBase = ROAM_FLAG_BASE_SLOT_D;
    else if (index == ROAM_SLOT_F)
        flagBase = ROAM_FLAG_BASE_SLOT_F;
    else if (index == ROAM_SLOT_3A)
        flagBase = ROAM_FLAG_BASE_SLOT_3A;
    if (DAT_10c62ed0 == ROAM_MAP_YARD)
    {
        gateYard10.Set(-54.0f, 704.0f, 18.0f, 0.0f);
        gateYard1a.Set(-239.0f, 338.0f, 391.0f, 0.0f);
        gateYardC.Set(1570.0f, 1530.0f, -457.0f, 0.0f);
        gateYard7.Set(2256.0f, 1540.0f, -412.0f, 0.0f);
        zone = 0;
    }
    else if (DAT_10c62ed0 == ROAM_MAP_VILLAGE)
    {
        gateVillageF.Set(87.0f, 506.0f, -7.0f, 0.0f);
        gateVillage17.Set(-271.0f, -278.0f, 133.0f, 0.0f);
    }
    else if (DAT_10c62ed0 == ROAM_MAP_HOUSE)
    {
        gateHouse1c.Set(-460.0f, 8.0f, -8.0f, 0.0f);
        gateHouse12.Set(56.0f, -296.0f, -78.0f, 0.0f);
    }
    else if (DAT_10c62ed0 == ROAM_MAP_GATE_A)
        gateA.Set(3.0f, -425.0f, 0.0f, 0.0f);
    else if (DAT_10c62ed0 == ROAM_MAP_GATE_B)
        gateB.Set(-132.0f, -367.0f, 0.0f, 0.0f);
    if (DAT_10874848.TestScriptFlag(flagBase) < 0)
    {
        DAT_10db6ee8[index].SetPosition(0, 0, ROAM_FAR_AWAY);
        return 0;
    }
    if (DAT_1032d69c == 0 && DAT_10c6351c != index)
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
    if (DAT_10c6351c > 0)
    {
        if (DAT_10c6351c != index)
            return 0;
    }
    if (DAT_10c6351c <= 0 && DAT_1032d69c == 0 && DAT_1020b430[index].setupStep >= 2)
        return 0;

    int blocker = 0;
    Vec4 position = DAT_10db6ee8[index].matrix.GetTranslation();
    for (int i = 1; i < 0x5c; i++)
    {
        if (i != index && DAT_10daaae8[i].kind == ROAM_OBSTACLE_KIND)
        {
            Vec4 other = DAT_10db6ee8[i].matrix.GetTranslation();
            if ((other - position).Length() < ROAM_OBSTACLE_NEAR)
            {
                if (i < index)
                {
                    DAT_10db6ee8[index].move.Fill(0.0f);
                    blocker = 1;
                }
                else
                    blocker = 2;
            }
        }
    }
    if (DAT_1020b430[index].headingHold != 0)
    {
        DAT_1020b430[index].headingHold--;
        DAT_10db6ee8[index].targetHeading = DAT_10255160[index][1];
    }

    if (DAT_1020b430[index].setupStep < ROAM_SETUP_STEPS)
    {
        DAT_10db6ee8[index].root->M_10194630(index, 0x400);
        DAT_10db6ee8[index].root->M_101946b0(0xfe, 0x400);
        DAT_10db6ee8[index].root->M_10194730(0x2f, 0x400);
        DAT_10db6ee8[index].root->M_10194730(0x2f, 0x200);
        DAT_10db6ee8[index].m_2390 = 2;
        DAT_1020b430[index].setupStep++;
        DAT_10db6ee8[index].m_022c.Fill(0.0f);
        DAT_1020b430[index].nextZone = DAT_10874848.TestScriptFlag(flagBase);
        DAT_1020b430[index].currentZone = DAT_10874848.TestScriptFlag(flagBase + 1);
        DAT_1020b430[index].previousZone = DAT_10874848.TestScriptFlag(flagBase + 2);
        DAT_1020b430[index].setupMap = DAT_10c62ed0;
        DAT_10255160[index][0] = ROAM_SHADOW_UNSET;
        if (DAT_1020b430[index].nextZone == 0)
        {
            if (index == ROAM_SLOT_D)
            {
                DAT_1020b430[ROAM_SLOT_D].nextZone = 0xf;
                DAT_1020b430[ROAM_SLOT_D].currentZone = 0xf;
            }
            else if (index == ROAM_SLOT_F)
            {
                DAT_1020b430[ROAM_SLOT_F].nextZone = 0xb;
                DAT_1020b430[ROAM_SLOT_F].currentZone = 0xb;
            }
            else if (index == ROAM_SLOT_3A)
            {
                DAT_1020b430[ROAM_SLOT_3A].nextZone = 6;
                DAT_1020b430[ROAM_SLOT_3A].currentZone = 6;
            }
            DAT_1020b430[index].state = ROAM_STATE_REST;
        }
        zone = DAT_1020b430[index].currentZone;
        if (zone == 0)
            zone = DAT_1020b430[index].nextZone;
        if (zone != 0)
        {
            if (DAT_1020b430[index].setupMap != DAT_101d6330[zone])
            {
                DAT_10db6ee8[index].SetPosition(DAT_10db6ee8[index].matrix._41, DAT_10db6ee8[index].matrix._42, -1500.0f);
                DAT_1020b430[index].crowdFrames = ROAM_SETUP_DONE_MARK;
                return 0;
            }
            float raise = 0.0f;
            if (zone == 0x16 || zone == 0x1d || zone == 0x1c || zone == 0x1b || zone == 0x19)
                raise = ROAM_RAISE_UPPER;
            else if (zone == 0xb || zone < 7)
                raise = ROAM_RAISE_LOWER;
            else if (zone == 0x13)
                raise = ROAM_RAISE_MIDDLE;
            raise += ROAM_RAISE_OFFSET;
            int attempts = 0;
            int obstructed;
            for (;;)
            {
                obstructed = 0;
                spot.x = 0.0f;
                spot.y = (float)(FUN_1013f880() % DAT_10216dc0[DAT_101d6310[zone]].radius);
                spot.z = raise;
                spot.RotateZ((float)(FUN_1013f880() % 360) / ROAM_DEGREES * ROAM_PI);
                DAT_10db6ee8[index].SetPosition((float)DAT_10216dc0[DAT_101d6310[zone]].x + spot.x,
                                                (float)DAT_10216dc0[DAT_101d6310[zone]].y + spot.y, raise);
                DAT_10db6ee8[index].targetHeading = (float)atan2(-spot.x, -spot.y) + ROAM_PI;
                DAT_10db6ee8[index].heading = DAT_10db6ee8[index].targetHeading;
                blocked = FUN_100242c0(index, 0, spot.Length(), 0, 1, 0);
                attempts++;
                for (int i = 1; i < 0x39; i++)
                {
                    if (DAT_10daaae8[i].kind == ROAM_OBSTACLE_KIND && i != index &&
                        (DAT_10db6ee8[i].matrix.GetTranslation() - DAT_10db6ee8[index].matrix.GetTranslation()).Length() <
                            ROAM_OBSTACLE_PLACE)
                        obstructed = 1;
                }
                if (attempts == ROAM_PLACE_ATTEMPTS)
                    break;
                if (obstructed == 0 && DAT_10db6ee8[index].IsClearOfMapLimited(0) != 0)
                {
                    Vec4 start;
                    Vec4 end;
                    Vec4 hit;
                    start = DAT_10db6ee8[index].matrix.GetTranslation();
                    end.Copy(start);
                    end.z -= 10000.0f;
                    if (DAT_10255f08 != 0 && DAT_10255f08->Collide(0, 0x4000000, &end, &start, &hit) != 0 && blocked == 0)
                    {
                        DAT_1020b430[index].crowdFrames = ROAM_SETUP_DONE_MARK;
                        return 0;
                    }
                }
            }
            DAT_10db6ee8[index].SetPosition((float)DAT_10216dc0[DAT_101d6310[zone]].x,
                                            (float)DAT_10216dc0[DAT_101d6310[zone]].y, raise);
            spot.x = 0.0f;
            spot.y = 75.0f;
            spot.z = raise;
            attempts = 0;
            for (;;)
            {
                obstructed = 0;
                spot.RotateZ(0.52359f);
                DAT_10db6ee8[index].SetPosition((float)DAT_10216dc0[DAT_101d6310[zone]].x + spot.x,
                                                (float)DAT_10216dc0[DAT_101d6310[zone]].y + spot.y, spot.z);
                DAT_10db6ee8[index].targetHeading = (float)atan2(-spot.x, -spot.y) + ROAM_PI;
                DAT_10db6ee8[index].heading = DAT_10db6ee8[index].targetHeading;
                blocked = FUN_100242c0(index, 0, spot.Length(), 0, 0, 0);
                attempts++;
                if (attempts % 12 == 0)
                {
                    spot.z = 0.0f;
                    spot.Normalized3();
                    spot.Copy(spot * 75.0f * (float)(attempts / 12 + 1));
                    spot.z = raise;
                }
                if (attempts == ROAM_PLACE_ATTEMPTS)
                    break;
                for (int i = 1; i < 0x39; i++)
                {
                    if (DAT_10daaae8[i].kind == ROAM_OBSTACLE_KIND && i != index &&
                        (DAT_10db6ee8[i].matrix.GetTranslation() - DAT_10db6ee8[index].matrix.GetTranslation()).Length() <
                            ROAM_OBSTACLE_PLACE)
                        obstructed = 1;
                }
                if (obstructed == 0 && DAT_10db6ee8[index].IsClearOfMapLimited(0) != 0)
                {
                    Vec4 start;
                    Vec4 end;
                    Vec4 hit;
                    start = DAT_10db6ee8[index].matrix.GetTranslation();
                    end.Copy(start);
                    end.z -= 10000.0f;
                    if (DAT_10255f08 != 0 && DAT_10255f08->Collide(0, 0x4000000, &end, &start, &hit) != 0 && blocked == 0)
                    {
                        DAT_1020b430[index].crowdFrames = ROAM_SETUP_DONE_MARK;
                        return 0;
                    }
                }
            }
            DAT_10db6ee8[index].SetPosition((float)DAT_10216dc0[DAT_101d6310[zone]].x,
                                            (float)DAT_10216dc0[DAT_101d6310[zone]].y, raise);
        }
        DAT_1020b430[index].crowdFrames = ROAM_SETUP_DONE_MARK;
        return 0;
    }
    if (DAT_104bfdf0 != 0 || DAT_104c0135 != 0 || DAT_104bfe1c != 0)
        return 0;

    if (DAT_1020b430[index].nextZone == 0)
        DAT_1020b430[index].nextZone = DAT_1020b430[index].currentZone;
    int next = DAT_1020b430[index].nextZone;
    DAT_10874848.SetScriptFlag(flagBase, next);
    int here = DAT_1020b430[index].currentZone;
    DAT_10874848.SetScriptFlag(flagBase + 1, here);
    if (here != next)
    {
        DAT_1020b430[index].previousZone = 0;
        DAT_10874848.SetScriptFlag(flagBase + 2, 0);
    }
    int map = DAT_1020b430[index].setupMap;
    if (DAT_101d6330[here] != map &&
        (DAT_1020b430[index].previousZone == 0 || (next == here && DAT_1020b430[index].walkMode != 2)) &&
        DAT_1020b430[index].state != ROAM_STATE_DOOR)
    {
        if ((map == ROAM_MAP_YARD && (next == 0xf || next == 0x1c || next == 0xb || next == 6)) ||
            (map == ROAM_MAP_HOUSE && (next == 0x1a || next == 0x17)) ||
            (map == ROAM_MAP_VILLAGE && (next == 0x12 || next == 0x10)) || (map == ROAM_MAP_GATE_A && next == 0xc) ||
            (map == ROAM_MAP_GATE_B && next == 7))
        {
            DAT_1020b430[index].walkMode = 0;
            DAT_1020b430[index].zoneStepFrames = 0;
            DAT_1020b430[index].state = ROAM_STATE_DOOR;
            return 0;
        }
        if (++DAT_1020b430[index].zoneStepFrames < 0x1f)
            return 0;
        if (FUN_1013f880() % 10000 > 0x4f)
            return 0;
        int oldZone = DAT_1020b430[index].currentZone;
        int tries = 0;
        DAT_1020b430[index].walkMode = 0;
        DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
        DAT_1020b430[index].nextZone = 0;
        for (;;)
        {
            int pick = DAT_1020b430[index].nextZone;
            if (pick != 0 && (oldZone != pick || tries > 0x31) &&
                (DAT_1020b430[index].previousZone != pick || tries > 0x31))
            {
                if (index != ROAM_SLOT_F)
                    break;
                if (!IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                {
                    if (DAT_1020b430[ROAM_SLOT_F].nextZone == 0x10 || DAT_1020b430[ROAM_SLOT_F].nextZone == 0x15)
                    {
                        DAT_1020b430[ROAM_SLOT_F].nextZone = DAT_1020b430[ROAM_SLOT_F].currentZone;
                        DAT_1020b430[ROAM_SLOT_F].currentZone = oldZone;
                    }
                    break;
                }
            }
            DAT_1020b430[index].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[index].currentZone);
            tries++;
        }
        DAT_1020b430[index].zoneStepFrames = 0;
        return 0;
    }
    if (DAT_1020b430[index].talkWait > 0)
    {
        DAT_1020b430[index].talkWait--;
        if (blocker == 2)
            DAT_1020b430[index].talkWait = 0;
        if (DAT_1020b430[index].talkWait == 0)
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        return 0;
    }
    if (DAT_1020b430[index].talkWait == -1 && index != ROAM_SLOT_3A)
    {
        if (FUN_1013f880() % 2 == 0)
        {
            DAT_10db6ee8[index].PlayMotion(6, 0, 5, 1.0f);
            DAT_1020b430[index].talkWait = ROAM_WAIT_VOICE_A;
            return 0;
        }
        DAT_10db6ee8[index].PlayMotion(5, 0, 5, 1.0f);
        DAT_1020b430[index].talkWait = ROAM_WAIT_VOICE_B;
        return 0;
    }
    if (DAT_1020b430[index].talkWait == -1 && index == ROAM_SLOT_3A)
        DAT_1020b430[ROAM_SLOT_3A].talkWait = 0;

    int rerolls = 0;
    int state = DAT_1020b430[index].state;
    if (state == ROAM_STATE_WALK)
    {
        if (FUN_1013f880() % 10000 < 0x1e && index != ROAM_SLOT_QUIET && index != ROAM_SLOT_3A)
            DAT_1020b430[index].talkWait = -1;
        if (DAT_1020b430[index].walkMode == 0)
        {
            spot.x = (float)from.x;
            spot.y = (float)from.y;
            DAT_10db6ee8[index].targetHeading =
                FUN_10001f80(spot.x - DAT_10db6ee8[index].matrix._41, spot.y - DAT_10db6ee8[index].matrix._42) + ROAM_PI;
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].targetHeading);
            crowded = DAT_10db6ee8[index].IsClearAny(&step) == 0;
            DAT_1020b430[index].blockedLast = 0;
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            float distance = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
            if (distance < (float)from.radius * 0.5f)
                DAT_1020b430[index].walkMode = 1;
        }
        else if (DAT_1020b430[index].walkMode == 1)
        {
            spot.x = (float)to.x;
            spot.y = (float)to.y;
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            if (DAT_1020b430[index].blockedLast == 0)
                DAT_10db6ee8[index].targetHeading =
                    FUN_10001f80(spot.x - DAT_10db6ee8[index].matrix._41, spot.y - DAT_10db6ee8[index].matrix._42) + ROAM_PI;
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            int walkable = DAT_10db6ee8[index].IsClearAny(&step);
            crowded = walkable == 0;
            if (walkable != 0 && (DAT_10db6ee8[index].IsClearOfMapLimited(&step) != 0 || DAT_1020b430[index].blockedLast != 0) &&
                FUN_10034cc0(index, step, ROAM_PARTY_RANGE) == 0)
                DAT_1020b430[index].blockedLast = 0;
            else
            {
                float distance = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
                if ((float)to.radius < distance)
                {
                    DAT_10db6ee8[index].targetHeading += ROAM_TURN_NUDGE;
                    step.Fill(0.0f);
                    step.y = ROAM_STEP;
                    step.RotateZ(-DAT_10db6ee8[index].heading);
                }
                DAT_1020b430[index].blockedLast = 1;
            }
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            if ((DAT_10802bf8[index].position - DAT_10db6ee8[index].matrix.GetTranslation()).Length2D() < ROAM_STILL_DISTANCE)
            {
                if (++DAT_1020b430[index].nearFrames > ROAM_STILL_FRAMES)
                {
                    DAT_1020b430[index].walkMode = 0;
                    DAT_1020b430[index].nearFrames = 0;
                }
            }
            else if (DAT_1020b430[index].nearFrames > 0)
                DAT_1020b430[index].nearFrames--;
            int zoneMap = DAT_1020b430[index].setupMap;
            int goal = DAT_1020b430[index].nextZone;
            int zoneHere = DAT_1020b430[index].currentZone;
            if ((zoneMap == ROAM_MAP_YARD && DAT_101d6330[zoneHere] == ROAM_MAP_YARD &&
                 (goal == 0x10 || goal == 0x1a || goal == 0xc || goal == 7)) ||
                (zoneMap == ROAM_MAP_HOUSE && DAT_101d6330[zoneHere] == ROAM_MAP_HOUSE && (goal == 0x1c || goal == 0x12)) ||
                (zoneMap == ROAM_MAP_VILLAGE && DAT_101d6330[zoneHere] == ROAM_MAP_VILLAGE && (goal == 0x17 || goal == 0xf)) ||
                (zoneMap == ROAM_MAP_GATE_A && DAT_101d6330[zoneHere] == ROAM_MAP_GATE_A && goal == 0xb) ||
                (zoneMap == ROAM_MAP_GATE_B && DAT_101d6330[zoneHere] == ROAM_MAP_GATE_B && goal == 6))
            {
                DAT_1020b430[index].walkMode = 0;
                DAT_1020b430[index].state = ROAM_STATE_LEAVE;
                return 0;
            }
            if ((DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D() < (float)to.radius)
            {
                if (to.kind != 1 && to.kind != 3)
                {
                    int old = DAT_1020b430[index].currentZone;
                    DAT_1020b430[index].walkMode = 0;
                    DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
                    DAT_1020b430[index].nextZone = 0;
                    for (;;)
                    {
                        int pick = DAT_1020b430[index].nextZone;
                        if (pick != 0 && old != pick && DAT_1020b430[index].previousZone != pick)
                        {
                            if (index != ROAM_SLOT_F)
                                return 0;
                            if (!IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                                return 0;
                        }
                        DAT_1020b430[index].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[index].currentZone);
                        if (++rerolls > 10)
                        {
                            DAT_1020b430[index].nextZone = old;
                            return 0;
                        }
                    }
                }
                int roll = FUN_1013f880() % ROAM_NEIGHBOURS;
                int candidate = DAT_101d6350[DAT_1020b430[index].nextZone][roll];
                if (DAT_1020b430[index].currentZone != candidate && candidate != 0 && FUN_1013f880() % 1000 > 99 &&
                    DAT_1020b430[index].previousZone != candidate)
                {
                    DAT_1020b430[index].walkMode = 0;
                    DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
                    DAT_1020b430[index].nextZone = candidate;
                    for (;;)
                    {
                        if (index != ROAM_SLOT_F)
                            return 0;
                        if (!IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                            break;
                        DAT_1020b430[ROAM_SLOT_F].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[ROAM_SLOT_F].currentZone);
                        if (++rerolls >= 0x32)
                            break;
                    }
                    if (IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                        DAT_1020b430[ROAM_SLOT_F].nextZone = DAT_1020b430[ROAM_SLOT_F].currentZone;
                    return 0;
                }
                DAT_1020b430[index].walkMode = 0;
                DAT_1020b430[index].state = to.kind;
                return 0;
            }
        }
    }
    else if (state == ROAM_STATE_REST)
    {
        spot.x = (float)to.x;
        spot.y = (float)to.y;
        spot.z = 0.0f;
        float distance = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
        if (!(distance > (float)to.radius) || DAT_1020b430[index].walkMode == -1)
        {
            if (DAT_1020b430[index].walkMode == 0)
            {
                DAT_1020b430[index].zoneStepFrames = 0;
                DAT_1020b430[index].walkMode = FUN_1013f880() % 3 + 1;
                if (DAT_1020b430[index].walkMode == 1)
                    DAT_10db6ee8[index].targetHeading = (float)(FUN_1013f880() % 360) / ROAM_DEGREES * ROAM_PI;
            }
            if (++DAT_1020b430[index].zoneStepFrames > 0x1e && FUN_1013f880() % 1000 < 5 &&
                (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D() < (float)to.radius)
            {
                DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
                DAT_1020b430[index].nextZone = 0;
                while (DAT_1020b430[index].nextZone == 0 || DAT_1020b430[index].previousZone == DAT_1020b430[index].nextZone ||
                       (index == ROAM_SLOT_F && IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone)))
                    DAT_1020b430[index].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[index].currentZone);
                DAT_1020b430[index].zoneStepFrames = 0;
                DAT_1020b430[index].state = ROAM_STATE_WALK;
                DAT_1020b430[index].walkMode = 0;
                return 0;
            }
            if (DAT_1020b430[index].walkMode == 1)
            {
                DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
                step.y = ROAM_STEP;
                step.RotateZ(-DAT_10db6ee8[index].heading);
                int walkable = DAT_10db6ee8[index].IsClearAny(&step);
                crowded = walkable == 0;
                if (DAT_1020b430[index].zoneStepFrames > 0x5a || crowded ||
                    (DAT_10db6ee8[index].IsClearOfMapLimited(&step) == 0 && DAT_1020b430[index].blockedLast == 0) ||
                    FUN_10034cc0(index, step, ROAM_PARTY_RANGE) != 0)
                    DAT_1020b430[index].walkMode = 0;
                if (DAT_104bfd08 == 0 && blocker != 1)
                    DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            }
            else if (DAT_1020b430[index].walkMode > 1)
            {
                DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
                if (DAT_1020b430[index].zoneStepFrames > 0x1e && FUN_1013f880() % 500 < 10)
                    DAT_1020b430[index].walkMode = FUN_1013f880() % 3;
                if (FUN_1013f880() % 10000 < 0x32)
                    DAT_1020b430[index].talkWait = -1;
                if (blocker == 2)
                {
                    DAT_1020b430[index].walkMode = 1;
                    DAT_1020b430[index].talkWait = 0;
                }
            }
        }
        else
        {
            DAT_1020b430[index].walkMode = -1;
            DAT_10db6ee8[index].targetHeading =
                FUN_10001f80(spot.x - DAT_10db6ee8[index].matrix._41, spot.y - DAT_10db6ee8[index].matrix._42) + ROAM_PI;
        }
        if (DAT_1020b430[index].walkMode == -1)
        {
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            crowded = DAT_10db6ee8[index].IsClearAny(&step) == 0;
            if (DAT_1020b430[index].zoneStepFrames < 0x5b && !crowded &&
                (DAT_10db6ee8[index].IsClearOfMapLimited(&step) != 0 || DAT_1020b430[index].blockedLast != 0) &&
                FUN_10034cc0(index, step, ROAM_PARTY_RANGE) == 0)
                DAT_1020b430[index].blockedLast = 0;
            else
            {
                DAT_1020b430[index].walkMode = 0;
                float away = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
                if ((float)to.radius < away)
                {
                    DAT_10db6ee8[index].targetHeading += ROAM_TURN_NUDGE_REST;
                    step.Fill(0.0f);
                    step.y = ROAM_STEP;
                    step.RotateZ(-DAT_10db6ee8[index].heading);
                }
                DAT_1020b430[index].blockedLast = 1;
            }
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
        }
    }
    else if (state == ROAM_STATE_LEAVE)
    {
        if (map == ROAM_MAP_YARD)
        {
            switch (next)
            {
            case 0x10:
                door.Copy(gateYard10);
                break;
            case 0x1a:
                door.Copy(gateYard1a);
                break;
            case 0xc:
                door.Copy(gateYardC);
                break;
            case 7:
                door.Copy(gateYard7);
                break;
            }
        }
        else if (map == ROAM_MAP_HOUSE)
        {
            if (next == 0x12)
                door.Copy(gateHouse12);
            else if (next == 0x1c)
                door.Copy(gateHouse1c);
        }
        else if (map == ROAM_MAP_VILLAGE)
        {
            if (next == 0xf)
                door.Copy(gateVillageF);
            else if (next == 0x17)
                door.Copy(gateVillage17);
        }
        else if (map == ROAM_MAP_GATE_A)
            door.Copy(gateA);
        else if (map == ROAM_MAP_GATE_B)
            door.Copy(gateB);
        int mode = DAT_1020b430[index].walkMode;
        if (mode == 0)
        {
            spot.x = (float)from.x;
            spot.y = (float)from.y;
            DAT_10db6ee8[index].targetHeading =
                FUN_10001f80(spot.x - DAT_10db6ee8[index].matrix._41, spot.y - DAT_10db6ee8[index].matrix._42) + ROAM_PI;
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].targetHeading);
            crowded = DAT_10db6ee8[index].IsClearAny(&step) == 0;
            DAT_1020b430[index].blockedLast = 0;
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            float distance = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
            if (distance < (float)from.radius * 0.5f)
                DAT_1020b430[index].walkMode = 1;
        }
        else if (mode == 1)
        {
            spot.x = door.x;
            spot.y = door.y;
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            if (DAT_1020b430[index].blockedLast == 0)
                DAT_10db6ee8[index].targetHeading =
                    FUN_10001f80(spot.x - DAT_10db6ee8[index].matrix._41, spot.y - DAT_10db6ee8[index].matrix._42) + ROAM_PI;
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            crowded = DAT_10db6ee8[index].IsClearAny(&step) == 0;
            DAT_1020b430[index].blockedLast = 0;
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            if ((DAT_10802bf8[index].position - DAT_10db6ee8[index].matrix.GetTranslation()).Length2D() < ROAM_STILL_DISTANCE)
            {
                if (++DAT_1020b430[index].nearFrames > ROAM_STILL_FRAMES)
                {
                    DAT_1020b430[index].walkMode = 0;
                    DAT_1020b430[index].nearFrames = 0;
                }
            }
            else if (DAT_1020b430[index].nearFrames > 0)
                DAT_1020b430[index].nearFrames--;
            if ((DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D() < ROAM_ARRIVE_RADIUS)
            {
                DAT_1020b430[index].zoneStepFrames = 0;
                DAT_1020b430[index].walkMode = 2;
            }
        }
        else if (mode == 2)
        {
            if (DAT_104bf888 >= 10000 && DAT_104bf888 % 10000 == index)
                DAT_104bf888 = 0;
            int frames = DAT_1020b430[index].zoneStepFrames;
            if (frames == 0)
            {
                int leaving = DAT_1020b430[index].currentZone;
                DAT_10874848.SetScriptFlag(flagBase + 2, leaving);
                DAT_1020b430[index].previousZone = leaving;
                DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
            }
            DAT_1020b430[index].zoneStepFrames = ++frames;
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            DAT_10db6ee8[index].alpha = (float)(ROAM_FADE_FRAMES - DAT_1020b430[index].zoneStepFrames) / 10.0f;
            if (DAT_1020b430[index].zoneStepFrames == ROAM_FADE_FRAMES)
            {
                DAT_1020b430[index].zoneStepFrames = 0;
                DAT_1020b430[index].walkMode = 0;
                DAT_1020b430[index].state = ROAM_STATE_ARRIVE;
                DAT_10db6ee8[index].SetPosition(-20000.0f, DAT_10db6ee8[index].matrix._42, -1500.0f);
            }
        }
    }
    else if (state == ROAM_STATE_ARRIVE)
    {
        if (DAT_1020b430[index].walkMode == 0)
        {
            DAT_1020b430[index].walkMode = 1;
            DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
            DAT_10db6ee8[index].targetHeading =
                (float)(FUN_1013f880() % 360 - 180) / ROAM_DEGREES * ROAM_PI + DAT_10db6ee8[index].targetHeading;
        }
        else if (DAT_1020b430[index].walkMode != 1)
        {
            if (FUN_1013f880() % 10000 < 0x5a)
                DAT_1020b430[index].talkWait = -1;
        }
        else
        {
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            crowded = DAT_10db6ee8[index].IsClearAny(&step) == 0;
            DAT_1020b430[index].blockedLast = 0;
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            spot.Fill(0.0f);
            spot.x = (float)to.x;
            spot.y = (float)to.y;
            float distance = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
            if ((float)to.radius < distance || DAT_10db6ee8[index].IsClearOfMapLimited(&step) == 0 || crowded)
            {
                if (blocker == 2)
                    DAT_1020b430[index].walkMode = 1;
                else
                {
                    DAT_1020b430[index].walkMode = 2;
                    DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
                }
            }
        }
        int sameGoal = 0;
        for (int i = 0; i < 5; i++)
        {
            if (DAT_101d67f0[i] != index &&
                DAT_1020b430[DAT_101d67f0[i]].nextZone == DAT_1020b430[index].currentZone)
            {
                sameGoal = 1;
                break;
            }
        }
        if (FUN_1013f880() % 10000 < 10 || sameGoal)
        {
            DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
            DAT_1020b430[index].nextZone = 0;
            while (DAT_1020b430[index].nextZone == 0 || DAT_1020b430[index].previousZone == DAT_1020b430[index].nextZone ||
                   (index == ROAM_SLOT_F && IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone)))
                DAT_1020b430[index].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[index].currentZone);
            DAT_1020b430[index].zoneStepFrames = 0;
            DAT_1020b430[index].state = ROAM_STATE_WALK;
            DAT_1020b430[index].walkMode = 0;
            return 0;
        }
    }
    else if (state == ROAM_STATE_DOOR)
    {
        int counterpart = zone;
        if (map == ROAM_MAP_YARD)
        {
            switch (next)
            {
            case 0xf:
                door.Copy(gateYard10);
                counterpart = 0x10;
                break;
            case 0x1c:
                door.Copy(gateYard1a);
                counterpart = 0x1a;
                break;
            case 0xb:
                door.Copy(gateYardC);
                counterpart = 0xc;
                break;
            case 6:
                door.Copy(gateYard7);
                counterpart = 7;
                break;
            }
        }
        else if (map == ROAM_MAP_HOUSE)
        {
            if (next == 0x17)
            {
                door.Copy(gateHouse12);
                counterpart = 0x12;
            }
            else if (next == 0x1a)
            {
                door.Copy(gateHouse1c);
                counterpart = 0x1c;
            }
        }
        else if (map == ROAM_MAP_VILLAGE)
        {
            if (next == 0x10)
            {
                door.Copy(gateVillageF);
                counterpart = 0xf;
            }
            else if (next == 0x12)
            {
                door.Copy(gateVillage17);
                counterpart = 0x17;
            }
        }
        else if (map == ROAM_MAP_GATE_A)
        {
            door.Copy(gateA);
            counterpart = 0xb;
        }
        else if (map == ROAM_MAP_GATE_B)
        {
            door.Copy(gateB);
            counterpart = 6;
        }
        int mode = DAT_1020b430[index].walkMode;
        if (mode == 0)
        {
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
            DAT_10db6ee8[index].SetPosition(door.x, door.y, door.z + 5.0f);
            DAT_10db6ee8[index].alpha = 0.0f;
            DAT_10db6ee8[index].shadowHidden = 1;
            DAT_1020b430[index].walkMode = -1;
            DAT_1020b430[index].zoneStepFrames = 0;
            float heading =
                FUN_10001f80((float)to.x - DAT_10db6ee8[index].matrix._41, (float)to.y - DAT_10db6ee8[index].matrix._42) +
                ROAM_PI;
            DAT_1020b430[index].previousZone = counterpart;
            DAT_10db6ee8[index].targetHeading = heading;
            DAT_10db6ee8[index].heading = heading;
            DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
        }
        else if (mode == -1)
        {
            DAT_10db6ee8[index].shadowHidden = 0;
            DAT_10db6ee8[index].alpha = 0.0f;
            if (++DAT_1020b430[index].zoneStepFrames == 0xf)
            {
                DAT_1020b430[index].walkMode = 1;
                DAT_1020b430[index].zoneStepFrames = 0;
                DAT_10db6ee8[index].shadowHidden = 0;
            }
        }
        else if (mode == 1)
        {
            DAT_10db6ee8[index].alpha = (float)(++DAT_1020b430[index].zoneStepFrames) * 0.1f;
            if (DAT_1020b430[index].zoneStepFrames == 10)
            {
                DAT_1020b430[index].walkMode = 2;
                DAT_1020b430[index].zoneStepFrames = 0;
            }
        }
        else if (mode == 2)
        {
            spot.x = (float)to.x;
            spot.y = (float)to.y;
            DAT_10db6ee8[index].PlayMotion(ROAM_MOTION(blocker), 0, 5, 1.0f);
            DAT_10db6ee8[index].targetHeading =
                FUN_10001f80(spot.x - DAT_10db6ee8[index].matrix._41, spot.y - DAT_10db6ee8[index].matrix._42) + ROAM_PI;
            step.y = ROAM_STEP;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            crowded = DAT_10db6ee8[index].IsClearAny(&step) == 0;
            if (DAT_104bfd08 == 0 && blocker != 1)
                DAT_10db6ee8[index].Push(step.x, step.y, step.z);
            float distance = (DAT_10db6ee8[index].matrix.GetTranslation() - spot).Length2D();
            if (distance < (float)to.radius)
            {
                if (to.kind != 1 && to.kind != 3)
                {
                    int old = DAT_1020b430[index].currentZone;
                    DAT_1020b430[index].walkMode = 0;
                    DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
                    DAT_1020b430[index].nextZone = 0;
                    DAT_1020b430[index].state = ROAM_STATE_WALK;
                    for (;;)
                    {
                        int pick = DAT_1020b430[index].nextZone;
                        if (pick != 0 && old != pick && DAT_1020b430[index].previousZone != pick)
                        {
                            if (index != ROAM_SLOT_F)
                                return 0;
                            if (!IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                                return 0;
                        }
                        DAT_1020b430[index].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[index].currentZone);
                    }
                }
                int roll = FUN_1013f880() % ROAM_NEIGHBOURS;
                int candidate = DAT_101d6350[DAT_1020b430[index].nextZone][roll];
                if (DAT_1020b430[index].currentZone != candidate && candidate != 0 && FUN_1013f880() % 1000 > 99 &&
                    DAT_1020b430[index].previousZone != candidate)
                {
                    DAT_1020b430[index].walkMode = 0;
                    DAT_1020b430[index].currentZone = DAT_1020b430[index].nextZone;
                    DAT_1020b430[index].nextZone = candidate;
                    for (;;)
                    {
                        if (index != ROAM_SLOT_F)
                            break;
                        if (!IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                            break;
                        DAT_1020b430[ROAM_SLOT_F].nextZone = RANDOM_NEIGHBOUR(DAT_1020b430[ROAM_SLOT_F].currentZone);
                        if (++rerolls >= 0x32)
                            break;
                    }
                    if (IS_BLOCKED_ZONE(DAT_1020b430[ROAM_SLOT_F].nextZone))
                        DAT_1020b430[ROAM_SLOT_F].nextZone = DAT_1020b430[ROAM_SLOT_F].currentZone;
                    DAT_1020b430[index].state = ROAM_STATE_WALK;
                    return 0;
                }
                DAT_1020b430[index].walkMode = 0;
                DAT_1020b430[index].state = to.kind;
                DAT_10874848.SetScriptFlag(flagBase + 2, DAT_1020b430[index].currentZone);
                return 0;
            }
        }
    }

    if (crowded == 0)
    {
        int partyNear = FUN_10034e80(index, 0, ROAM_PARTY_NEAR);
        if (DAT_1020b430[index].crowdFrames > 10)
        {
            if (partyNear != 0)
            {
                DAT_10802bf8[index].position.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
                return 0;
            }
            DAT_10db6ee8[index].root->M_10194730(0x2f, 0x400);
            DAT_10db6ee8[index].root->M_10194730(0x2f, 0x200);
        }
        if (partyNear == 0)
            DAT_1020b430[index].crowdFrames = 0;
    }
    else if (++DAT_1020b430[index].crowdFrames == ROAM_CROWD_ALERT)
    {
        DAT_10db6ee8[index].root->M_10194730(0x2d, 0x400);
        DAT_10db6ee8[index].root->M_10194730(0x2d, 0x200);
    }
    DAT_10802bf8[index].position.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    return 0;
}
