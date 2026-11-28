// The BS_004 boss: the moss root creature (kind 0x46). Once the game is over (DAT_10210d30) it runs the ending scene
// (the camera slides along the creature while the screen fades), otherwise the fight: the creature hovers over a path
// of nodes, picks attack states by random odds that depend on how many of its three minions (the slots after it) are
// down, dives at the player, sweeps its tail, breathes fire and calls the minions back. The moves are written as the
// original blends them: the movement vector is smoothed towards a per-state step vector every frame.

#include <math.h>
#include <windows.h>
#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "char_slot.h"
#include "char_state.h"
#include "effect_manager.h"
#include "texture_manager.h"
#include "bs004.h"
#include "bgm_track.h"
#include "char_equip_models.h"
#include "doppler_loops.h"
#include "dodge_golem.h"
#include "enemy_chase_hop.h"
#include "enemy_fighter.h"
#include "engine_memory.h"
#include "game.h"
#include "game_main.h"
#include "hop_spit_enemy.h"
#include "last_position.h"
#include "partner_follow.h"
#include "route_flier.h"
#include "target_race.h"
#include "title_screen.h"
#include "value_command.h"
#include "bs003.h"
#include "action_select_vars.h"
#include "char_ground.h"
#include "map_placement.h"
#include "texture_load.h"
#include "camera_vars.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "render_vars.h"
#include "script_system.h"
#include "world.h"
#include "math_float.h"

#define BS004_PI 3.1415f
#define BS004_TWO_PI 6.283f
#define BS004_ENDING_MODEL_A 88                 // the two models of the ending (bs_004en.it3 and bs_004pa.it3)
#define BS004_ENDING_MODEL_B 89
#define BS004_ENDING_START (-30)
#define BS004_PEOPLE_FIRST 0x3a
#define BS004_ROOM_FLAG_BASE 1000               // + 2 * room: flag word of the ending
#define BS004_FAR_X (-200000.0f)
#define BS004_HOVER_HEIGHT 700.0f
#define BS004_FLOOR_FIRST 92                    // slots 92..123: the kind 0x25 characters it stops when it is hit
#define BS004_FLOOR_END 124
#define BS004_SPECTATOR_FIRST 93                // slots 93..95: the three characters it checks for nearness
#define BS004_SPECTATOR_END 96
#define BS004_MINION_FIRST_OFFSET 2             // minions are the slots index + 2 .. index + 4
#define BS004_FLAG_DEFEAT_MODEL 0x1a4           // script flag holding the model that leaves with the defeat scene
#define BS004_PATH_NODE 0                       // DAT_1021bdf8[slot][0]: "@MossBeeRoot", the node whose frames are the path
#define BS004_BONE_FOOT_L 0                     // DAT_1042d918[...]: bones of the creature
#define BS004_BONE_FOOT_R 1
#define BS004_BONE_TAIL 2
#define BS004_BONE_TORCH 3
#define BS004_BONE_FOREARM_L 4
#define BS004_BONE_FINGER_L 5
#define BS004_BONE_FOREARM_R 6
#define BS004_BONE_FINGER_R 7
#define BS004_NODE_FLAGS_HIDE 0x20
#define BS004_BASE_KIND_PACKED_GROUP 0x31
#define BS004_BASE_KIND_HIDE_ARMOUR 0x10
#define BS004_KIND_FLOOR 0x25

#define BS004_MOVE_NONE 0
#define BS004_MOVE_WALK 1                       // move by the vector, scaled by the motion speed (Char_Move 0x100241e0)
#define BS004_MOVE_FLY 2                        // move by the vector, shrunk until the way is clear of the map

// States (Bs004State::state).
#define BS004_STATE_HOVER 0
#define BS004_STATE_FOLLOW_PATH 1
#define BS004_STATE_DRIFT 2
#define BS004_STATE_RECOIL 3
#define BS004_STATE_PICK_POINT 4
#define BS004_STATE_APPROACH_POINT 5
#define BS004_STATE_WAIT 6
#define BS004_STATE_ROAR 7
#define BS004_STATE_DIVE_A 8
#define BS004_STATE_SWIPE 9
#define BS004_STATE_APPROACH_POINT_B 10
#define BS004_STATE_DIVE_B 11
#define BS004_STATE_TAIL_SWEEP 12
#define BS004_STATE_SLAM 13
#define BS004_STATE_RISE_A 14
#define BS004_STATE_PLUNGE_A 15
#define BS004_STATE_RISE_B 16
#define BS004_STATE_PLUNGE_B 17
#define BS004_STATE_GROUND_BLAST 18
#define BS004_STATE_FIRE_RAIN 19
#define BS004_STATE_PICK 100

// Frames of the rise and plunge states.
#define BS004_RISE_SETTLE_FRAMES 0x3c          // the climb starts after this many frames of settling
#define BS004_RISE_LOW_TOP 950.0f              // height state 14 climbs to
#define BS004_RISE_HIGH_TOP 2550.0f            // height state 16 climbs to
#define BS004_MOTION_FRAME_RISE_TOP 0x156      // frame of the climb motion at which the plunge may start
#define BS004_PLUNGE_LANDED 1000               // frames value of a plunge that touched the ground
#define BS004_SLAM_LUNGE_SPEED 25.0f           // forward speed the slam throws it with
#define BS004_SLAM_LUNGE_LIFT 10.0f            // and the lift
#define BS004_SPREAD_WIDE 0.66731f             // weights of the claw spread bursts (they add up to about 1)
#define BS004_SPREAD_NARROW 0.333f
#define BS004_PLUNGE_DONE 2000                 // frames value of a plunge that bounced back

#define BS004_EFFECT_SPARK 0x6d4
#define BS004_EFFECT_PAIR 0x6dd

static __forceinline float Atan2(float y, float x)
{
    return FUN_10001f80(y, x);
}

static __forceinline Node *Bs004Bone(int which)
{
    return (Node *)DAT_1042d918[which];
}

// Smooths the movement vector towards a state's step vector: move = last move * keep + step * give (DAT_10802bf8
// keeps the movement of the last frame in its position).
static __forceinline void BlendMove(int index, Vec4 &move, const Vec4 &step, float keep, float give)
{
    move.Copy(DAT_10802bf8[index].position * keep + step * give);
}

// Starts effect `effect` for the creature at `where`; `owner` is the node matrix the effect follows (0 = none).
static __forceinline void StartEffect(int index, int effect, const Vec4 &where, const Matrix4 *owner, const Matrix4 *pose,
                        const EffectOptions *options)
{
    FUN_1001d8b0(index, effect, where, -1, -1, owner, 0, 0, pose, options);
}

// Fills the options of the creature's larger effects: its effect parameter, a scale and the follow flag byte.
static __forceinline void SetUpOptions(int index, EffectOptions &options, float scale)
{
    options.m_0000 = 0;
    options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
    options.m_0008 = scale;
    options.m_000e = 5;
}

// A turn about z of `heading` placed at `where`.
static __forceinline void MakePose(Matrix4 &pose, float heading, const Vec4 &where)
{
    pose.SetIdentity();
    pose = RotationZ(heading);
    pose.SetTranslation(where);
}

// Two small burst effects (0x6dd) at the hands: each starts at the origin of its pose, which sits on a bone.
static __forceinline void StartHandBursts(int index, Matrix4 &poseA, Matrix4 &poseB)
{
    Vec4 where;
    StartEffect(index, BS004_EFFECT_PAIR, where, 0, &poseA, 0);
    StartEffect(index, BS004_EFFECT_PAIR, where, 0, &poseB, 0);
}

// One burst of the claw spread: moves `pose` (on a hand) towards the camera look point DAT_10802bb8 by the
// weights `wide` for the tip and `narrow` for the hand, then starts the burst there.
static __forceinline void StartSpreadBurst(int index, Matrix4 &pose, const Vec4 &where, float wide, float narrow)
{
    pose.SetTranslation(DAT_10802bb8[index][0] * wide + pose.GetTranslation() * narrow);
    StartEffect(index, BS004_EFFECT_PAIR, where, 0, &pose, 0);
}

// The two hand poses of the bursts: the creature's heading placed at two bones.
static __forceinline void MakeHandPoses(int index, Matrix4 &poseA, Matrix4 &poseB, int boneA, int boneB)
{
    poseA.SetIdentity();
    poseA = RotationZ(DAT_10db6ee8[index].heading);
    poseB = poseA;
    poseA.SetTranslation(Bs004Bone(boneA)->worldMatrix.GetTranslation());
    poseB.SetTranslation(Bs004Bone(boneB)->worldMatrix.GetTranslation());
}

// Starts a minion slot over: clears the tables of its slot like Char_SpawnEnemy (0x10050510) does, tags its model nodes
// and puts its equipment back on. `fan` is the minion's place in the fan of three (-1, 0 or 1).
static __forceinline void ActivateMinion(int slot, int fan)
{
    AnimModel &minion = DAT_10db6ee8[slot];
    CharSlot &minionSlot = DAT_10daaae8[slot];
    minionSlot.stats[STAT_HEALTH] = minionSlot.stats[STAT_MAX_HEALTH];
    minion.root->M_10194630(slot, 0x400);
    minion.root->M_101946b0(0xfe, 0x400);
    minion.root->M_10194730(0x2f, 0x400);
    minion.root->M_10194630(0, 0x200);
    minion.root->M_101946b0(0xff, 0x200);
    minion.root->M_10194730(0x2f, 0x200);
    minion.alpha = 1.0f;
    DAT_10254090[slot] = 0;
    DAT_10453008[slot] = 0;
    DAT_10254598[slot] = 0x5b;
    DAT_1030b8f8[slot] = 0;
    DAT_1020d540[slot] = 0;
    DAT_10256420[slot] = 1;
    minion.shadowOpacity = 1;
    DAT_10254390[slot] = 0;
    DAT_1021f940[slot] = 0;
    DAT_1020e138[slot] = -1;
    minion.m_0294 = 0;
    DAT_10253a58[slot] = 0;
    DAT_1021b4c8[slot] = (char)(slot % 5) - 1;
    for (int word = 0; word < 0x10; word++)
        DAT_1020b430[slot].words[word] = 0;
    DAT_1020b430[slot].words[2] = fan;
    FUN_10065b10(slot, 0x2d5, 1, 0);
    minionSlot.money = 0;
    minionSlot.m_00c0 = 0;
    minionSlot.m_00c4 = 0;
    minionSlot.m_00c8 = 0;
    DAT_1030c480[slot] = 0;
    DAT_10467510[slot] = 0;
    minion.m_02a8 = 0;
    minion.holdState = 0;
    minion.holdWeight = 0.0f;
    minion.m_022c.Fill(0.0f);
    DAT_10467ba0[slot] = 0;
    DAT_102239d0[slot] = 0;
    DAT_10210d30[slot] = 0;
    DAT_104686b0[slot] = 0;
    DAT_1020d4c0[slot] = 0;
    DAT_102550e0[slot] = 0;
    DAT_10218fc0[slot] = 0;
    DAT_10210b30[slot] = 0;
    DAT_10802bf8[slot].position.Fill(0.0f);
    DAT_10802bf8[slot].step.Fill(0.0f);
    int packed = minionSlot.stats[9];
    DAT_10315350[slot] = 0;
    DAT_1021f6c0[slot] = 0;
    DAT_102548c0[slot] = 0;
    DAT_10219e40[slot] = 0;
    DAT_1042d9e8[slot] = 0;
    bool hidesArmour = minionSlot.kind == BS004_BASE_KIND_HIDE_ARMOUR || DAT_1030b668[slot] == BS004_BASE_KIND_HIDE_ARMOUR;
    if (packed / 10000 == 0 &&
        ((minionSlot.kind != BS004_BASE_KIND_PACKED_GROUP && DAT_1030b668[slot] != BS004_BASE_KIND_PACKED_GROUP) ||
         packed % 100 == 0))
    {
        if (hidesArmour)
        {
            Node *armour = minion.root->Find("@EN_016ar.lwo", 0);
            if (armour != 0)
                armour->ClearFlags(BS004_NODE_FLAGS_HIDE);
        }
    }
    else
    {
        if (hidesArmour)
        {
            Node *armour = minion.root->Find("@EN_016ar.lwo", 0);
            if (armour != 0)
                armour->SetFlags(BS004_NODE_FLAGS_HIDE);
        }
        int weapon = 0;
        if (minionSlot.kind == BS004_BASE_KIND_PACKED_GROUP || DAT_1030b668[slot] == BS004_BASE_KIND_PACKED_GROUP)
            weapon = packed % 100;
        FUN_100404e0(slot, packed / 10000000 % 100, packed / 100000 % 100, packed / 1000 % 100, weapon, 0, 0);
    }
}

// Sets the target heading to face the player (the creature's model faces backwards: turned by pi).
static __forceinline void FacePlayer(AnimModel &model, const Vec4 &ownPos, const Vec4 &playerPos)
{
    model.targetHeading = Atan2(playerPos.x - ownPos.x, playerPos.y - ownPos.y) + BS004_PI;
}

// Sets the target heading to face the way the creature moves (backwards model, turned by pi).
static __forceinline void FaceMove(AnimModel &model, const Vec4 &move)
{
    model.targetHeading = Atan2(move.x, move.y) + BS004_PI;
}

// Turns an angle into -pi..pi.
static __forceinline float WrapAngle(float angle)
{
    while (angle > BS004_PI)
        angle -= BS004_TWO_PI;
    while (angle < -BS004_PI)
        angle += BS004_TWO_PI;
    return angle;
}

// Starts a spark effect at a random bone (not the torch bones 2 and 3 of the index list).
static __forceinline void StartBoneSpark(int index)
{
    Matrix4 pose;
    pose.SetIdentity();
    int pick = FUN_1013f880() % 6;
    if (pick == 2 || pick == 3)
        pick++;
    Vec4 where = Bs004Bone(pick)->worldMatrix.GetTranslation();
    StartEffect(index, BS004_EFFECT_SPARK, where, 0, &pose, 0);
}

// Plans a climb: the glide (DAT_10802bf8 step) heads half way towards the player and up to height `top`, at 1/80 of that
// vector per frame, with `zDivisor` frames for the vertical part. `move` is used as scratch.
static __forceinline void PlanRise(int index, Vec4 &move, const Vec4 &ownPos, const Vec4 &playerPos, float top,
                                   float zDivisor)
{
    move.Fill(0.0f);
    move.Copy(playerPos - ownPos);
    move.Copy(move * 0.5f);
    move.z = top - ownPos.z;
    DAT_10802bf8[index].step.Copy(move.Divided(80.0f));
    DAT_10802bf8[index].step.z = move.z / zDivisor;
}

// Plans a plunge from height `top`: the glide heads for the player at 1/60 of the way per frame; `move` is scratch.
static __forceinline void PlanPlunge(int index, Vec4 &move, const Vec4 &ownPos, const Vec4 &playerPos, float top)
{
    move.Fill(0.0f);
    move.Copy(playerPos - ownPos);
    move.z = top - ownPos.z;
    DAT_10802bf8[index].step.Copy(move.Divided(60.0f));
    move.z = (top - ownPos.z) / 15.0f;
}

// The plunge reaches the ground: shakes the screen, plays `sound` and starts the blast `effect` and the dust (0x6db)
// there. The creature stays put (state frames jump to BS004_PLUNGE_LANDED).
static __forceinline void LandFromPlunge(int index, Vec4 &move, const Vec4 &ownPos, int sound, int effect)
{
    Bs004State &st = DAT_1020b430[index].bs004;
    FUN_1016b230("");
    FUN_10055ee0();
    FUN_1003ad20(sound, 0, 1.0f);
    move.Fill(0.0f);
    st.motion = 0xd;
    st.frames = BS004_PLUNGE_LANDED;
    DAT_10db6ee8[index].SetPosition(ownPos.x, ownPos.y, 0.0f);
    EffectOptions options;
    Matrix4 pose;
    Vec4 where;
    SetUpOptions(index, options, 0.9f);
    pose.SetIdentity();
    pose.SetTranslation(ownPos);
    StartEffect(index, effect, where, 0, &pose, &options);
    StartEffect(index, 0x6db, where, 0, &pose, &options);
}

// The part of a plunge after it landed: stands still, then bounces back off the glide (`bounce` is its new height), then
// slows to a stop and hands over to the state pick. `fadeOut` also clears the screen tint of the high plunge.
static __forceinline void FinishPlunge(int index, Bs004State &st, Vec4 &move, float bounce, bool fadeOut)
{
    if (st.frames < BS004_PLUNGE_LANDED + 3)
    {
        move.Fill(0.0f);
        st.motion = 0xd;
    }
    else if (st.frames < BS004_PLUNGE_DONE)
    {
        move.Fill(0.0f);
        st.motion = 0xd;
        st.frames = BS004_PLUNGE_DONE;
        DAT_10802bf8[index].step.Copy(DAT_10802bf8[index].step * -2.0f);
        DAT_10802bf8[index].step.z = bounce;
    }
    else if (st.frames > BS004_PLUNGE_DONE)
    {
        if (fadeOut)
            FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
        st.motion = 0xd;
        move.Copy(DAT_10802bf8[index].step);
        BlendMove(index, move, move, 0.2f, 0.8f);
        DAT_10802bf8[index].step.Copy(DAT_10802bf8[index].step * 0.9f);
        if (st.frames > 0x7ed)
        {
            st.state = BS004_STATE_PICK;
            st.motion = 2;
        }
    }
}

// The spinning blast of a plunge: turns the creature by `turn` radians per frame and, while it falls, starts the dust
// shards (`threeShards`: one more for the high plunge) and the 0x45d effect on its model.
static __forceinline void SpinAndBurst(int index, const Vec4 &ownPos, float turn, bool threeShards)
{
    AnimModel &self = DAT_10db6ee8[index];
    float spin = self.heading + turn;
    self.heading = spin;
    self.targetHeading = spin;
    Matrix4 pose;
    EffectOptions options;
    Vec4 where;
    pose.SetIdentity();
    pose.SetTranslation(ownPos);
    options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
    int frames = DAT_1020b430[index].bs004.frames;
    options.m_0000 = 0;
    options.m_000e = 5;
    if (frames > 0xf && frames < 1000)
    {
        StartEffect(index, 0x6e5, where, 0, &pose, 0);
        StartEffect(index, 0x6e5, where, 0, &pose, 0);
        if (threeShards)
            StartEffect(index, 0x6e5, where, 0, &pose, 0);
        options.flags |= 2;
        StartEffect(index, 0x45d, where, 0, &pose, &options);
    }
}

// 0x100c39c0 (Bs004_Update). One frame of the moss root creature in slot `index`; see the comment at the top of the
// file. While the game is over it runs the ending scene (returns 1); otherwise it returns 0.
int Bs004_Update(int index)
{
    Bs004State &st = DAT_1020b430[index].bs004;
    Vec4 ownPos;
    Vec4 playerPos;
    Vec4 offset;
    Vec4 step;
    Vec4 move;
    Vec4 target;
    Vec4 probe;
    int moveMode = BS004_MOVE_NONE;
    int turn = 1;

    DAT_1021f6c0[index] = 0;
    DAT_10db6ee8[index].mode = 0;
    DAT_10db6ee8[index].m_238c = 0;
    DAT_10218fc0[index] = 0;
    DAT_11b04510 = 1;
    DAT_10db6ee8[index].locked = 1;
    st.motion = -1;
    DAT_104c0110 = index;
    DAT_10db6ee8[index].uniformScale = 0.06f;
    if (DAT_104c0380 % 0x1e == index % 0x1e)
    {
        DAT_10db6ee8[index].m_0012 = 2;
        FUN_1001c090(index, 1);
    }
    ownPos.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    playerPos.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    offset = ownPos - playerPos;
    float distance2D = offset.Length2D();
    offset = ownPos - playerPos;
    float distance3D = offset.Length();
    DAT_1020344c = 1.8f;
    DAT_10866d98.z = playerPos.z / 3.0f + 300.0f;
    if (DAT_104bfdf0 != 0 || DAT_104c0135 != 0 || DAT_104bfe1c != 0)
    {
        DAT_10db6ee8[index].m_0294 = 1;
        st.paused = 1;
        return 0;
    }
    if (st.paused != 0)
    {
        st.paused = 0;
        DAT_10db6ee8[index].m_0294 = 0;
    }

    if (DAT_10210d30[index] != 0)
    {
        if (DAT_1020b478[index].value == 0)
        {
            DAT_1020b478[index].value = 1;
            for (int word = 0; word < 0xf; word++)
                DAT_1020b430[index].words[word] = 0;
        }
        AnimModel &endingA = DAT_10db6ee8[BS004_ENDING_MODEL_A];
        AnimModel &endingB = DAT_10db6ee8[BS004_ENDING_MODEL_B];
        int room = DAT_10c63ffc;
        DAT_104c0354 = 2;
        DAT_10daaae8[index].m_0141 = 1;
        DAT_10daaae8[index].stats[STAT_EFFECT_PARAM] = 0;
        DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        int roomFlags = DAT_10874848.TestScriptFlag(room * 2 + BS004_ROOM_FLAG_BASE);
        DAT_10874848.SetScriptFlag(room * 2 + BS004_ROOM_FLAG_BASE, roomFlags | (1 << (index - BS004_PEOPLE_FIRST)));
        endingB.scale.w = DAT_10255160[index][1];
        int &timer = st.hitLevel;
        if (timer < 0x1e)
        {
            FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
            if (timer % 10 == 0 || FUN_1013f880() % 100 == 0)
            {
                Vec4 spot = DAT_10db6ee8[index].matrix.GetTranslation();
                FUN_1001d8b0(index, FUN_1013f880() % 2 == 0 ? 0x321 : 0x322, spot, -1, -1, 0, 0, 0, 0, 0);
            }
            if (timer >= 0)
                FUN_100241e0(index, 0.0f, 0.0f, (550.0f - ownPos.z) / (float)(0x1e - timer));
        }
        else if (timer == 0x1e)
        {
            FUN_1013fb10(3, 200, 30);
            FUN_100b9bb0(index, 0x1ff, 1, 0);
        }
        if (timer == 0xb4)
        {
            FUN_1000db30(0, 0, 0);
            FUN_1000db30(0, 100, 20);
        }
        else if (timer == 0xc3)
        {
            FUN_1013fb10(3, 0, 10);
            DAT_10874848.MarkKilled(DAT_10daaae8[index].routeRecord);
            DAT_10db6ee8[index].SetPosition(BS004_FAR_X, 0.0f, 0.0f);
            endingA.SetPosition(BS004_FAR_X, 0.0f, 0.0f);
            endingB.SetPosition(BS004_FAR_X, 0.0f, 0.0f);
            DAT_104c00c8 = 0;
            DAT_104c0354 = 0;
            DAT_11018734 = 1;
        }
        if (st.hits == 0)
        {
            FUN_10011c60();
            for (int floor = BS004_FLOOR_FIRST; floor < BS004_FLOOR_END; floor++)
            {
                if (DAT_10daaae8[floor].kind == BS004_KIND_FLOOR && DAT_1020b430[floor].words[8] != 0)
                    DAT_1020b430[floor].words[8] = 0;
            }
            DAT_10db6ee8[index].PlayMotion(0x12, 1, 5, 1.0f);
            st.pathPoint = 0x12;
            FUN_10065b10(index, 0x2fd, 1, 0);
            if (DAT_104bfe78 == 0)
                st.hits = 1;
            endingA.targetHeading = DAT_10db6ee8[index].targetHeading;
            endingB.targetHeading = DAT_10db6ee8[index].targetHeading;
            endingA.heading = DAT_10db6ee8[index].heading;
            endingB.heading = DAT_10db6ee8[index].heading;
            timer = BS004_ENDING_START;
            endingA.m_0000.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
            endingB.m_0000.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
            if (DAT_10db6ee8[index].root != 0)
                DAT_10db6ee8[index].root->M_101946b0(0, 0xffffffff);
            if (endingA.root != 0)
                endingA.root->M_101946b0(0, 0xffffffff);
            if (endingB.root != 0)
                endingB.root->M_101946b0(0, 0xffffffff);
            DAT_10db6ee8[index].alpha = 1.0f;
            DAT_104c00c8 = 1;
            DAT_10453008[index] = -10001;
            endingA.alpha = 0.0f;
            endingB.alpha = 0.0f;
        }
        int clock = timer;
        if (clock > 0)
        {
            if (clock < 0x3c)
            {
                DAT_10db6ee8[index].alpha = 1.0f - (float)clock / 60.0f;
            }
            else
            {
                DAT_10db6ee8[index].m_0294 = 1;
                DAT_10db6ee8[index].motionFrame = endingA.motionFrame;
                DAT_10db6ee8[index].alpha = 0.0f;
            }
            if (clock < 0x3c)
                endingA.scale.Set(1.0f, 0.0f, 0.0f, endingA.scale.w + (float)(0x3c - clock) / 1830.0f);
            else if (clock < 90)
                endingA.scale.Set(1.0f, 0.0f, 0.0f, 1.0f);
            else if (clock < 0xa8)
            {
                if (clock == 0x5a)
                {
                    FUN_10065b10(index, 0x2fe, 1, 0);
                    if (DAT_10c646f4 != 0)
                    {
                        Script_ValueCommand(0x7549, 0x4b);
                        int modelId = DAT_10874848.TestScriptFlag(BS004_FLAG_DEFEAT_MODEL);
                        if (modelId != 0)
                        {
                            DAT_10db6ee8[modelId].SetPosition(-100000.0f - DAT_10db6ee8[index].matrix._41, DAT_10db6ee8[index].matrix._42,
                                                              DAT_10db6ee8[index].matrix._43);
                            DAT_10c6356c = 100000;
                            DAT_10c63588 = 7;
                            Script_ValueCommand(0x753f, 0x4b);
                        }
                    }
                }
                float fade = (float)(0xa8 - clock);
                float rise = (float)(clock - 90);
                endingA.scale.Set(1.0f, endingA.scale.y + fade * fade * 0.004213826f,
                                  endingA.scale.z - rise * rise * 0.009481108f, endingA.scale.w - rise / 3081.0f);
                if (endingA.scale.w < 0.0f)
                    endingA.scale.w = 0.0f;
            }
            else
                endingA.scale.Fill(0.0f);
        }
        clock = timer;
        if (clock > 0x29)
        {
            if (clock < 0x72)
            {
                float since = (float)(clock - 0x2a);
                endingB.scale.z = 0.0f;
                endingB.scale.x = (float)(0x72 - clock) * 0.000152207f + endingB.scale.x;
                endingB.scale.y = since / 8.0f;
                endingB.scale.w = since / 144.0f + 0.5f;
                if (endingB.scale.x < 0.0f)
                    endingB.scale.x = 0.0f;
            }
            else if (clock < 0xd2)
            {
                if (clock == 0x72)
                    endingB.scale.x = 0.3f;
                float since = (float)(clock - 0x72);
                float left = (float)(0xd2 - clock);
                endingB.scale.x = endingB.scale.x - since * 8.591065e-05f;
                endingB.scale.y = left * 0.08591065f + endingB.scale.y;
                endingB.scale.z = since * 0.128866f + endingB.scale.z;
                endingB.scale.w = (left * left) / 9216.0f;
                if (endingB.scale.x < 0.0f)
                    endingB.scale.x = 0.0f;
                endingB.heading = endingB.heading - (float)(clock - 0x60) * 0.00035f;
                endingB.targetHeading = endingB.heading;
            }
            else
                endingB.scale.Fill(0.0f);
        }
        endingA.m_0294 = 1;
        endingB.m_0294 = 1;
        endingA.motionFrame = DAT_10db6ee8[index].motionFrame;
        endingB.motionFrame = DAT_10db6ee8[index].motionFrame;
        endingA.SetPosition(DAT_10db9240, DAT_10db9244, DAT_10db9248);
        endingB.SetPosition(DAT_10db9240, DAT_10db9244, DAT_10db9248);
        timer++;
        endingA.alpha = endingA.scale.w;
        DAT_10255160[index][1] = endingB.scale.w;
        endingB.scale.w = DAT_10255160[index][1] * 1.5f;
        endingB.alpha = endingB.scale.w;

        step.Fill(0.0f);
        step.y += 1050.0f;
        step.RotateZ(-(Atan2(DAT_10868818.y - DAT_10da9150.y, DAT_10868818.x - DAT_10da9150.x) + BS004_PI));
        DAT_10edbd48.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
        DAT_10edbd48.z += 600.0f;
        DAT_10866d98.Copy(DAT_10edbd48 + step * 1.6f);
        if (timer < 0xa1)
        {
            float k = (float)(0xa0 - timer);
            k = (k + 1.0f) * k * 0.5f;
            k = k * k;
            float blend = 1.0f - k / 3.292411e+08f;
            DAT_10866d98.Copy(DAT_10da9150 + (DAT_10866d98 - DAT_10da9150) * blend);
            DAT_10edbd48.Copy(DAT_10868818 + (DAT_10edbd48 - DAT_10868818) * blend);
        }
        probe.Fill(0.0f);
        if (DAT_10866d98.Length2D() > 1550.0f && DAT_10255f08 != 0 &&
            DAT_10255f08->CastSegment("CA", 4, &DAT_10866d98, &DAT_10edbd48, &target, 0) != 0)
        {
            float gap = (DAT_10edbd48 - target).Length();
            gap = gap <= 60.0f ? gap * 0.75f : gap - 50.0f;
            offset.Copy(DAT_10edbd48 - DAT_10866d98);
            offset.Copy(offset.Normalized3() * gap);
            DAT_10866d98.Copy(target + offset);
            if (gap < 300.0f)
            {
                offset.Copy(DAT_10866d98 - DAT_10868818);
                offset.RotateZ(0.0f);
                DAT_10da9150.Copy(DAT_10868818 + offset);
            }
        }
        float distance = (DAT_10edbd48 - DAT_10866d98).Length();
        float &closest = DAT_10255160[index][0];
        if (distance <= closest)
            closest = distance;
        else
        {
            closest = (distance - closest) * 0.3f + closest;
            offset.Copy(DAT_10edbd48 - DAT_10866d98);
            offset.Copy(offset.Normalized3() * closest);
            DAT_10866d98.Copy(DAT_10edbd48 - offset);
        }
        return 1;
    }

    // Fight. The first frame loads the models and finds the nodes.
    DAT_1021f9c0[index] = 0;
    DAT_10daaae8[index].m_0141 = 0;
    DAT_10daaae8[index].stats[STAT_BUSY] = 0;
    if (st.setUp == 0)
    {
        if (DAT_10255f08 == 0)
            return 0;
        DAT_1020b48c[index].value = DAT_10daaae8[index].stats[STAT_VALUE_4];
        DAT_104c0354 = 3;
        st.playingMotion = -1;
        st.motion = 2;
        st.state = BS004_STATE_HOVER;
        st.frames = 0x118;
        st.requested = 1;
        Node *crown = DAT_10db6ee8[index].root->Find("futa", 0);
        if (crown != 0)
            crown->SetFlags(BS004_NODE_FLAGS_HIDE);
        st.setUp = 1;
        if (DAT_104c0390 == 0)
        {
            DAT_104c0390 = (Node *)(*DAT_11b0b938)(0x18a0, "tsm_lprootnode");
            DAT_104c0390->Init();
            DAT_104c0390->LoadIt3(FUN_1013f8a0("mossroot.it3", 0), 0, 0);
        }
        st.pathPoint = 0;
        st.previousPoint = 0;
        DAT_1021bdf8[index][BS004_PATH_NODE] = DAT_104c0390->Find("@MossBeeRoot", 0);
        DAT_1042d918[BS004_BONE_FOOT_L] = (int)DAT_10db6ee8[index].root->Find("Bip01 L Foot", 0);
        DAT_1042d918[BS004_BONE_FOOT_R] = (int)DAT_10db6ee8[index].root->Find("Bip01 R Foot", 0);
        DAT_1042d918[BS004_BONE_TAIL] = (int)DAT_10db6ee8[index].root->Find("Bone_tail_top", 0);
        DAT_1042d918[BS004_BONE_TORCH] = (int)DAT_10db6ee8[index].root->Find("Bone_torch_top", 0);
        DAT_1042d918[BS004_BONE_FOREARM_L] = (int)DAT_10db6ee8[index].root->Find("Bip01 L Forearm", 0);
        DAT_1042d918[BS004_BONE_FINGER_L] = (int)DAT_10db6ee8[index].root->Find("Bip01 L Finger0", 0);
        DAT_1042d918[BS004_BONE_FOREARM_R] = (int)DAT_10db6ee8[index].root->Find("Bip01 R Forearm", 0);
        DAT_1042d918[BS004_BONE_FINGER_R] = (int)DAT_10db6ee8[index].root->Find("Bip01 R Finger0", 0);
        DAT_10db6ee8[BS004_ENDING_MODEL_A].LoadFile(FUN_1013f8a0("bs_004en.it3", 0));
        DAT_10db6ee8[BS004_ENDING_MODEL_B].LoadFile(FUN_1013f8a0("bs_004pa.it3", 0));
        DAT_10db6ee8[BS004_ENDING_MODEL_A].LoadModel(FUN_1013f8a0("bs_004m.it3", 0));
        DAT_10db6ee8[BS004_ENDING_MODEL_B].LoadModel(FUN_1013f8a0("bs_004m.it3", 0));
        DAT_10db6ee8[BS004_ENDING_MODEL_A].mode = 0;
        DAT_10db6ee8[BS004_ENDING_MODEL_B].mode = 0;
        DAT_10db6ee8[BS004_ENDING_MODEL_A].scale.Fill(0.0f);
        DAT_10db6ee8[BS004_ENDING_MODEL_B].scale.Fill(0.0f);
        DAT_10255160[index][0] = 0.0f;
        DAT_10db6ee8[BS004_ENDING_MODEL_A].SetPosition(BS004_FAR_X, 0.0f, 0.0f);
        DAT_10db6ee8[BS004_ENDING_MODEL_B].SetPosition(BS004_FAR_X, 0.0f, 0.0f);
        DAT_10836c70.ResetFrameTimer();
    }
    DAT_10255a88[index] = 0;
    DAT_10210d30[index] = 0;
    if (DAT_10210b30[index] != 0)
        DAT_10210b30[index] = 0;

    // Health bookkeeping: the damage taken adds up, a drop below half health starts the angry phase once.
    int health = DAT_10daaae8[index].stats[STAT_HEALTH];
    if (DAT_1020b474[index].value != health)
    {
        if (health < DAT_1020b474[index].value)
            DAT_1020b470[index].value += DAT_1020b474[index].value - health;
        DAT_1020b474[index].value = health;
    }
    if (DAT_1020b47c[index].value == 0 && health < DAT_10daaae8[index].stats[STAT_MAX_HEALTH] / 2)
    {
        if (st.state == BS004_STATE_WAIT || st.state == BS004_STATE_APPROACH_POINT)
            st.state = BS004_STATE_PICK;
        if (st.state != BS004_STATE_PICK && st.state != BS004_STATE_APPROACH_POINT_B)
        {
            st.state = BS004_STATE_PICK;
            DAT_1020b480[index].value = 0x10;
            st.minionsDown = 0;
            DAT_1020b47c[index].value = 1;
        }
    }
    Matrix4 &lastPose = DAT_10868838[index][0];
    offset = lastPose.GetTranslation() - ownPos;
    if (offset.Length() >= 5.0f || st.state == BS004_STATE_PICK)
        DAT_1020b494[index].value = 0;
    else
    {
        int stuck = ++DAT_1020b494[index].value;
        if (stuck > 1000 ||
            (st.state != 0x13 && st.state != 0x12 && st.state != 0 && st.state != 2 && st.state != 3 &&
             st.state != 6 && stuck > 300))
        {
            st.state = BS004_STATE_APPROACH_POINT_B;
            st.frames = 0;
            st.lastState = 1;
            st.previousPoint = st.pathPoint;
            st.pathPoint = FUN_1013f880() % 0x11;
            DAT_1020b494[index].value = 0;
        }
    }
    lastPose.SetTranslation(ownPos);
    if (DAT_10db9274 == 0)
    {
        if (DAT_104c00e8 == 0x23 || DAT_104c00e8 == 0x26 || DAT_104c00e8 == 0x2a)
            DAT_1020b498[index].value = 1;
    }
    else
        DAT_1020b498[index].value = 0;
    DAT_102192c0[index] = (DAT_1020b498[index].value != 0 ? 0x177 : 0) - 0xaf;

    // Hit reaction.
    int hitLevel = DAT_102550e0[index];
    if (hitLevel != 0)
    {
        int forceDown = 0;
        st.playingMotion = -1;
        if (st.hitLevel < hitLevel)
        {
            if (st.state == BS004_STATE_WAIT)
            {
                int difficulty = DAT_10874848.TestScriptFlag(0x1c7);
                if (difficulty == 1 && st.frames < 0x7d)
                    st.frames = 0x7d;
                else if (difficulty == 0 && st.frames < 0xaf)
                    st.frames = 0xaf;
                else if (difficulty == 2 && st.frames < 0xd2)
                    st.frames = 0xd2;
            }
            st.hits++;
        }
        if (hitLevel > 10)
            DAT_102550e0[index] = 10;
        if (st.state == BS004_STATE_WAIT && DAT_10daaae8[index].stats[STAT_MAX_HEALTH] / 3 < DAT_1020b470[index].value)
        {
            DAT_102550e0[index] = 0;
            st.state = BS004_STATE_PICK;
            st.lastState = BS004_STATE_WAIT;
            st.minionsDown = 0;
            forceDown = 1;
        }
        if (st.state == 2 || st.state == 3 || (st.motionLayer > 3 && st.motionLayer < 7) || DAT_10daaae8[index].stats[STAT_BUSY] != 0)
        {
            if (st.motionFrame < 0)
                DAT_10db6ee8[index].PlayMotion(st.motionLayer, 1, 3, 1.0f);
            else
            {
                DAT_10db6ee8[index].PlayMotion(st.motionLayer, 1, 0, 1.0f);
                DAT_10db6ee8[index].motion->frame = st.motionFrame;
                DAT_10db6ee8[index].motion->m_0004 = (float)st.motionFrame;
                DAT_10db6ee8[index].motion->Advance();
            }
            DAT_102550e0[index] = 0;
        }
        int current = st.state;
        if (current == 6 || current == 5 || current == 0x10 || current == 0x11)
        {
            st.hits = 0;
            if (forceDown == 0)
                FUN_100b9bb0(index, FUN_1013f880() % 3 + 0x1fa, 0, 0);
        }
        else
        {
            if (st.hits >= 5 || (DAT_1020b488[index].value >= 0x2e && st.minionsDown >= 3))
            {
                st.minionsDown = 3;
                st.state = BS004_STATE_PICK;
                DAT_1020b494[index].value = 0;
                st.hits = 0;
                DAT_102550e0[index] = 0;
            }
            else if (st.minionsDown < 3)
            {
                st.state = BS004_STATE_APPROACH_POINT_B;
                DAT_1020b494[index].value = 0;
                st.hits = 0;
                st.frames = 0;
                st.previousPoint = st.pathPoint;
                st.pathPoint = FUN_1013f880() % 0x11;
                DAT_102550e0[index] = 0;
                DAT_10daaae8[index].m_0141 = 1;
            }
            if (forceDown == 0)
                FUN_100b9bb0(index, 0x1fd, 1, 0);
        }
    }

    Node *path = DAT_1021bdf8[index][BS004_PATH_NODE];
    switch (st.state)
    {
    case BS004_STATE_HOVER:
        step.Fill(0.0f);
        step.z = BS004_HOVER_HEIGHT - ownPos.z;
        step.Copy(step.Normalized3() * 5.0f);
        BlendMove(index, move, step, 0.9f, 0.1f);
        moveMode = BS004_MOVE_WALK;
        st.motion = 2;
        if (st.frames == 0)
            DAT_1020b488[index].value = 0;
        st.frames++;
        if (st.frames > 0x1c2 && FUN_1013f880() % st.frames > 0x172)
            st.state = BS004_STATE_PICK;
        if (st.hits > 4 || (DAT_1020b488[index].value > 0x2d && st.minionsDown > 2))
        {
            st.minionsDown = 3;
            st.state = BS004_STATE_PICK;
            DAT_1020b494[index].value = 0;
            st.hits = 0;
        }
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        break;
    case BS004_STATE_FOLLOW_PATH:
        target.Fill(0.0f);
        target.z = BS004_HOVER_HEIGHT;
        if (st.pathPoint < path->frameMatrixCount)
            target.Copy(path->frameMatrices[st.pathPoint].GetTranslation());
        step = target - ownPos;
        if (step.Length2D() >= 15.0f)
            moveMode = BS004_MOVE_WALK;
        else
            st.state = BS004_STATE_PICK;
        step.Copy(step.Normalized3() * 10.0f);
        BlendMove(index, move, step, 0.8f, 0.2f);
        st.motion = 0;
        DAT_10db6ee8[index].targetHeading = Atan2(move.x, move.y) + BS004_PI;
        break;
    case BS004_STATE_DRIFT:
        step.Fill(0.0f);
        step.z = BS004_HOVER_HEIGHT - ownPos.z;
        step.Copy(step.Normalized3() * 5.0f);
        BlendMove(index, move, step, 0.9f, 0.1f);
        moveMode = BS004_MOVE_WALK;
        st.motion = 4;
        st.frames++;
        if (st.frames == 1)
        {
            st.minionsDown = 0;
            FUN_100b9bb0(index, 0x1f8, 0, 0);
        }
        DAT_1020b494[index].value = 0;
        if ((st.frames < 0x5b || FUN_1013f880() % st.frames < 0x5b) && (st.frames < 0x2e || st.minionsAlive > 0) &&
            st.minionsDown < 1)
        {
            if (st.frames % 0x1e == 0 && st.frames < 0x5b)
            {
                int minion = index + st.frames / 0x1e + 1;
                if (DAT_10db6ee8[minion].matrix._41 < -50000.0f || DAT_10210d30[minion] != 0)
                {
                    int fan = minion - index - 2;
                    int tries = 0;
                    target.Fill(0.0f);
                    target.z = 300.0f;
                    target.RotateY((float)fan);
                    target.Copy(target + ownPos);
                    while (target.Length2D() > 1450.0f && tries < 30)
                    {
                        fan++;
                        tries++;
                        target.Fill(0.0f);
                        target.z = 300.0f - (float)(tries * 10);
                        target.RotateY((float)fan);
                        target.Copy(target + ownPos);
                    }
                    if (target.Length2D() >= 1450.0f)
                    {
                        target.Fill(0.0f);
                        target.z = 800.0f;
                    }
                    if (target.Length2D() < 1450.0f)
                    {
                        Matrix4 pose;
                        DAT_10db6ee8[minion].SetPosition(target.x, target.y, target.z);
                        ActivateMinion(minion, fan);
                        pose.SetIdentity();
                        pose.SetTranslation(target);
                        StartEffect(index, 5, target, 0, &pose, 0);
                    }
                }
            }
        }
        else
        {
            st.state = BS004_STATE_PICK;
            st.minionsDown = 3;
            st.frames = 0;
        }
        if (st.hits > 4 || (DAT_1020b488[index].value > 0x2d && st.minionsDown > 2))
        {
            st.minionsDown = 3;
            st.state = BS004_STATE_PICK;
            DAT_1020b494[index].value = 0;
            st.hits = 0;
        }
        if (st.frames > 0x1e)
            DAT_10daaae8[index].held = 1;
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        break;
    case BS004_STATE_RECOIL:
        DAT_10daaae8[index].held = 0;
        st.motion = 6;
        st.frames++;
        if (DAT_10db6ee8[index].motionFrame == 0xa0 || st.frames > 0x28)
            st.state = BS004_STATE_PICK;
        break;
    case BS004_STATE_PICK_POINT:
        st.state = BS004_STATE_APPROACH_POINT;
        st.frames = 0;
        st.previousPoint = st.pathPoint;
        st.pathPoint = FUN_1013f880() % 3 + 0x32;
        break;
    case BS004_STATE_APPROACH_POINT:
    {
        // Flies to the path point (the way to the wait): sets down on it and waits.
        DAT_10daaae8[index].m_0141 = 1;
        st.hits = 0;
        target.Fill(0.0f);
        target.z = BS004_HOVER_HEIGHT;
        if (st.pathPoint < path->frameMatrixCount)
            target.Copy(path->frameMatrices[st.pathPoint].GetTranslation());
        step = target - ownPos;
        float distanceSquared = step.LengthSquared3();
        if (st.frames == 0)
            FUN_10065b10(index, 0x2d6, 1, 0);
        st.frames++;
        if (distanceSquared >= 100.0f)
        {
            step.Copy(step.Normalized3() * 100.0f);
            if ((step + move).LengthSquared3() <= distanceSquared)
            {
                BlendMove(index, move, step, 0.2f, 0.8f);
                moveMode = BS004_MOVE_FLY;
                st.motion = 7;
            }
            else
            {
                step.Fill(0.0f);
                DAT_10db6ee8[index].SetPosition(target.x, target.y, target.z);
                st.state = BS004_STATE_WAIT;
                st.lastState = 6;
                st.frames = 0;
                DAT_10daaae8[index].m_0141 = 0;
            }
        }
        else
        {
            DAT_10daaae8[index].m_0141 = 0;
            st.state = BS004_STATE_WAIT;
            st.lastState = 6;
            DAT_10db6ee8[index].SetPosition(target.x, target.y, target.z);
        }
        DAT_10db6ee8[index].targetHeading = Atan2(move.x, move.y);
        {
            Matrix4 pose;
            MakePose(pose, 0.0f, probe);
            probe.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
            StartEffect(index, 0x6cb, probe, 0, &pose, 0);
        }
        break;
    }
    case BS004_STATE_APPROACH_POINT_B:
    {
        // The same flight when it was sent after a hit: it picks its next state on arrival instead of waiting.
        DAT_10daaae8[index].m_0141 = 1;
        st.hits = 0;
        target.Fill(0.0f);
        target.z = BS004_HOVER_HEIGHT;
        if (st.pathPoint < path->frameMatrixCount)
            target.Copy(path->frameMatrices[st.pathPoint].GetTranslation());
        step = target - ownPos;
        float distanceSquared = step.LengthSquared3();
        if (st.frames == 0)
            FUN_10065b10(index, 0x2d6, 1, 0);
        st.frames++;
        if (distanceSquared >= 100.0f)
        {
            step.Copy(step.Normalized3() * 100.0f);
            if ((step + move).LengthSquared3() <= distanceSquared)
            {
                BlendMove(index, move, step, 0.2f, 0.8f);
                moveMode = BS004_MOVE_FLY;
                st.motion = 7;
            }
            else
            {
                step.Fill(0.0f);
                DAT_10db6ee8[index].SetPosition(target.x, target.y, target.z);
                st.state = BS004_STATE_PICK;
                DAT_10daaae8[index].m_0141 = 0;
            }
        }
        else
        {
            DAT_10daaae8[index].m_0141 = 0;
            st.state = BS004_STATE_PICK;
            DAT_10db6ee8[index].SetPosition(target.x, target.y, target.z);
        }
        DAT_10db6ee8[index].targetHeading = Atan2(move.x, move.y);
        {
            Matrix4 pose;
            MakePose(pose, 0.0f, probe);
            probe.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
            StartEffect(index, 0x6cb, probe, 0, &pose, 0);
        }
        break;
    }
    case BS004_STATE_WAIT:
        st.hits = 0;
        st.motion = 2;
        st.frames++;
        if ((st.frames > 300 && FUN_1013f880() % st.frames > 300) ||
            DAT_10daaae8[index].stats[STAT_MAX_HEALTH] / 3 < DAT_1020b470[index].value)
        {
            st.state = BS004_STATE_PICK;
            st.lastState = 6;
            st.minionsDown = 0;
        }
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        for (int spectator = BS004_SPECTATOR_FIRST; spectator < BS004_SPECTATOR_END; spectator++)
        {
            AnimModel &watcher = DAT_10db6ee8[spectator];
            int &closeness = DAT_1020b430[spectator].words[8];
            Vec4 gap = watcher.matrix.GetTranslation() - playerPos;
            if (gap.Length2D() >= 500.0f)
                closeness = 0;
            else
            {
                if (closeness != 2 || watcher.matrix._43 + 330.0f <= playerPos.z)
                    closeness = 1;
                else
                    closeness = 2;
                if (DAT_1020b430[spectator].words[6] > 0x3c)
                    DAT_1020b430[spectator].words[6] = 0x3c;
                gap = watcher.matrix.GetTranslation() - ownPos;
                if (gap.Length2D() >= 100.0f)
                {
                    if (playerPos.z <= 150.0f)
                    {
                        gap = watcher.matrix.GetTranslation() - move;
                        if (gap.Length2D() < 300.0f && (DAT_10db9274 != 0 || closeness == 2))
                            closeness = 2;
                    }
                    else
                        closeness = 3;
                }
                else
                    closeness = 3;
            }
        }
        if (st.state == BS004_STATE_PICK)
        {
            for (int floor = BS004_FLOOR_FIRST; floor < BS004_FLOOR_END; floor++)
            {
                if (DAT_10daaae8[floor].kind == BS004_KIND_FLOOR && DAT_1020b430[floor].words[8] != 0)
                    DAT_1020b430[floor].words[8] = 0;
            }
        }
        break;
    case BS004_STATE_ROAR:
        FUN_1000ec70(0xf, 0.7f, 0.3f, 0.7f, 1.0f);
        st.motion = 0x11;
        if (st.frames == 0)
        {
            FUN_10065b10(index, 0x2d7, 1, 0);
            FUN_100b9bb0(index, 0x1f9, 0, 0);
        }
        if (st.frames == 0xf)
        {
            int first = 0x2d;
            for (int candidate = 0x2d; candidate < 0x7c; candidate++)
            {
                if (DAT_1021f3c0[candidate] == 'N')
                {
                    first = candidate;
                    break;
                }
            }
            EffectOptions options;
            Matrix4 pose;
            SetUpOptions(index, options, 1.2f);
            pose.SetIdentity();
            for (int victim = first; victim < first + 13 && victim < 0x7c; victim++)
            {
                if (FUN_1013f880() % 2 == 0)
                {
                    int roll = FUN_1013f880();
                    int &word = DAT_1020b430[victim].words[6];
                    word = word < 0x2d ? roll % 0x3c + 0x5fa : 0x5dc;
                    Vec4 where = DAT_10db6ee8[victim].matrix.GetTranslation();
                    if (DAT_1020b430[victim].words[4] != 200)
                        pose.m[14] -= 150.0f;
                    pose.SetTranslation(where);
                    FUN_1001d8b0(index, 0x6cc, where, -1, -1, 0, 0, roll % 0x3c, &pose, &options);
                }
            }
        }
        st.frames++;
        if (st.frames < 0x50)
        {
            Matrix4 pose;
            pose.SetIdentity();
            FUN_1013f880();
            probe.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
            FUN_1001d8b0(index, BS004_EFFECT_SPARK, probe, -1, -1, 0, 0, 0, &pose, 0);
        }
        if (DAT_10db6ee8[index].motionFrame == 0x261)
        {
            st.state = BS004_STATE_PICK;
            FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        break;
    case BS004_STATE_DIVE_A:
        // Flies at the player (aiming 400 above it, speeding up over the first second) and swipes once it is near.
        st.frames++;
        step = playerPos - ownPos;
        step.z = (playerPos.z + 400.0f) - ownPos.z;
        step.Normalized3();
        if (playerPos.z + 400.0f != ownPos.z)
            step.z *= 4.0f;
        step.Copy(step * (st.frames < 0x3c ? ((float)st.frames / 60.0f) * 20.0f + 5.0f : 25.0f));
        BlendMove(index, move, step, 0.7f, 0.3f);
        if (distance2D < 600.0f && floorf(playerPos.z + 400.0f - ownPos.z) < 500.0f)
        {
            st.state = BS004_STATE_SWIPE;
            st.lastState = BS004_STATE_SWIPE;
            st.frames = 0;
        }
        st.motion = 0;
        moveMode = BS004_MOVE_WALK;
        FaceMove(DAT_10db6ee8[index], move);
        if ((st.frames & 1) != 0)
        {
            probe.Fill(0.0f);
            StartEffect(index, 0x6d5, probe, &Bs004Bone(BS004_BONE_TAIL)->worldMatrix, 0, 0);
        }
        break;
    case BS004_STATE_DIVE_B:
    {
        // The deep dive (aiming 550 above the player, a little faster): sweeps the tail once it is within 200.
        st.frames++;
        step = playerPos - ownPos;
        step.z = (playerPos.z + 550.0f) - ownPos.z;
        step.Normalized3();
        if (playerPos.z + 550.0f != ownPos.z)
            step.z *= 3.0f;
        step.Copy(step * (st.frames < 0x3c ? ((float)st.frames / 60.0f) * 25.0f + 5.0f : 30.0f));
        BlendMove(index, move, step, 0.7f, 0.3f);
        if (distance2D < 200.0f)
        {
            st.state = BS004_STATE_TAIL_SWEEP;
            st.lastState = BS004_STATE_TAIL_SWEEP;
            st.frames = 0;
        }
        st.motion = 0;
        moveMode = BS004_MOVE_WALK;
        FaceMove(DAT_10db6ee8[index], move);
        if (st.frames % 0xf == 0)
        {
            Matrix4 poseA;
            Matrix4 poseB;
            MakeHandPoses(index, poseA, poseB, BS004_BONE_FOREARM_L, BS004_BONE_FOREARM_R);
            StartHandBursts(index, poseA, poseB);
        }
        break;
    }
    case BS004_STATE_SWIPE:
    {
        // The swipe: glides towards where the player was when it started (a twentieth of the way per frame), rising to
        // hover height and dropping with the player's height while the claw comes down; it faces the way it moves.
        st.motion = 0xe;
        moveMode = BS004_MOVE_WALK;
        DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        if (st.frames == 0)
        {
            move.Copy(playerPos - ownPos);
            DAT_10802bf8[index].step.Copy(move.Divided(20.0f));
        }
        st.frames++;
        move.Fill(0.0f);
        move.Copy(DAT_10802bf8[index].step);
        {
            int frame = DAT_10db6ee8[index].motion->frame;
            if (frame < 0x1b3 || DAT_10db6ee8[index].motion->blending != 0)
            {
                move.z = ((playerPos.z - ownPos.z) + 250.0f) / (float)st.frames;
                if (move.z < -20.0f)
                    move.z = -20.0f;
            }
            else if (frame < 0x1b4 || DAT_10db6ee8[index].motionFrame > 0x1c1)
                move.z = 0.0f;
            else
            {
                move.z = (BS004_HOVER_HEIGHT - ownPos.z) / (float)(0x1c2 - DAT_10db6ee8[index].motionFrame);
                if (move.z > 20.0f)
                    move.z = 20.0f;
            }
        }
        BlendMove(index, move, move, 0.5f, 0.5f);
        if (DAT_10db6ee8[index].motionFrame == 0x1c1)
        {
            DAT_10daaae8[index].stats[STAT_BUSY] = 0;
            st.state = BS004_STATE_PICK;
            st.motion = 2;
        }
        if (DAT_10db6ee8[index].motionFrame == 0x1a4)
        {
            EffectOptions options;
            Vec4 where;
            Matrix4 pose = RotationZ(Atan2(move.x, move.y) + BS004_PI);
            options.m_0000 = 0;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_000e = 5;
            StartEffect(index, 0x6d7, where, &Bs004Bone(BS004_BONE_TAIL)->worldMatrix, &pose, &options);
            FUN_10065b10(index, 0x2d8, 1, 0);
            FUN_100b9bb0(index, 0x1f6, 0, 1);
        }
        DAT_10db6ee8[index].targetHeading = Atan2(move.x, move.y) + BS004_PI;
        break;
    }
    case BS004_STATE_TAIL_SWEEP:
    {
        Matrix4 poseA;
        Matrix4 poseB;
        st.motion = 8;
        st.frames++;
        step = playerPos - ownPos;
        step.z = (playerPos.z + 550.0f) - ownPos.z;
        step.Normalized3();
        step.Copy(step * 5.0f);
        BlendMove(index, move, step, 0.7f, 0.3f);
        moveMode = BS004_MOVE_WALK;
        FaceMove(DAT_10db6ee8[index], move);
        if (st.frames > 0x14 && DAT_10db6ee8[index].motionFrame == 0x116)
        {
            st.state = BS004_STATE_PICK;
            st.requested = 1;
        }
        if (DAT_10db6ee8[index].motionFrame == 0x104)
        {
            FUN_10065b10(index, 0x2d9, 1, 0);
            FUN_100b9bb0(index, 0x1f7, 0, 1);
        }
        if (DAT_10db6ee8[index].motionFrame < 0xff || DAT_10db6ee8[index].motionFrame > 0x108)
        {
            if ((DAT_10db6ee8[index].motionFrame & 1) != 0)
            {
                MakeHandPoses(index, poseA, poseB, BS004_BONE_FINGER_L, BS004_BONE_FINGER_R);
                StartHandBursts(index, poseA, poseB);
            }
        }
        else
        {
            MakeHandPoses(index, poseA, poseB, BS004_BONE_FINGER_L, BS004_BONE_FINGER_R);
            StartHandBursts(index, poseA, poseB);
            if (DAT_10db6ee8[index].motionFrame == 0x108)
            {
                EffectOptions options;
                Matrix4 pose = RotationX(-1.5708f);
                StartHandBursts(index, poseA, poseB);
                pose.SetTranslation(ownPos);
                pose.m[14] -= 350.0f;
                SetUpOptions(index, options, 0.9f);
                StartEffect(index, ownPos.z >= 525.0f ? 0x6e3 : 0x6e1, probe, 0, &pose, &options);
                if (ownPos.z >= 525.0f)
                    FUN_10065b10(index, 0x2da, 1, 0);
            }
            if (DAT_10db6ee8[index].motionFrame > 0x103)
            {
                Vec4 where;
                StartSpreadBurst(index, poseA, where, BS004_SPREAD_WIDE, BS004_SPREAD_NARROW);
                StartSpreadBurst(index, poseA, where, BS004_SPREAD_WIDE, BS004_SPREAD_NARROW);
                StartSpreadBurst(index, poseA, where, BS004_SPREAD_NARROW, BS004_SPREAD_WIDE);
                StartSpreadBurst(index, poseA, where, BS004_SPREAD_NARROW, BS004_SPREAD_WIDE);
            }
        }
        break;
    }
    case BS004_STATE_SLAM:
    {
        st.motion = 9;
        st.frames++;
        step.Fill(0.0f);
        if (DAT_10db6ee8[index].motionFrame == 0x124)
        {
            // The slam throws the creature forward: 25 along its facing, 10 up, which becomes its last move.
            step.y = BS004_SLAM_LUNGE_SPEED;
            step.RotateZ(-DAT_10db6ee8[index].heading);
            step.z = BS004_SLAM_LUNGE_LIFT;
            DAT_10802bf8[index].position.Copy(step);
            float pitch = FUN_10001f60(offset.z / (distance3D > 0.0f ? distance3D : 1.0f));
            EffectOptions options;
            if (pitch > 0.7854f)
                pitch = 0.7854f;
            pitch = pitch * -1.0f;
            Matrix4 pose = RotationX(pitch) * RotationZ(Atan2(playerPos.x - ownPos.x, playerPos.y - ownPos.y) + BS004_PI);
            SetUpOptions(index, options, 1.3f);
            pose.m[12] += ownPos.x;
            pose.m[13] += ownPos.y - 100.0f;
            pose.m[14] = ownPos.z + 100.0f;
            StartEffect(index, 0x6e3, probe, 0, &pose, &options);
            FUN_10065b10(index, 0x2da, 1, 0);
            FUN_100b9bb0(index, 0x1f7, 0, 1);
        }
        BlendMove(index, move, step, 0.8f, 0.2f);
        moveMode = BS004_MOVE_WALK;
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        if (DAT_10db6ee8[index].motionFrame == 0x134)
            st.state = BS004_STATE_PICK;
        if (DAT_10db6ee8[index].motionFrame < 300)
        {
            Matrix4 poseA;
            Matrix4 poseB;
            MakeHandPoses(index, poseA, poseB, BS004_BONE_FINGER_L, BS004_BONE_FINGER_R);
            StartHandBursts(index, poseA, poseB);
            if (DAT_10db6ee8[index].motionFrame > 0x122)
            {
                Vec4 where;
                StartSpreadBurst(index, poseB, where, BS004_SPREAD_WIDE, BS004_SPREAD_NARROW);
                StartSpreadBurst(index, poseB, where, BS004_SPREAD_WIDE, BS004_SPREAD_NARROW);
                StartSpreadBurst(index, poseB, where, BS004_SPREAD_NARROW, BS004_SPREAD_WIDE);
                StartSpreadBurst(index, poseB, where, BS004_SPREAD_NARROW, BS004_SPREAD_WIDE);
            }
        }
        break;
    }
    case BS004_STATE_RISE_A:
        // Climbs to BS004_RISE_LOW_TOP, drifting towards the player, then hands over to the plunge.
        st.motion = 2;
        moveMode = BS004_MOVE_FLY;
        st.frames++;
        DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        if (st.frames < BS004_RISE_SETTLE_FRAMES)
        {
            move.Fill(0.0f);
            move.z = -4.0f;
            BlendMove(index, move, move, 0.8f, 0.2f);
        }
        else
        {
            st.motion = 10;
            if (st.frames == BS004_RISE_SETTLE_FRAMES)
                PlanRise(index, move, ownPos, playerPos, BS004_RISE_LOW_TOP, 28.0f);
            BlendMove(index, move, DAT_10802bf8[index].step, 0.8f, 0.2f);
            if (DAT_10db6ee8[index].motionFrame == BS004_MOTION_FRAME_RISE_TOP && playerPos.z + 500.0f < ownPos.z)
            {
                st.state = BS004_STATE_PLUNGE_A;
                st.lastState = BS004_STATE_PLUNGE_A;
                st.frames = 0;
            }
        }
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        break;
    case BS004_STATE_PLUNGE_A:
    {
        // Drops on the spot below it: a short lift, then a fall at 2.4 per frame, the landing blast, a recoil and a fade out.
        DAT_10daaae8[index].stats[STAT_BUSY] = 0;
        st.frames++;
        st.motion = 0xb;
        moveMode = BS004_MOVE_FLY;
        if (st.frames == 1)
            PlanPlunge(index, move, ownPos, playerPos, BS004_RISE_LOW_TOP);
        if (st.frames < 0xf)
        {
            move.Copy(DAT_10802bf8[index].step);
            move.z = (float)(0xf - st.frames) * 1.6f;
            BlendMove(index, move, move, 0.7f, 0.3f);
        }
        else if (st.frames < 0x1e)
        {
            move.Copy(DAT_10802bf8[index].step);
            move.z = (float)(0xf - st.frames) * 2.4f;
            BlendMove(index, move, move, 0.7f, 0.3f);
            moveMode = BS004_MOVE_FLY;
            if (st.frames == 0x19)
            {
                FUN_10065b10(index, 0x2db, 1, 0);
                FUN_100b9bb0(index, 0x1fe, 0, 1);
            }
        }
        else if (st.frames < BS004_PLUNGE_LANDED && ownPos.z > 10.0f)
        {
            move.Copy(DAT_10802bf8[index].step);
            move.z = (float)(0x1e - st.frames) * 2.4f - 36.0f;
            BlendMove(index, move, move, 0.7f, 0.3f);
            moveMode = BS004_MOVE_FLY;
        }
        else if (st.frames < BS004_PLUNGE_LANDED)
            LandFromPlunge(index, move, ownPos, 0x2dc, 0x6e7);
        else
            FinishPlunge(index, st, move, 40.0f, false);
        if (st.frames < BS004_PLUNGE_DONE)
            SpinAndBurst(index, ownPos, 0.2f, false);
        break;
    }
    case BS004_STATE_RISE_B:
        // Like state 14 but up to BS004_RISE_HIGH_TOP, slower and with the voice line at the start of the climb.
        st.motion = 2;
        moveMode = BS004_MOVE_FLY;
        st.frames++;
        DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        DAT_10daaae8[index].stats[STAT_VALUE_4] = 5;
        if (st.frames < BS004_RISE_SETTLE_FRAMES)
        {
            move.Fill(0.0f);
            move.z = -2.0f;
            BlendMove(index, move, move, 0.8f, 0.2f);
            FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
            break;
        }
        if (st.frames == BS004_RISE_SETTLE_FRAMES)
            FUN_10065b10(index, 0x2dd, 1, 0);
        st.motion = 10;
        if (st.frames == BS004_RISE_SETTLE_FRAMES)
            PlanRise(index, move, ownPos, playerPos, BS004_RISE_HIGH_TOP, 56.0f);
        BlendMove(index, move, DAT_10802bf8[index].step, 0.8f, 0.2f);
        if (DAT_10db6ee8[index].motionFrame == BS004_MOTION_FRAME_RISE_TOP && st.frames > 0x5a && ownPos.z > 1000.0f)
        {
            st.state = BS004_STATE_PLUNGE_B;
            st.lastState = BS004_STATE_PLUNGE_B;
            st.frames = 0;
        }
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        break;
    case BS004_STATE_PLUNGE_B:
    {
        // Like state 15 from the high point: the screen is tinted while it falls, three shards per frame, a harder landing.
        FUN_1000ec70(0xf, 1.0f, 0.6f, 0.6f, 1.0f);
        int frames = st.frames;
        st.frames = frames + 1;
        int restoredValue = DAT_1020b48c[index].value;
        DAT_10daaae8[index].stats[STAT_BUSY] = 0;
        st.motion = 0xb;
        moveMode = BS004_MOVE_FLY;
        DAT_10daaae8[index].stats[STAT_VALUE_4] = restoredValue;
        if (frames + 1 == 1)
            PlanPlunge(index, move, ownPos, playerPos, BS004_RISE_HIGH_TOP);
        if (st.frames < 0xf)
        {
            move.Copy(DAT_10802bf8[index].step);
            move.z = (float)(0xf - st.frames) * 1.6f;
            BlendMove(index, move, move, 0.7f, 0.3f);
        }
        else if (st.frames == 0xf)
        {
            // Sometimes it hangs for one more frame (more often the longer it did not).
            int hangs = DAT_1020b490[index].value;
            if (FUN_1013f880() % 300 < 10 - hangs / 5)
            {
                st.frames = 0xe;
                DAT_1020b490[index].value = hangs + 1;
            }
            else
            {
                DAT_1020b490[index].value = 0;
                PlanPlunge(index, move, ownPos, playerPos, BS004_RISE_HIGH_TOP);
            }
        }
        else if (st.frames < 0x1e)
        {
            move.Copy(DAT_10802bf8[index].step);
            move.z = (float)(0xf - st.frames) * 2.4f;
            BlendMove(index, move, move, 0.7f, 0.3f);
            moveMode = BS004_MOVE_FLY;
            if (st.frames == 0x19)
            {
                FUN_10065b10(index, 0x2de, 1, 0);
                FUN_100b9bb0(index, 0x1f9, 0, 0);
            }
        }
        else if (st.frames < BS004_PLUNGE_LANDED && ownPos.z > 10.0f)
        {
            move.Copy(DAT_10802bf8[index].step);
            move.z = (float)(0x1e - st.frames) * 1.2f - 36.0f;
            BlendMove(index, move, move, 0.7f, 0.3f);
            moveMode = BS004_MOVE_FLY;
        }
        else if (st.frames < BS004_PLUNGE_LANDED)
            LandFromPlunge(index, move, ownPos, 0x2df, 0x6ec);
        else
            FinishPlunge(index, st, move, 80.0f, true);
        if (st.frames < BS004_PLUNGE_DONE)
            SpinAndBurst(index, ownPos, 0.6f, true);
        break;
    }
    case BS004_STATE_GROUND_BLAST:
        st.frames++;
        DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        st.motion = 0xf;
        step.Fill(0.0f);
        if (st.frames < 0x1e)
        {
            DAT_10daaae8[index].stats[STAT_BUSY] = 0;
            st.motion = 2;
            if (ownPos.z >= 650.0f)
                st.frames = 0x1d;
            else
            {
                step.z = 10.0f;
                st.frames = 0x1c;
            }
        }
        else if (st.frames < 0x3c)
        {
            if (st.frames == 0x3b)
            {
                EffectOptions options;
                SetUpOptions(index, options, 1.0f);
                options.flags |= 2;
                probe.Fill(0.0f);
                FUN_1001d8b0(index, 0x6fa, probe, -1, -1, &DAT_10db6ee8[index].root->Find("Bip01", 0)->worldMatrix, 0, 0, 0, &options);
            }
            else if (st.frames == 0x25)
                FUN_10065b10(index, 0x2e0, 1, 0);
        }
        else if (st.frames < 0x78)
        {
            step.z = -8.0f;
            StartBoneSpark(index);
        }
        else if (st.frames == 0x78)
            st.motion = 0x15;
        else if (DAT_10db6ee8[index].motionFrame == 0x1e0)
        {
            float pitch = FUN_10001f60(offset.z / (distance3D > 0.0f ? distance3D : 1.0f));
            EffectOptions options;
            SetUpOptions(index, options, 2.0f);
            if (pitch > 0.7854f)
                pitch = 0.7854f;
            pitch = pitch * -1.0f;
            Matrix4 pose = RotationX(pitch) * RotationZ(DAT_10db6ee8[index].heading);
            StartEffect(index, 0x6f3, ownPos, 0, &pose, &options);
            FUN_10065b10(index, 0x2e1, 1, 0);
            FUN_100b9bb0(index, 0x1f5, 0, 1);
            probe.Fill(0.0f);
            probe.y = -1.0f;
            probe.RotateX(pitch);
            probe.RotateZ(DAT_10db6ee8[index].heading);
            if (probe.z < -0.01f)
            {
                probe.Copy(probe * (ownPos.z / probe.z));
                probe.Copy(probe + ownPos);
                probe.z = 0.0f;
                pose.SetTranslation(probe);
                StartEffect(index, 0x6f6, probe, 0, &pose, &options);
            }
        }
        else if (DAT_10db6ee8[index].motionFrame == 0x1fd)
        {
            st.state = BS004_STATE_PICK;
            DAT_10daaae8[index].stats[STAT_BUSY] = 0;
        }
        if (st.frames < 0x79)
        {
            if (st.frames < 0x73)
                FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        }
        else
            st.motion = 0x15;
        BlendMove(index, move, step, 0.9f, 0.1f);
        break;
    case BS004_STATE_FIRE_RAIN:
        st.motion = 0x10;
        step.Fill(0.0f);
        moveMode = BS004_MOVE_WALK;
        st.frames++;
        step.z = (600.0f - ownPos.z) / 20.0f;
        BlendMove(index, move, step, 0.9f, 0.1f);
        if (st.frames == 0)
        {
            FUN_10065b10(index, 0x2e2, 1, 0);
            FUN_100b9bb0(index, 0x1f8, 0, 1);
        }
        else if (st.frames > 0x18 && st.frames % 7 == 0 && FUN_1013f880() % 0x1e < 0x14)
        {
            EffectOptions options;
            Matrix4 pose = RotationZ(DAT_10db6ee8[index].heading);
            SetUpOptions(index, options, 0.75f);
            probe.Fill(0.0f);
            probe.y = -150.0f;
            probe.x = (float)(FUN_1013f880() % 600 - 300);
            probe.z = (float)(FUN_1013f880() % 600 - 300);
            probe.RotateX((float)(FUN_1013f880() % 60 - 30) / 180.0f * BS004_PI);
            probe.RotateZ(((float)(FUN_1013f880() % 60 - 30) / 180.0f) * BS004_PI + DAT_10db6ee8[index].heading);
            probe.Copy(probe + ownPos);
            pose.SetTranslation(probe);
            StartEffect(index, 0x6fc, probe, 0, &pose, &options);
            FUN_1003ad20(0x2e3, &ownPos, 1.0f);
        }
        if (st.frames > 0x96 && (st.frames >> 1) < FUN_1013f880() % st.frames)
            st.state = BS004_STATE_PICK;
        if (st.frames % 3 == 0)
            StartBoneSpark(index);
        FacePlayer(DAT_10db6ee8[index], ownPos, playerPos);
        break;
    case BS004_STATE_PICK:
    {
        // Picks the next state: the odds of each attack depend on whether it is angry (health below half), how often
        // it picked an attack in a row, and what it did last.
        int picked = DAT_1020b484[index].value;
        int angry = DAT_1020b47c[index].value;
        int handled = 0;
        bool picksAttack;
        DAT_1020b470[index].value = 0;
        if (angry == 0)
        {
            int divisor = 6 - picked;
            if (divisor < 2)
                divisor = 2;
            picksAttack = FUN_1013f880() % divisor == 0;
            if (!picksAttack && picked > 2)
                picksAttack = FUN_1013f880() % 100 < (picked * picked * picked) / 2;
        }
        else
        {
            int divisor = 0xc - picked;
            if (divisor < 3)
                divisor = 3;
            picksAttack = FUN_1013f880() % divisor == 0;
            if (!picksAttack && picked > 5)
                picksAttack = FUN_1013f880() % 100 < picked * picked;
        }
        switch (st.lastState)
        {
        case 0:
            DAT_1020b494[index].value = 0;
            if (FUN_1013f880() % 2 != 0)
            {
                st.state = BS004_STATE_FOLLOW_PATH;
                handled = 1;
            }
            else
                picksAttack = true;
            break;
        case 1:
            if (!picksAttack)
            {
                if (st.pathPoint < 0x12)
                    st.state = BS004_STATE_DRIFT;
                else
                {
                    st.previousPoint = st.pathPoint;
                    st.state = BS004_STATE_FOLLOW_PATH;
                    st.pathPoint = FUN_1013f880() % 0x11;
                }
                handled = 1;
            }
            break;
        case 2:
            st.state = BS004_STATE_RECOIL;
            DAT_1020b494[index].value = 0;
            handled = 1;
            break;
        case 3:
            DAT_1020b494[index].value = 0;
            st.state = BS004_STATE_HOVER;
            handled = 1;
            break;
        case 6:
            st.state = BS004_STATE_APPROACH_POINT_B;
            handled = 1;
            break;
        case 10:
            picksAttack = true;
            break;
        default:
            if (st.requested == 1)
            {
                st.state = BS004_STATE_FOLLOW_PATH;
                st.requested = 0;
                handled = 1;
            }
            else if (st.requested == 2)
            {
                st.state = BS004_STATE_APPROACH_POINT_B;
                st.requested = 0;
                handled = 1;
            }
            else if (!picksAttack)
            {
                st.state = BS004_STATE_FOLLOW_PATH;
                handled = 2;
            }
            break;
        }
        if (handled == 0 || picksAttack)
        {
            // The attacks in the order of their cumulative weights.
            static const int attacks[8] = {8, 7, 0xb, 0xd, 0xe, 0x10, 0x12, 0x13};
            int weight[7];
            int hard = DAT_1020b47c[index].value != 0;
            int scale = hard ? 2 : 1;
            int roarWeight = hard ? 0x96 : 200;
            int diveWeight = hard ? 0x96 : 100;
            int slamWeight = hard ? 0x96 : 100;
            int riseAWeight = hard ? 0x32 : 0;
            int riseBWeight = hard ? 0x28 : 0;
            int blastWeight = hard ? 0x4b : 0;
            int plungeWeight = hard ? 100 : 10;
            int fireWeight = 100;
            DAT_1020b484[index].value++;
            switch (st.lastState)
            {
            case 7:
                diveWeight += scale * 300;
                roarWeight += scale * 200;
                break;
            case 9:
                fireWeight = scale * 200 + 100;
                break;
            case 0xc:
                fireWeight = scale * 500 + 100;
                break;
            case 0xd:
                plungeWeight += scale * 100;
                break;
            case 0xf:
                roarWeight += scale * 100;
                break;
            case 0x11:
                blastWeight += scale * 200;
                break;
            case 0x12:
                blastWeight += scale * 200;
                fireWeight = scale * 200 + 100;
                break;
            case 0x13:
                riseAWeight += scale * 100;
                roarWeight += scale * 200;
                break;
            }
            weight[0] = roarWeight;
            weight[1] = weight[0] + riseAWeight;
            weight[2] = diveWeight + weight[1];
            weight[3] = fireWeight + weight[2];
            weight[4] = weight[3] + 0x32;
            weight[5] = riseBWeight + weight[4];
            weight[6] = slamWeight + weight[5];
            int total = weight[6] + plungeWeight + blastWeight;
            int roll = FUN_1013f880() % total;
            int pick = 0;
            do
            {
                if (roll < weight[pick])
                    break;
                pick++;
            } while (pick < 7);
            st.state = attacks[pick];
        }
        if (DAT_1020b480[index].value != 0)
        {
            st.state = DAT_1020b480[index].value;
            DAT_1020b480[index].value = 0;
        }
        st.frames = 0;
        if (st.minionsDown > 2 && st.state != BS004_STATE_RECOIL && st.lastState != BS004_STATE_RECOIL)
            st.state = BS004_STATE_PICK_POINT;
        st.lastState = st.state;
        if (st.state == BS004_STATE_FOLLOW_PATH && FUN_1013f880() % 1000 < 0x96)
            st.state = BS004_STATE_APPROACH_POINT_B;
        if ((st.state == BS004_STATE_FOLLOW_PATH && handled == 2) || st.state == BS004_STATE_APPROACH_POINT_B)
        {
            // Goes to one of the two path points nearest to where it moves.
            int nearest = -1;
            int second = -1;
            float nearestDistance = 0.0f;
            float secondDistance = 0.0f;
            for (int point = 0; point < 0x11; point++)
            {
                float d = (path->frameMatrices[point].GetTranslation() - ownPos).LengthSquared3();
                if (nearest == -1 || d < nearestDistance)
                {
                    second = nearest;
                    secondDistance = nearestDistance;
                    nearest = point;
                    nearestDistance = d;
                }
                else if (second == -1 || d < secondDistance)
                {
                    second = point;
                    secondDistance = d;
                }
            }
            st.previousPoint = st.pathPoint;
            st.pathPoint = FUN_1013f880() % 2 == 0 ? second : nearest;
        }
        else if (st.state == BS004_STATE_FOLLOW_PATH && handled == 1)
        {
            st.previousPoint = st.pathPoint;
            st.pathPoint = FUN_1013f880() % 0x11;
        }
        if (st.state == BS004_STATE_HOVER)
            DAT_1020b484[index].value = 0;
        break;
    }
    }

    // Start the motion the state asked for.
    if (st.motion > -1 && st.motion != st.playingMotion)
    {
        DAT_10db6ee8[index].PlayMotion(st.motion, 0, 3, 1.0f);
        st.playingMotion = st.motion;
    }
    DAT_10802bf8[index].position.Copy(move);
    if (moveMode == BS004_MOVE_WALK)
    {
        FUN_100241e0(index, move.x * DAT_10db6ee8[index].speedScale, move.y * DAT_10db6ee8[index].speedScale, move.z * DAT_10db6ee8[index].speedScale);
    }
    else if (moveMode == BS004_MOVE_FLY)
    {
        Vec4 shrunk = move;
        int tries = 0x1e;
        do
        {
            probe.Copy(ownPos + shrunk);
            if (DAT_10db6ee8[index].IsClearOfMapLimited(&shrunk) != 0 && DAT_10255f08 != 0 &&
                DAT_10255f08->CastSegment(0, 4, &probe, &ownPos, &target, 0) == 0)
            {
                if (shrunk.Length2D() < 2000.0f)
                    break;
            }
            tries--;
            shrunk.x *= 0.9f;
            shrunk.y *= 0.9f;
            shrunk.z *= 0.95f;
        } while (tries != 0);
        probe.Copy(ownPos + shrunk);
        float height = DAT_10802c08[index][0].z + probe.z;
        if (tries != 0 || (height > 0.0f && height < 1000.0f) || height < 0.0f)
            DAT_10db6ee8[index].SetPosition(probe.x, probe.y, probe.z);
        turn = 0;
    }

    // Count the minions that are down and standing, and note when none changed.
    int standing = 0;
    for (int minion = index + BS004_MINION_FIRST_OFFSET; minion < index + 5; minion++)
    {
        int &seen = DAT_1020b430[minion].words[4];
        if (seen == 0)
        {
            if (DAT_10210d30[minion] > 0)
            {
                seen = 1;
                st.minionsDown++;
            }
            if (seen == 0 && DAT_10db6ee8[minion].matrix._41 > -50000.0f)
                standing++;
        }
    }
    DAT_1020b488[index].value++;
    if (st.minionsAlive != standing)
        DAT_1020b488[index].value = 0;
    st.minionsAlive = standing;
    st.hitLevel = DAT_102550e0[index];
    if (DAT_10db6ee8[index].motion->blending == 0)
    {
        st.motionLayer = DAT_10db6ee8[index].motion->layer;
        st.motionFrame = DAT_10db6ee8[index].motion->frame;
    }
    else
    {
        st.motionLayer = DAT_10db6ee8[index].motion->nextLayer;
        st.motionFrame = -1;
    }

    // Turn towards the target heading, at most a third (2.5 while it flies) of the angle per frame.
    if (turn != 0)
    {
        float wanted = WrapAngle(DAT_10db6ee8[index].targetHeading);
        float difference = WrapAngle(WrapAngle(DAT_10db6ee8[index].heading) - wanted);
        if (difference != 0.0f && st.state != BS004_STATE_APPROACH_POINT)
            difference = difference / 2.5f;
        DAT_10db6ee8[index].targetHeading = DAT_10db6ee8[index].heading - difference;
    }

    // Footstep and flap sounds on the frames of its motions.
    switch (DAT_10db6ee8[index].motionFrame)
    {
    case 0x219:
    case 0x230:
    case 0x242:
    case 0x256:
    case 0x276:
    case 0x28a:
    case 0x1ae:
    case 0x6e:
    case 10:
    case 0x26:
    case 0x4c:
    case 0x82:
    case 0x14a:
    case 0x197:
        FUN_10065b10(index, 0x2e4, 1, 0);
        break;
    }
    return 0;
}
