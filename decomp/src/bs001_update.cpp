// The BS_001 boss (kind 100): a fighter that walks, runs, lunges, punches, claws and smashes at the player, with an
// optional partner slot that copies its animation. Each frame it fades itself and its partner in and out, reacts to
// hits, runs the defeat scene (the four piece models fly apart while the camera swings round), picks its next move
// by chance and runs the move it is in, then spawns the footstep and hit effects of the motion it plays.

#include <math.h>
#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "char_slot.h"
#include "char_state.h"
#include "effect_manager.h"
#include "texture_manager.h"
#include "game.h"
#include "bs001.h"
#include "bs003.h"
#include "hop_spit_enemy.h"
#include "partner_follow.h"
#include "doppler_loops.h"
#include "target_race.h"
#include "route_flier.h"
#include "dodge_golem.h"
#include "game_main.h"
#include "texture_load.h"
#include "value_command.h"
#include "camera_vars.h"
#include "render_vars.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "scene_reset.h"
#include "script_system.h"
#include "world.h"
#include "boss_voice.h"
#include "last_position.h"

#define BS001_PI 3.1415f
#define BS001_TWO_PI 6.283f

// Fade counters (Bs001Fade): frames of the fade out and of the fade in; the model is gone at the middle.
#define BS001_FADE_MIDDLE 15
#define BS001_FADE_END 30
// The fall below which the boss counts as lost and fades back in at its partner's side.
#define BS001_LOST_HEIGHT -3400.0f
#define BS001_LOST_LIMIT -10000.0f
// Where a model waits while it is hidden.
#define BS001_HIDDEN_X -200000.0f
// Height the faded-out models wait at.
#define BS001_FADE_PARK_Z 600.0f
#define BS001_FADE_PARK_PARTNER_Z 1200.0f

// Voice cooldowns (frames the neighbouring boss keeps quiet after a line).
#define BS001_VOICE_COOLDOWN_FADE 0x2d
#define BS001_VOICE_COOLDOWN_STAND 0x2d
#define BS001_VOICE_COOLDOWN_SHOUT 0x1e
#define BS001_VOICE_GETS_UP 0x1e5
// State 4 is not used: it falls through to the move chooser.
#define BS001_IDLE_UNUSED 4
// Node flag toggled on the door nodes "@mp_105t1" / "@mp_105t2" while the boss rises.
#define BS001_DOOR_FLAG 0x20
// Script flag that tells how far the story is (the rise effects run faster at 2).
#define BS001_STAGE_FLAG 0x1c7
// Where the boss waits while it rises.
#define BS001_RISE_PARK_X -60000.0f

// Motions the boss plays (Bs001State::motion).
#define BS001_MOTION_WALK 0
#define BS001_MOTION_RUN 1
#define BS001_MOTION_IDLE 2
#define BS001_MOTION_TURN 3
#define BS001_MOTION_STOMP 4
#define BS001_MOTION_LUNGE 5
#define BS001_MOTION_SHOUT 10
#define BS001_MOTION_FALL 0xb
#define BS001_MOTION_DOWN 0x13
#define BS001_MOTION_SMASH 0x14
#define BS001_MOTION_HURT 0x19
#define BS001_MOTION_HIT 0x1a

// Voices (Enemy_PlayVoice).
#define BS001_VOICE_FADE 0x1e4
#define BS001_VOICE_PAIN_FIRST 0x1e6
#define BS001_VOICE_PAIN_COUNT 5
#define BS001_VOICE_SHOUT 500
#define BS001_VOICE_DEFEATED 0x1eb
#define BS001_VOICE_STAND 0x1ec
#define BS001_VOICE_RISE 0x1ed
#define BS001_VOICE_FALL_FIRST 0x1ee
#define BS001_VOICE_RETREAT 0x1f0
#define BS001_VOICE_PUNCH 0x1f1
#define BS001_VOICE_CLAW 0x1f2
#define BS001_VOICE_SMASH 499

// Sound effects (Character_PlaySound).
#define BS001_SOUND_DEFEAT_START 0x2fd
#define BS001_SOUND_DEFEAT_BLAST 0x2fe
#define BS001_SOUND_STOMP 0x2ff
#define BS001_SOUND_SHOUT_RING 0x300
#define BS001_SOUND_SHOUT 0x301
#define BS001_SOUND_PUNCH 0x302
#define BS001_SOUND_CLAW 0x303
#define BS001_SOUND_SMASH_HIT 0x304
#define BS001_SOUND_STEP_FIRST 0x305
#define BS001_SOUND_STEP_SECOND 0x306
#define BS001_SOUND_SMASH_END 0x307

// Effects.
#define BS001_EFFECT_FOOTSTEP 0x2de
#define BS001_EFFECT_SHOUT_RING 0x611
#define BS001_EFFECT_SHOUT_SPARK 0x19
#define BS001_EFFECT_SHOUT_SPARK_OUTER 0x1d
#define BS001_EFFECT_SHOUT_CLOUD 0x271a
#define BS001_EFFECT_STOMP_L 0x613
#define BS001_EFFECT_STOMP_R 0x614
#define BS001_EFFECT_PUNCH_FLASH 0x60e
#define BS001_EFFECT_PUNCH_TRAIL 0x5ef
#define BS001_EFFECT_CLAW_FLASH 0x661
#define BS001_EFFECT_SMASH_DUST 0x454
#define BS001_EFFECT_SMASH_FLASH 0x44e
#define BS001_EFFECT_TRAIL_STANDARD 0x664
#define BS001_EFFECT_TRAIL_SMASH 0x455
#define BS001_EFFECT_ATTACH 5
#define BS001_EFFECT_RISE_DUST 0x160
#define BS001_EFFECT_DEFEAT_SMOKE_A 0x321
#define BS001_EFFECT_DEFEAT_SMOKE_B 0x322
#define BS001_EFFECT_BLOW 0x960

// Frame ranges of the smash animation (motion frame of the model).
#define BS001_SMASH_FRAME_FIRST 0x234
#define BS001_SMASH_FRAME_LAST 0x24e
#define BS001_SMASH_END_FRAME 0x289

// Defeat scene (Bs001State::defeatTimer counts its frames).
#define BS001_DEFEAT_FADE_FRAMES 0x5a
#define BS001_DEFEAT_FLASH_FRAMES 0x3c
#define BS001_DEFEAT_BLAST_FRAME 0x72
#define BS001_DEFEAT_END_FRAME 0xa8
#define BS001_DEFEAT_SCENE_END 0xd2
#define BS001_DEFEAT_SPLIT_FIRST 0x2a
#define BS001_DEFEAT_FILTER_FRAME 0xb4
#define BS001_DEFEAT_FINISH_FRAME 0xc3
#define BS001_DEFEAT_DUMMY_FRAME 0x1e
#define BS001_DEFEAT_CAMERA_DELAY 0xa0
#define BS001_DEFEAT_START_VALUE -30
// Where the defeat camera sits relative to the boss (before it is turned): to the side and behind.
#define BS001_CAMERA_SIDE 950.0f
#define BS001_CAMERA_BACK 200.0f

static inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

static inline Node *Bs001BoneNode(int which)
{
    return (Node *)DAT_1042d918[which];
}

// Brings `angle` into -pi..pi.
static __forceinline float WrapAngle(float angle)
{
    while (angle > BS001_PI)
        angle -= BS001_TWO_PI;
    while (angle < -BS001_PI)
        angle += BS001_TWO_PI;
    return angle;
}

// Starts effect `effect` for the boss at `position`, attached to `owner` (a node's world matrix) with the given
// matrix and options.
static __forceinline void BossEffect(int index, int effect, const Vec4 &position, const Matrix4 *owner, const Matrix4 *matrix,
                       const EffectOptions *options)
{
    FUN_1001d8b0(index, effect, position, -1, -1, owner, 0, 0, matrix, options);
}

// Starts effect `effect` at the world matrix of bone `bone` (if the boss has it) with the boss's effect options.
static __forceinline void BoneEffect(int index, int effect, const Vec4 &position, Node *bone, const EffectOptions *options)
{
    if (bone != 0)
        FUN_1001d8b0(index, effect, position, -1, -1, 0, 0, 0, &bone->worldMatrix, options);
}

// Hides model `model` far away.
static __forceinline void HideModel(AnimModel &model)
{
    model.SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
}

// Turns the boss's goal heading towards `direction` (x/y of a vector from the boss to somewhere).
static __forceinline void FaceDirection(AnimModel &model, const Vec4 &direction)
{
    model.targetHeading = (float)atan2(direction.x, direction.y) + BS001_PI;
}

// Pushes the boss forwards (along its heading) by the 2D length of `direction`, unless enemies are frozen.
static __forceinline void PushForwards(AnimModel &model, Vec4 &direction)
{
    direction.y = -direction.Length2D();
    direction.x = 0.0f;
    direction.RotateZ(-model.heading);
    model.Push(direction.x, direction.y, 0.0f);
}

// Starts the attach effect (a flash bound to the bone) on `bone`, if the boss has it.
static __forceinline void AttachEffect(int index, const Vec4 &position, Node *bone)
{
    if (bone != 0)
        BossEffect(index, BS001_EFFECT_ATTACH, position, &bone->worldMatrix, 0, 0);
}

// Starts a weapon trail effect that follows `node`, if the boss has it.
static __forceinline void TrailEffect(int index, int effect, const Vec4 &position, Node *node, const EffectOptions *options)
{
    if (node != 0)
        BossEffect(index, effect, position, &node->worldMatrix, 0, options);
}

// A footstep: the dust effect at the foot bone and its sound.
static __forceinline void Footstep(int index, Node *foot, const Vec4 &effectPos, Matrix4 &boneMatrix)
{
    if (foot != 0)
    {
        boneMatrix = foot->worldMatrix;
        BossEffect(index, BS001_EFFECT_FOOTSTEP, effectPos, 0, &boneMatrix, 0);
        Vec4 stepPosition = boneMatrix.GetTranslation();
        FUN_10054550(BS001_SOUND_STEP_FIRST, 5, &stepPosition);
    }
}

// A footstep of the claw chase motions: the dust effect and a positional sound loop effect.
static __forceinline void StepSound(int index, Node *foot, const Vec4 &effectPos, Matrix4 &boneMatrix)
{
    if (foot != 0)
    {
        boneMatrix = foot->worldMatrix;
        BossEffect(index, BS001_EFFECT_FOOTSTEP, effectPos, 0, &boneMatrix, 0);
        Vec4 stepPosition = boneMatrix.GetTranslation();
        FUN_1003ad20(BS001_SOUND_STEP_FIRST, &stepPosition, 1.0f);
    }
}

// 0x1010c650 (Bs001_Update; C++ linkage). One frame of the BS_001 boss in slot `index`.
int Bs001_Update(int index)
{
    AnimModel &model = DAT_10db6ee8[index];
    AnimModel &partnerModel = DAT_10db6ee8[DAT_10daaae8[index].m_00a4];
    int &fadeTrigger = DAT_10daaae8[index].stats[STAT_PICKUPS];
    int restartMotion = 0;
    int quickBlend = 0;
    int syncFlag = 0;
    int hardMode = DAT_10c635f8 == 2;
    Vec4 moveDir;
    Vec4 selfPos;
    Vec4 temp;

    if (model.matrix._43 < BS001_LOST_HEIGHT && model.matrix._41 > BS001_LOST_LIMIT)
        fadeTrigger = 100;
    if (fadeTrigger == 100 && DAT_1020b430[index + 2].bs001Fade.fadeOut == 0)
    {
        DAT_1020b430[index + 2].bs001Fade.fadeOut = 1;
        if (DAT_10c66cc0 != 0)
            BossSayLine(index, BS001_VOICE_FADE, BS001_VOICE_COOLDOWN_FADE);
    }
    if (DAT_10daaae8[DAT_10daaae8[index].m_00a4].stats[STAT_PICKUPS] == 100)
    {
        if (DAT_10daaae8[index].m_00a8 == 0)
        {
            if (DAT_1020b430[index + 2].bs001Fade.fadeIn == 0)
                DAT_1020b430[index + 2].bs001Fade.fadeIn = 1;
        }
        else
            DAT_10daaae8[DAT_10daaae8[index].m_00a4].stats[STAT_PICKUPS] = 0;
    }
    if (DAT_1020b430[index + 2].bs001Fade.fadeOut != 0)
    {
        if (DAT_1020b430[index + 2].bs001Fade.fadeOut < BS001_FADE_MIDDLE)
        {
            float step = (float)(BS001_FADE_MIDDLE - DAT_1020b430[index + 2].bs001Fade.fadeOut);
            DAT_1020b430[index + 2].bs001Fade.fadeOut++;
            model.alpha = step / 15.0f;
        }
        else if (DAT_1020b430[index + 2].bs001Fade.fadeOut == BS001_FADE_MIDDLE)
        {
            model.alpha = 0.0f;
            model.SetPosition(0.0f, 0.0f, BS001_FADE_PARK_Z);
            DAT_1020b430[index + 2].bs001Fade.fadeOut++;
        }
        else if (DAT_1020b430[index + 2].bs001Fade.fadeOut < BS001_FADE_END)
        {
            float step = (float)(DAT_1020b430[index + 2].bs001Fade.fadeOut - BS001_FADE_MIDDLE);
            DAT_1020b430[index + 2].bs001Fade.fadeOut++;
            model.alpha = step / 15.0f;
        }
        else
        {
            DAT_1020b430[index + 2].bs001Fade.fadeOut = -1;
            int next = DAT_1020b430[index + 2].bs001Fade.fadeOut;
            model.alpha = 1.0f;
            fadeTrigger = 0;
            DAT_1020b430[index + 2].bs001Fade.fadeOut = next + 1;
        }
    }
    if (DAT_1020b430[index + 2].bs001Fade.fadeIn != 0)
    {
        if (DAT_1020b430[index + 2].bs001Fade.fadeIn < BS001_FADE_MIDDLE)
        {
            float step = (float)(BS001_FADE_MIDDLE - DAT_1020b430[index + 2].bs001Fade.fadeOut);
            partnerModel.alpha = step / 15.0f;
        }
        else if (DAT_1020b430[index + 2].bs001Fade.fadeIn == BS001_FADE_MIDDLE)
        {
            partnerModel.alpha = 0.0f;
            partnerModel.SetPosition(0.0f, 0.0f, BS001_FADE_PARK_PARTNER_Z);
        }
        else if (DAT_1020b430[index + 2].bs001Fade.fadeIn < BS001_FADE_END)
        {
            float step = (float)(DAT_1020b430[index + 2].bs001Fade.fadeOut - BS001_FADE_MIDDLE);
            partnerModel.alpha = step / 15.0f;
        }
        else
        {
            partnerModel.alpha = 1.0f;
            DAT_1020b430[index + 2].bs001Fade.fadeIn = -1;
            DAT_10daaae8[DAT_10daaae8[index].m_00a4].stats[STAT_PICKUPS] = 0;
        }
        DAT_1020b430[index + 2].bs001Fade.fadeIn++;
    }
    if (DAT_1020b430[index + 1].bs001Aux.partnerCooldown == 0 && DAT_10daaae8[index].m_00a8 == 0)
        syncFlag = 1;
    if (DAT_1020b430[index + 1].bs001Aux.partnerCooldown != 0 && DAT_10daaae8[index].m_00a8 != 0)
        syncFlag = 2;
    int partnerMode = DAT_1020b430[index].bs001.initialised;
    DAT_11b04510 = 1;
    DAT_10219e40[index] = (char)0xff;
    DAT_104c0110 = index;
    DAT_1021f6c0[index] = 0;
    DAT_1020b430[index].bs001.motion = -1;
    model.uniformScale = 0.15f;
    if (partnerMode != 0)
    {
        moveDir.Copy(model.matrix.GetTranslation());
        temp.Copy(moveDir);
        temp.w = -40.0f;
        temp.RotateZ(model.heading);
        moveDir.Copy(temp + moveDir);
        selfPos.Copy(moveDir);
        selfPos.z += 30.0f;
        selfPos.Fill(0.0f);
        model.matrix.SetTranslation(selfPos);
    }
    if (DAT_104bfdf0 != 0 || DAT_104c0135 != 0 || DAT_104bfe1c != 0)
    {
        model.m_0294 = 1;
        if (DAT_10daaae8[index].m_00a8 != 0)
            partnerModel.m_0294 = 1;
        DAT_1020b430[index].bs001.cutsceneHeld = 1;
        return 0;
    }
    if (DAT_1020b430[index].bs001.cutsceneHeld != 0)
    {
        model.m_0294 = 0;
        DAT_1020b430[index].bs001.cutsceneHeld = 0;
        if (DAT_10daaae8[index].m_00a8 != 0)
            partnerModel.m_0294 = 0;
    }
    if (DAT_102550e0[index] == 0)
    {
        DAT_1020b430[index + 1].bs001Aux.lastHitLevel = 0;
        if (DAT_1020b430[index + 1].bs001Aux.m_0004 > 0)
            ;
        if (DAT_1020b430[index].bs001.hurtLevel > 0)
            DAT_1020b430[index].bs001.hurtLevel--;
    }
    else
    {
        int state = DAT_1020b430[index].bs001.state;
        if (state == BS001_STAGGER || state == BS001_DOWN || state == BS001_HURT_STAGGER)
        {
            DAT_102550e0[index] = 0;
            model.PlayMotion(BS001_MOTION_HIT, 1, 5, 1.0f);
            if (DAT_10daaae8[index].m_00a8 != 0)
                partnerModel.PlayMotion(BS001_MOTION_HIT, 1, 5, 1.0f);
            DAT_1020b430[index + 1].bs001Aux.lastHitLevel = DAT_102550e0[index];
        }
        else if ((int)DAT_102550e0[index] <= DAT_1020b430[index + 1].bs001Aux.lastHitLevel)
            DAT_1020b430[index + 1].bs001Aux.lastHitLevel = DAT_102550e0[index];
        else
        {
            DAT_1020b430[index].bs001.hurtLevel += 10;
            if (DAT_10c635f8 == 1)
            {
                unsigned char value = (unsigned char)((11 - DAT_1020b430[index].bs001.hurtLevel) * 4);
                unsigned int level = 0x78;
                if (value > 0x77)
                    level = value;
                DAT_102550e0[index] = (unsigned char)(level / 10);
            }
            else if (DAT_10c635f8 == 0)
            {
                unsigned char value = (unsigned char)(DAT_1020b430[index].bs001.hurtLevel * -10 + 0x2c);
                unsigned int level = 0x1e;
                if (value >= 0x1e)
                    level = value;
                DAT_102550e0[index] = (unsigned char)(level / 10);
            }
            else if (DAT_10c635f8 == 2)
            {
                unsigned int level = (unsigned char)(DAT_1020b430[index].bs001.hurtLevel * -0x14 + 0x2c);
                DAT_102550e0[index] = (unsigned char)(level / 10);
            }
            if (DAT_10daaae8[index].m_00a8 == 0)
            {
                if (DAT_10daaae8[index].stats[STAT_HEALTH] > 0)
                {
                    int pain = FUN_1013f880();
                    FUN_100b9bb0(index, pain % BS001_VOICE_PAIN_COUNT + BS001_VOICE_PAIN_FIRST, 0, 0);
                }
                DAT_1020b430[index + 1].bs001Aux.lastHitLevel = DAT_102550e0[index];
            }
            else
            {
                Playback *playback = model.motion;
                if (playback->blending != 0)
                    partnerModel.PlayMotion(playback->nextLayer, 1, 2, 1.0f);
                else
                    partnerModel.PlayMotion(playback->layer, 1, 0, 1.0f);
                partnerModel.motion->m_0004 = (float)model.motionFrame;
                partnerModel.motion->layer = model.motion->layer;
                DAT_1020b430[index + 1].bs001Aux.lastHitLevel = DAT_102550e0[index];
            }
        }
    }

    moveDir.Copy(DAT_10db6ee8[PLAYER_SLOT].matrix.GetTranslation());
    selfPos.Copy(model.matrix.GetTranslation());
    (selfPos - moveDir).Length2D();
    float distance = (selfPos - moveDir).Length();
    if (DAT_1020b430[index + 1].bs001Aux.partnerCooldown != 0)
        DAT_1020b430[index + 1].bs001Aux.partnerCooldown--;
    if (DAT_1020b430[index + 1].bs001Aux.knockActive != 0)
    {
        partnerModel.mode = 0;
        DAT_1020b430[index + 1].bs001Aux.knockZ -= 200;
        if (DAT_1020b430[index + 1].bs001Aux.knockZ < -0xdac)
            DAT_1020b430[index + 1].bs001Aux.knockZ = -0xdac;
        Vec4 knock;
        temp.Copy(partnerModel.matrix.GetTranslation());
        knock.x = (float)DAT_1020b430[index + 1].bs001Aux.knockX / 100.0f;
        knock.y = (float)DAT_1020b430[index + 1].bs001Aux.knockY / 100.0f;
        knock.z = (float)DAT_1020b430[index + 1].bs001Aux.knockZ / 100.0f;
        if (temp.z + knock.z < 10.0f && DAT_1020b430[index + 2].bs001Fade.fadeIn == 0)
        {
            Vec4 bounce;
            bounce.z = knock.z;
            if (partnerModel.IsClearOfMapLimited(&bounce) == 0)
            {
                DAT_1020b430[index + 1].bs001Aux.knockZ = (int)((float)DAT_1020b430[index + 1].bs001Aux.knockZ);
                knock.z = knock.z * -0.5f;
                DAT_1020b430[index + 1].bs001Aux.knockX = (int)((float)DAT_1020b430[index + 1].bs001Aux.knockX);
                DAT_1020b430[index + 1].bs001Aux.knockY = (int)((float)DAT_1020b430[index + 1].bs001Aux.knockY);
                if (knock.z < 4.0f)
                {
                    DAT_1020b430[index + 1].bs001Aux.knockActive = 0;
                    partnerModel.mode = 1;
                    knock.z = 0.0f;
                }
            }
        }
        if (partnerModel.IsClearOfMapLimited(&knock) == 0)
        {
            knock.x = knock.x * -1.0f;
            DAT_1020b430[index + 1].bs001Aux.knockY = -DAT_1020b430[index + 1].bs001Aux.knockY;
            knock.y = knock.y * -1.0f;
            DAT_1020b430[index + 1].bs001Aux.knockX = -DAT_1020b430[index + 1].bs001Aux.knockX;
            while (partnerModel.IsClearOfMapLimited(&knock) == 0)
            {
                Vec4 away;
                away.Copy(partnerModel.matrix.GetTranslation() * -1.0f);
                away.Copy(away.Normalized3());
                away.Copy(away * 15.0f);
                knock.x = away.x + knock.x;
                knock.y = away.y + knock.y;
                if ((knock + temp).Length2D() < 100.0f)
                    break;
            }
        }
        temp.Copy(knock + partnerModel.matrix.GetTranslation());
        partnerModel.SetPosition(temp.x, temp.y, temp.z);
    }

    if (DAT_10210d30[index] != 0)
    {
        // The defeat scene: the boss and its pieces fly apart while the camera swings round.
        int roomFlagWord = DAT_10c63ffc * 2 + 1000;
        DAT_10db6ee8[PLAYER_SLOT].shadowHidden = 0;
        DAT_10daaae8[index].stats[3] = 0;
        DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        DAT_104c0354 = 2;
        DAT_10daaae8[index].m_0141 = 1;
        unsigned int roomFlags = DAT_10874848.TestScriptFlag(roomFlagWord);
        DAT_10874848.SetScriptFlag(roomFlagWord, roomFlags | (1 << ((index - 0x3a) & 0x1f)));
        if (DAT_1020b430[index].bs001.defeatTimer < 0x1e)
        {
            if (DAT_1020b430[index].bs001.defeatTimer % 10 == 0 || FUN_1013f880() % 100 == 0)
            {
                Vec4 smokePos = model.matrix.GetTranslation();
                if ((FUN_1013f880() & 1) == 0)
                    FUN_1001d8b0(index, BS001_EFFECT_DEFEAT_SMOKE_A, smokePos, -1, -1, 0, 0, 0, 0, 0);
                else
                    FUN_1001d8b0(index, BS001_EFFECT_DEFEAT_SMOKE_B, smokePos, -1, -1, 0, 0, 0, 0, 0);
            }
        }
        else if (DAT_1020b430[index].bs001.defeatTimer == BS001_DEFEAT_DUMMY_FRAME)
        {
            FUN_1013fb10(3, 200, 30);
            FUN_100b9bb0(index, BS001_VOICE_DEFEATED, 1, 0);
        }
        if (DAT_1020b430[index].bs001.defeatTimer == BS001_DEFEAT_FILTER_FRAME)
        {
            FUN_1000db30(0, 0, 0);
            FUN_1000db30(0, 100, 20);
        }
        else if (DAT_1020b430[index].bs001.defeatTimer == BS001_DEFEAT_FINISH_FRAME)
        {
            FUN_1013fb10(3, 0, 10);
            DAT_10874848.MarkKilled(DAT_10daaae8[index].routeRecord);
            model.SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            DAT_10db6ee8[index + 1].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            DAT_10db6ee8[BS001_PART_FIRST].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            DAT_10db6ee8[BS001_PART_FIRST + 1].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            DAT_10db6ee8[BS001_PART_FIRST + 2].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            DAT_10db6ee8[BS001_PART_FIRST + 3].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            DAT_104c00c8 = 0;
            DAT_104c0354 = 0;
            DAT_11018734 = 1;
        }
        AnimModel *parts = &DAT_10db6ee8[BS001_PART_FIRST];
        if (DAT_1020b430[index].bs001.hitRecover == 0)
        {
            FUN_10011c60();
            model.PlayMotion(BS001_MOTION_SHOUT + 3, 1, 5, 1.0f);
            DAT_1020b430[index].bs001.motion = 0xd;
            if (DAT_10daaae8[index].m_00a8 != 0)
                partnerModel.PlayMotion(0xd, 1, 5, 1.0f);
            FUN_10065b10(index, BS001_SOUND_DEFEAT_START, 1, 0);
            if (DAT_104bfe78 == 0)
                DAT_1020b430[index].bs001.hitRecover = 1;
            parts[0].heading = model.heading;
            parts[0].targetHeading = model.heading;
            parts[2].heading = DAT_10db6ee8[index + 1].heading;
            parts[2].targetHeading = DAT_10db6ee8[index + 1].heading;
            parts[1].heading = model.targetHeading;
            parts[1].targetHeading = model.targetHeading;
            parts[3].heading = DAT_10db6ee8[index + 1].targetHeading;
            parts[3].targetHeading = DAT_10db6ee8[index + 1].targetHeading;
            DAT_1020b430[index].bs001.defeatTimer = BS001_DEFEAT_START_VALUE;
            if (DAT_10daaae8[index].m_00a8 == 0)
            {
                parts[2].PlayMotion(0xf, 1, 0, 1.0f);
                parts[3].PlayMotion(0xf, 1, 0, 1.0f);
            }
            DAT_10da9150.Copy(DAT_118e69e8);
            DAT_10868818.Copy(DAT_118e6cb8);
            model.root->M_101946b0(0, 0xffffffff);
            DAT_10db6ee8[index + 1].root->M_101946b0(0, 0xffffffff);
            parts[0].root->M_101946b0(0, 0xffffffff);
            parts[1].root->M_101946b0(0, 0xffffffff);
            parts[2].root->M_101946b0(0, 0xffffffff);
            parts[3].root->M_101946b0(0, 0xffffffff);
            model.alpha = 1.0f;
            DAT_10db6ee8[index + 1].alpha = 1.0f;
            DAT_104c00c8 = 1;
            DAT_10453008[index] = -10001;
            parts[0].alpha = 0.0f;
            parts[1].alpha = 0.0f;
            parts[2].alpha = 0.0f;
            parts[3].alpha = 0.0f;
        }
        int timer = DAT_1020b430[index].bs001.defeatTimer;
        if (timer > 0)
        {
            float shown;
            if (timer < BS001_DEFEAT_FADE_FRAMES)
            {
                shown = 1.0f - (float)timer / 90.0f;
                model.alpha = shown;
            }
            else
            {
                model.m_0294 = 1;
                model.motionFrame = parts[0].motionFrame;
                shown = 0.0f;
                model.alpha = 0.0f;
                DAT_10db6ee8[index + 1].m_0294 = 1;
                DAT_10db6ee8[index + 1].motionFrame = parts[2].motionFrame;
            }
            DAT_10db6ee8[index + 1].alpha = shown;
            if (timer < BS001_DEFEAT_FLASH_FRAMES)
            {
                float rise = (float)(BS001_DEFEAT_FLASH_FRAMES - timer);
                parts[0].scale.x = 1.0f;
                parts[0].scale.y = 0.0f;
                parts[0].scale.z = 0.0f;
                parts[0].scale.w = rise / 1830.0f + parts[0].scale.w;
            }
            else if (timer < BS001_DEFEAT_FADE_FRAMES)
            {
                parts[0].scale.x = 1.0f;
                parts[0].scale.y = 0.0f;
                parts[0].scale.z = 0.0f;
                parts[0].scale.w = 1.0f;
            }
            else if (timer < BS001_DEFEAT_END_FRAME)
            {
                if (timer == BS001_DEFEAT_FADE_FRAMES)
                {
                    FUN_10065b10(index, BS001_SOUND_DEFEAT_BLAST, 1, 0);
                    if (DAT_10c636c8 != 0)
                    {
                        Script_ValueCommand(0x7549, 0x4b);
                        if (DAT_10c6356c != 0)
                        {
                            DAT_10c6356c = 100000;
                            DAT_10c63588 = 7;
                            Script_ValueCommand(0x753f, 0x4b);
                        }
                    }
                }
                float remaining = (float)(BS001_DEFEAT_END_FRAME - timer);
                float elapsed = (float)(timer - BS001_DEFEAT_FADE_FRAMES);
                parts[0].scale.x = 1.0f;
                parts[0].scale.y = remaining * remaining * 0.004213826f + parts[0].scale.y;
                parts[0].scale.z = parts[0].scale.z - elapsed * elapsed * 0.009481108f;
                parts[0].scale.w = parts[0].scale.w - elapsed / 3081.0f;
                if (parts[0].scale.w < 0.0f)
                    parts[0].scale.w = 0.0f;
            }
            else
                parts[0].scale.Fill(0.0f);
        }
        timer = DAT_1020b430[index].bs001.defeatTimer;
        if (timer > BS001_DEFEAT_SPLIT_FIRST - 1)
        {
            if (timer < BS001_DEFEAT_BLAST_FRAME)
            {
                float since = (float)(timer - BS001_DEFEAT_SPLIT_FIRST);
                parts[1].scale.z = 0.0f;
                parts[1].scale.x = (float)(BS001_DEFEAT_BLAST_FRAME - timer) * 0.000152207f + parts[1].scale.x;
                parts[1].scale.y = since / 8.0f;
                parts[1].scale.w = since / 144.0f + 0.5f;
                if (parts[1].scale.x < 0.0f)
                    parts[1].scale.x = 0.0f;
            }
            else if (timer < BS001_DEFEAT_SCENE_END)
            {
                if (timer == BS001_DEFEAT_BLAST_FRAME)
                    parts[1].scale.x = 0.3f;
                float since = (float)(timer - BS001_DEFEAT_BLAST_FRAME);
                float remaining = (float)(BS001_DEFEAT_SCENE_END - timer);
                parts[1].scale.x = parts[1].scale.x - since * 8.591065e-05f;
                parts[1].scale.y = remaining * 0.08591065f + parts[1].scale.y;
                parts[1].scale.z = since * 0.128866f + parts[1].scale.z;
                parts[1].scale.w = (remaining * remaining) / 9216.0f;
                if (parts[1].scale.x < 0.0f)
                    parts[1].scale.x = 0.0f;
                float spin = (float)(timer - 0x60) * 5e-05f;
                parts[1].heading -= spin;
                parts[3].heading -= spin;
                parts[1].targetHeading = parts[1].heading;
                parts[3].targetHeading = parts[3].heading;
            }
            else
                parts[1].scale.Fill(0.0f);
        }
        parts[0].m_0294 = 1;
        parts[1].m_0294 = 1;
        parts[2].m_0294 = 1;
        parts[3].m_0294 = 1;
        DAT_1020b430[index].bs001.defeatTimer++;
        parts[0].motionFrame = model.motionFrame;
        parts[1].motionFrame = model.motionFrame;
        parts[2].motionFrame = DAT_10db6ee8[index + 1].motionFrame;
        parts[3].motionFrame = DAT_10db6ee8[index + 1].motionFrame;
        parts[0].SetPosition(model.matrix._41, model.matrix._42, model.matrix._43);
        parts[2].SetPosition(DAT_10db6ee8[index + 1].matrix._41, DAT_10db6ee8[index + 1].matrix._42,
                             DAT_10db6ee8[index + 1].matrix._43);
        if (DAT_104bfe80 < 2)
        {
            // Low effect detail leaves out the smoke pieces.
            parts[1].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
            parts[3].SetPosition(BS001_HIDDEN_X, 0.0f, 0.0f);
        }
        else
        {
            parts[1].SetPosition(model.matrix._41, model.matrix._42, model.matrix._43);
            parts[3].SetPosition(DAT_10db6ee8[index + 1].matrix._41, DAT_10db6ee8[index + 1].matrix._42,
                                 DAT_10db6ee8[index + 1].matrix._43);
        }
        parts[1].alpha = parts[1].scale.w;
        parts[3].alpha = parts[1].scale.w;
        parts[0].alpha = parts[0].scale.w;
        parts[2].alpha = parts[0].scale.w;
        parts[2].scale.Copy(parts[0].scale);
        parts[3].scale.Copy(parts[1].scale);

        // The camera circles the boss, then eases back out of walls and towards its resting distance.
        Vec4 offset;
        offset.Fill(0.0f);
        offset.x += BS001_CAMERA_SIDE;
        offset.y -= BS001_CAMERA_BACK;
        float facing = FUN_10001f80(DAT_10868818.x - DAT_10da9150.x, DAT_10868818.y - DAT_10da9150.y);
        float swing = (float)(DAT_1020b430[index].bs001.defeatTimer + 30);
        offset.RotateZ(-(facing + BS001_PI) - swing * swing * 0.00003f - 0.04363f);
        DAT_10edbd48.Copy(model.matrix.GetTranslation());
        DAT_10edbd48.z += 400.0f;
        DAT_10866d98.Copy(DAT_10edbd48 + offset * 1.6f);
        if (DAT_1020b430[index].bs001.defeatTimer < 0xa1)
        {
            float remaining = (float)(0xa0 - DAT_1020b430[index].bs001.defeatTimer);
            float travel = (remaining + 1.0f) * remaining * 0.5f;
            travel = travel * travel;
            float closeness = 1.0f - travel / 329241088.0f;
            DAT_10866d98.Copy(DAT_10da9150 + (DAT_10866d98 - DAT_10da9150) * closeness);
            DAT_10edbd48.Copy(DAT_10868818 + (DAT_10edbd48 - DAT_10868818) * closeness);
        }
        Vec4 wallHit;
        if (DAT_10866d98.Length2D() > 1550.0f && DAT_10255f08 != 0 &&
            DAT_10255f08->CastSegment("CA", 4, &DAT_10866d98, &DAT_10edbd48, &wallHit, 0) != 0)
        {
            float gap = (wallHit - DAT_10edbd48).Length();
            if (gap <= 60.0f)
                gap = gap * 0.75f;
            else
                gap = gap - 50.0f;
            DAT_10866d98.Copy(DAT_10edbd48 + (DAT_10866d98 - DAT_10edbd48).Normalized3() * gap);
            Vec4 playerSide = DAT_10da9150 - DAT_10868818;
            playerSide.RotateZ(-0.1313f);
            DAT_10da9150.Copy(DAT_10868818 + playerSide);
        }
        float distance = (DAT_10edbd48 - DAT_10866d98).Length();
        if (distance <= DAT_10255160[index][0])
            DAT_10255160[index][0] = distance;
        else
        {
            DAT_10255160[index][0] = (distance - DAT_10255160[index][0]) * 0.3f + DAT_10255160[index][0];
            DAT_10866d98.Copy(DAT_10edbd48 + (DAT_10866d98 - DAT_10edbd48).Normalized3() * DAT_10255160[index][0]);
        }
        DAT_11af3e28.Copy(DAT_10866d98);
        DAT_118e6c78.Copy(DAT_10edbd48);
        return 1;
    }

    DAT_1021f9c0[index] = 0;
    DAT_1021f9c0[index + 1] = 0;
    DAT_10daaae8[index].m_0141 = 0;
    if (DAT_1020b430[index].bs001.initialised == 0)
    {
        AnimModel *parts = &DAT_10db6ee8[BS001_PART_FIRST];
        parts[0].LoadFile(FUN_1013f8a0("bs_001en.it3", 0));
        parts[1].LoadFile(FUN_1013f8a0("bs_001pa.it3", 0));
        parts[2].LoadFile(FUN_1013f8a0("bs_001be.it3", 0));
        parts[3].LoadFile(FUN_1013f8a0("bs_001bp.it3", 0));
        parts[0].LoadModel(FUN_1013f8a0("bs_001m.it3", 0));
        parts[1].LoadModel(FUN_1013f8a0("bs_001m.it3", 0));
        parts[2].LoadModel(FUN_1013f8a0("bs_001m.it3", 0));
        parts[3].LoadModel(FUN_1013f8a0("bs_001m.it3", 0));
        parts[0].mode = 0;
        parts[1].mode = 0;
        parts[2].mode = 0;
        parts[3].mode = 0;
        parts[0].scale.Fill(0.0f);
        parts[1].scale.Fill(0.0f);
        parts[2].scale.Fill(0.0f);
        parts[3].scale.Fill(0.0f);
        HideModel(parts[0]);
        HideModel(parts[1]);
        HideModel(parts[2]);
        HideModel(parts[3]);
        DAT_1020b430[index].bs001.initialised = 1;
        DAT_10836c70.ResetFrameTimer();
        Node *root = model.root;
        DAT_1021bdf8[index][BS001_NODE_RIGHT_UPPER_ARM] = root->Find("Bip01 R UpperArm", 0);
        DAT_1021bdf8[index][BS001_NODE_LEFT_UPPER_ARM] = root->Find("Bip01 L UpperArm", 0);
        DAT_1021bdf8[index][BS001_NODE_LEFT_HAND] = root->Find("Bip01 L Hand", 0);
        DAT_1021bdf8[index][BS001_NODE_LEFT_FINGER] = root->Find("Bip01 L Finger0", 0);
        DAT_1021bdf8[index][BS001_NODE_RIGHT_HAND] = root->Find("Bip01 R Hand", 0);
        DAT_1042d918[BS001_BONE_LEFT_TOE] = (int)root->Find("Bip01 L Toe0", 0);
        DAT_1042d918[BS001_BONE_RIGHT_TOE] = (int)root->Find("Bip01 R Toe0", 0);
        DAT_1042d918[BS001_BONE_LEFT_FOOT] = (int)root->Find("Bip01 L Foot", 0);
        DAT_1042d918[BS001_BONE_RIGHT_FOOT] = (int)root->Find("Bip01 R Foot", 0);
        DAT_1042d918[BS001_BONE_HEAD] = (int)root->Find("Bip01 Head", 0);
        DAT_1042d918[BS001_BONE_FOOTSTEPS] = (int)root->Find("Bip01 Footsteps", 0);
        DAT_1042d918[BS001_BONE_SPINE] = (int)root->Find("Bip01 Spine", 0);
        DAT_1020b430[index].bs001.state = BS001_REST;
        DAT_1020b430[index].bs001.motion = BS001_MOTION_IDLE;
        model.PlayMotion(BS001_MOTION_IDLE, 1, 5, 1.0f);
        partnerModel.PlayMotion(BS001_MOTION_IDLE, 1, 5, 1.0f);
        DAT_10da9150.Copy(DAT_118e69e8);
        DAT_10868818.Copy(DAT_118e6cb8);
        DAT_10802c08[index][0].Copy(DAT_10da9150);
        DAT_10255160[index][1] = 0.0f;
        DAT_10daaae8[index + 2].kind = -1;
    }
    DAT_10255a88[index] = 0;
    DAT_10210d30[index] = 0;
    DAT_1020b430[index].bs001.idle = 0;
    if (DAT_10210b30[index] != 0)
    {
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].bs001.state = BS001_REST;
        DAT_1020b430[index].bs001.finished = 1;
    }
    if (DAT_1020b430[index].bs001.hitRecover < 0)
        DAT_1020b430[index].bs001.hitRecover = 0;
    else if (DAT_1020b430[index].bs001.hitRecover > 10)
        DAT_1020b430[index].bs001.hitRecover = 0;
    else if (DAT_1020b430[index].bs001.hitRecover != 0 && DAT_1020b430[index].bs001.hitRecover < 10)
        DAT_1020b430[index].bs001.hitRecover--;
    if (distance < 10000.0f)
        DAT_1020344c = distance * 0.0003f + 0.95f;
    DAT_1101dddc = 125.0f;
    if (DAT_10daaae8[index].m_00a8 != 0)
        DAT_10daaae8[index].held = 1;
    if (DAT_1020b430[index].bs001.phase == 1)
    {
        DAT_1020b430[index].bs001.state = BS001_HURT_STAGGER;
        DAT_1020b430[index].bs001.frame = 0;
        DAT_1020b430[index].bs001.phase = 2;
    }

    int hit = DAT_102550e0[index];
    Bs001State &self = DAT_1020b430[index].bs001;
    Bs001Aux &aux = DAT_1020b430[index + 1].bs001Aux;
    LastPosition &lastMove = DAT_10802bf8[index];
    Vec4 origin;
    if (hit == 0)
    {
        if (self.state == BS001_REST)
        {
            self.motion = BS001_MOTION_IDLE;
            self.frame++;
            if (self.frame > 0x3c && FUN_1013f880() % 0x32 < 5)
                self.state = BS001_CHOOSE;
            if (syncFlag != 0)
                self.state = BS001_CHOOSE;
        }
        else if (self.state == BS001_STAND_TURN)
        {
            self.motion = BS001_MOTION_IDLE;
            if (self.frame == 0 && (FUN_1013f880() & 1) != 0 && DAT_10c66cc0 != 0)
                BossSayLine(index, BS001_VOICE_STAND, BS001_VOICE_COOLDOWN_STAND);
            if (self.frame % 0x28 == 0x14)
                FUN_1001d8b0(index, BS001_EFFECT_BLOW, selfPos, -1, -1, 0, 0, 0, 0, 0);
            self.frame++;
            if (self.frame > 0x3c && self.frame % 0x50 == 0xf && FUN_1013f880() % 0x32 < 0x19)
                self.state = BS001_CHOOSE;
            if (syncFlag == 2)
                self.state = BS001_CHOOSE;
        }
        else if (self.state == BS001_WALK)
        {
            temp = moveDir - selfPos;
            moveDir.Copy(temp);
            moveDir.Copy(Vec3_Normalized(&moveDir));
            if (DAT_10daaae8[index].held == 0)
            {
                self.state = BS001_RUN;
                self.frame = -1;
            }
            else
                moveDir.Copy(moveDir * 5.0f);
            FaceDirection(model, moveDir);
            if (DAT_104bfd08 == 0)
                PushForwards(model, moveDir);
            self.motion = BS001_MOTION_WALK;
            if (self.frame == 0)
                restartMotion = 1;
            self.frame++;
            if (self.frame > 0x3c && FUN_1013f880() % 100 < 10)
                self.state = BS001_CHOOSE;
            if (syncFlag == 2)
                self.state = BS001_CHOOSE;
        }
        else if (self.state == BS001_RUN)
        {
            temp = moveDir - selfPos;
            moveDir.Copy(temp);
            moveDir.Copy(Vec3_Normalized(&moveDir));
            moveDir.Copy(moveDir * 8.0f);
            FaceDirection(model, moveDir);
            if (DAT_104bfd08 == 0)
                PushForwards(model, moveDir);
            self.motion = BS001_MOTION_RUN;
            if (self.frame == 0)
                restartMotion = 1;
            self.frame++;
            if (self.frame > 0x3c && FUN_1013f880() % 100 < 0x14)
                self.state = BS001_CHOOSE;
            if (syncFlag == 2)
                self.state = BS001_CHOOSE;
        }
        else if (self.state == BS001_IDLE_UNUSED)
            self.state = BS001_CHOOSE;
        else if (self.state == BS001_LUNGE)
        {
            Vec4 lunge;
            lunge.Fill(0.0f);
            temp = moveDir - selfPos;
            moveDir.Copy(temp);
            moveDir.Copy(Vec3_Normalized(&moveDir));
            FaceDirection(model, moveDir);
            moveDir.y = -moveDir.Length2D();
            moveDir.x = 0.0f;
            moveDir.RotateZ(-model.heading);
            lunge.Copy(lunge + moveDir * 5.0f);
            lunge.Normalized3();
            lunge.Copy(lunge * 5.0f * 1.8f);
            if (self.frame < 10)
                lunge.Copy(lunge * 0.05f);
            self.motion = BS001_MOTION_STOMP;
            if (DAT_104bfd08 == 0)
                model.Push(lunge.x, lunge.y, lunge.z);
            if (self.frame % 0x28 > 3 && self.frame % 0x28 < 0x25)
            {
                Vec4 stompPos;
                EffectOptions options;
                options.flags |= 2;
                BoneEffect(index, BS001_EFFECT_STOMP_R, stompPos, DAT_1021bdf8[index][BS001_NODE_RIGHT_UPPER_ARM], &options);
                BoneEffect(index, BS001_EFFECT_STOMP_L, stompPos, DAT_1021bdf8[index][BS001_NODE_LEFT_UPPER_ARM], &options);
                DAT_1021f6c0[index] = 1;
            }
            self.frame++;
            if (self.frame % 0x28 == 0 && FUN_1013f880() % (self.frame / 0x28 + 1) > 0)
            {
                self.motion = BS001_MOTION_IDLE;
                self.state = BS001_CHOOSE;
                self.frame = 0;
                self.finished = 1;
            }
            if (self.frame % 0x28 % 7 == 0 && self.frame % 0x28 > 5)
                FUN_10065b10(index, BS001_SOUND_STOMP, 1, 0);
        }
        else if (self.state == BS001_SHOUT)
        {
            if (self.frame == 0)
            {
                restartMotion = 1;
                EffectOptions ringOptions;
                ringOptions.owner = index;
                for (int ring = 0; ring < 3; ring++)
                    FUN_1001d8b0(index, BS001_EFFECT_SHOUT_RING, selfPos, -1, -1, 0, 0, 0, 0, &ringOptions);
                temp = moveDir - selfPos;
                moveDir.Copy(temp);
                moveDir.Copy(Vec3_Normalized(&moveDir));
                FaceDirection(model, moveDir);
                moveDir.y = -moveDir.Length2D();
                moveDir.x = 0.0f;
                moveDir.RotateZ(-model.targetHeading);
                lastMove.position.Copy(moveDir);
                if (DAT_10c66cc0 != 0)
                    BossSayLine(index, BS001_VOICE_SHOUT, BS001_VOICE_COOLDOWN_SHOUT);
            }
            self.motion = BS001_MOTION_SHOUT;
            self.frame++;
            if (self.frame == 1)
            {
                AttachEffect(index, origin, DAT_1021bdf8[index][BS001_NODE_RIGHT_HAND]);
                AttachEffect(index, origin, DAT_1021bdf8[index][BS001_NODE_LEFT_HAND]);
            }
            if (self.frame == 10)
            {
                Vec4 spawn;
                spawn.Copy(model.matrix.GetTranslation());
                spawn.z += 50.0f;
                EffectOptions sparkOptions;
                sparkOptions.m_0004 = DAT_10daaae8[index].stats[3];
                sparkOptions.flags |= 2;
                sparkOptions.m_0000 = 0;
                int rays = hardMode * 2 + 8;
                for (int ray = 0; ray < rays; ray++)
                {
                    Vec4 rayPos;
                    rayPos.Fill(0.0f);
                    Matrix4 placement;
                    placement = RotationZ((float)ray * (2.0f / ((float)hardMode * 2.0f + 8.0f)) * BS001_PI + model.heading);
                    placement.SetTranslation(spawn);
                    FUN_1001d8b0(index, ray == 0 ? BS001_EFFECT_SHOUT_SPARK : BS001_EFFECT_SHOUT_SPARK_OUTER, rayPos, -1, -1,
                                 0, 0, 0, &placement, &sparkOptions);
                }
                if (self.frame == 10)
                    FUN_10065b10(index, BS001_SOUND_SHOUT_RING, 1, 0);
            }
            if (self.frame == 0x23)
            {
                model.PlayMotion(BS001_MOTION_IDLE, 1, 5, 1.0f);
                self.motion = BS001_MOTION_IDLE;
                if (DAT_10daaae8[index].m_00a8 != 0)
                    partnerModel.PlayMotion(BS001_MOTION_IDLE, 1, 5, 1.0f);
                self.state = BS001_REST;
                self.frame = 0;
                self.finished = 1;
            }
        }
        else if (self.state == BS001_STAGGER)
        {
            int frameNow = self.frame;
            if (frameNow == 0)
            {
                restartMotion = 1;
                self.phase = 2;
            }
            self.motion = BS001_MOTION_HURT;
            if (self.phase == 0 && DAT_10daaae8[index].stats[STAT_HEALTH] < DAT_10daaae8[index].stats[STAT_MAX_HEALTH] / 2)
            {
                self.phase = 2;
                self.state = BS001_HURT_STAGGER;
            }
            if (frameNow > 0xd2)
            {
                self.state = BS001_DOWN;
                self.frame = 0;
            }
            self.frame++;
        }
        else if (self.state == BS001_DOWN)
        {
            self.frame++;
            if (self.frame == 1)
            {
                self.motion = BS001_MOTION_DOWN;
                restartMotion = 1;
            }
            else if (self.frame > 9)
            {
                if (self.frame < 0x1e)
                    model.SetPosition(model.matrix._41, model.matrix._42, model.matrix._43 + 50.0f);
                else
                {
                    self.state = BS001_RISE;
                    self.frame = 0;
                    aux.m_0004 = 0;
                    DAT_104c00c8 = 1;
                    model.alpha = 1.0f;
                    FUN_100b9bb0(index, BS001_VOICE_GETS_UP, 0, 0);
                    self.motion = BS001_MOTION_IDLE;
                }
            }
        }
        else if (self.state == BS001_RISE)
        {
            self.motion = BS001_MOTION_IDLE;
            if (self.frame == 1)
            {
                if (DAT_10255f08 != 0)
                {
                    Node *doorA = DAT_10255f08->Find("@mp_105t1", 0);
                    if (doorA != 0)
                        doorA->SetFlags(BS001_DOOR_FLAG);
                    Node *doorB = DAT_10255f08->Find("@mp_105t2", 0);
                    if (doorB != 0)
                        doorB->ClearFlags(BS001_DOOR_FLAG);
                }
                DAT_10db6ee8[PLAYER_SLOT].shadowHidden = 0;
                DAT_104c00c8 = 0;
                restartMotion = 1;
                AnimModel &home = DAT_10daaae8[index].m_00a8 == 0 ? model : partnerModel;
                Vec4 homePos;
                homePos.Copy(home.matrix.GetTranslation());
                self.homeX = (int)homePos.x;
                self.homeY = (int)homePos.y;
                self.homeZ = (int)homePos.z;
                model.SetPosition(BS001_RISE_PARK_X, 0.0f, 0.0f);
                self.frame++;
            }
            else
            {
                DAT_10daaae8[index].m_0141 = 1;
                if ((DAT_10874848.TestScriptFlag(BS001_STAGE_FLAG) < 2 && self.frame % 0x2d == 1) ||
                    (DAT_10874848.TestScriptFlag(BS001_STAGE_FLAG) == 2 && self.frame % 0x19 == 1))
                {
                    Vec4 dustPos;
                    dustPos.Copy(DAT_10db6ee8[PLAYER_SLOT].matrix.GetTranslation());
                    dustPos.x += (float)(FUN_1013f880() % 300 - 150);
                    dustPos.z = 0.0f;
                    dustPos.y += (float)(FUN_1013f880() % 300 - 150);
                    EffectOptions dustOptions;
                    dustOptions.m_0004 = (int)((float)DAT_10daaae8[index].stats[3] * 1.2f);
                    dustOptions.m_0000 = 0;
                    dustOptions.m_000e = 5;
                    FUN_1001d8b0(index, BS001_EFFECT_RISE_DUST, dustPos, -1, -1, 0, 0, 0, 0, &dustOptions);
                }
                if (self.frame == 0xdc)
                {
                    model.SetPosition((float)self.homeX, (float)self.homeY, (float)self.homeZ);
                    self.state = BS001_REVIVE;
                    self.frame = 0;
                    FUN_100b9bb0(index, BS001_VOICE_RISE, 0, 1);
                }
                self.frame++;
            }
        }
        else if (self.state == BS001_REVIVE)
        {
            self.frame++;
            self.motion = BS001_MOTION_IDLE;
            if (self.frame == 0x1e)
            {
                self.homeY = -1;
                self.state = BS001_CHOOSE;
                self.frame = 0;
                DAT_10daaae8[index].m_0141 = 0;
                Matrix4 burst;
                FUN_10065b10(index, BS001_SOUND_SHOUT, 1, 0);
                burst.SetIdentity();
                origin.Fill(0.0f);
                burst = model.matrix;
                FUN_1001d8b0(index, BS001_EFFECT_SHOUT_CLOUD, origin, -1, -1, 0, 0, 0, &burst, 0);
            }
        }
        else if (self.state == BS001_KNOCKED)
        {
            aux.attackCount = 0;
            DAT_10daaae8[index].m_0141 = 1;
            if (self.frame == 0)
            {
                restartMotion = 1;
                FUN_100b9bb0(index, BS001_VOICE_FALL_FIRST + (FUN_1013f880() & 1), 1, 0);
            }
            self.frame++;
            self.motion = BS001_MOTION_FALL;
            if (model.motionFrame == 0x133)
            {
                DAT_10daaae8[index].m_00a8 = 0;
                partnerModel.PlayMotion(0xf, 1, 0, 1.0f);
                self.homeY = 0;
                Vec4 thrown;
                thrown.y = -16.0f;
                thrown.RotateZ(-model.heading);
                aux.knockZ = 5000;
                aux.knockX = (int)(thrown.x * 100.0f);
                aux.knockY = (int)(thrown.y * 100.0f);
                aux.knockActive = 1;
            }
            if (model.motionFrame == 0x15d)
            {
                self.motion = BS001_MOTION_IDLE;
                self.state = BS001_REST;
                self.frame = 0;
                self.finished = 1;
                DAT_10daaae8[index].m_0141 = 0;
                DAT_10daaae8[index].held = 0;
            }
        }
        else if (self.state == BS001_RETREAT)
        {
            aux.knockZ = 0;
            aux.knockX = 0;
            aux.knockY = 0;
            aux.knockActive = 0;
            DAT_10daaae8[index].m_0141 = 1;
            if (self.frame == 0)
            {
                restartMotion = 1;
                partnerModel.PlayMotion(0x11, 1, 0, 1.0f);
                FUN_100b9bb0(index, BS001_VOICE_RETREAT, 0, 0);
            }
            self.frame++;
            self.motion = BS001_MOTION_DOWN;
            if (self.frame == 5)
                DAT_10daaae8[index].m_00a8 = 1;
            else if (self.frame == 10)
            {
                self.state = BS001_CHOOSE;
                self.frame = 0;
                DAT_10daaae8[index].m_0141 = 0;
                DAT_10daaae8[index].held = 1;
            }
        }
        else if (self.state == BS001_PUNCH)
        {
            self.motion = 5;
            if (self.frame == 0)
            {
                restartMotion = 1;
                temp = moveDir - selfPos;
                lastMove.position.Copy(temp);
                lastMove.position.Normalized3();
                lastMove.position.Copy(lastMove.position * 5.0f);
                FUN_100b9bb0(index, BS001_VOICE_PUNCH, 0, 1);
            }
            moveDir.Copy(lastMove.position);
            DAT_1021f6c0[index] = 1;
            if (self.frame < 10 || self.frame > 0x16)
                moveDir.Copy(moveDir * 0.01f);
            if (self.frame == 10)
            {
                Node *hand = DAT_1021bdf8[index][BS001_NODE_LEFT_HAND];
                if (hand != 0)
                {
                    Vec4 flashPos;
                    Matrix4 flashPose;
                    EffectOptions flashOptions;
                    AttachEffect(index, flashPos, hand);
                    flashPose = RotationZ(model.heading);
                    flashPose.SetTranslation(hand->worldMatrix.GetTranslation());
                    flashOptions.m_0004 = DAT_10daaae8[index].stats[3];
                    flashOptions.flags |= 2;
                    flashOptions.m_0000 = 0;
                    BossEffect(index, BS001_EFFECT_PUNCH_FLASH, flashPos, 0, &flashPose, &flashOptions);
                    FUN_10065b10(index, BS001_SOUND_PUNCH, 1, 0);
                }
            }
            if (self.frame > 4 && self.frame < 0x19)
            {
                Vec4 trailPos;
                Matrix4 trailPose;
                trailPose = RotationZ(model.heading);
                Node *finger = DAT_1021bdf8[index][BS001_NODE_LEFT_FINGER];
                if (finger != 0)
                {
                    trailPose.SetTranslation(finger->worldMatrix.GetTranslation());
                    BossEffect(index, BS001_EFFECT_PUNCH_TRAIL, trailPos, 0, &trailPose, 0);
                }
            }
            FaceDirection(model, moveDir);
            if (DAT_104bfd08 == 0)
                model.Push(moveDir.x, moveDir.y, 0.0f);
            self.frame++;
            if (self.frame > 0x19)
            {
                self.motion = BS001_MOTION_IDLE;
                self.state = BS001_CHOOSE;
                self.frame = 0;
                self.finished = 1;
            }
        }
        else if (self.state == BS001_CLAW)
        {
            Vec4 clawPos;
            clawPos.Fill(0.0f);
            if (self.frame == 0)
            {
                restartMotion = 1;
                FUN_100b9bb0(index, BS001_VOICE_CLAW, 0, 1);
            }
            self.motion = 0x15;
            temp = moveDir - selfPos;
            moveDir.Copy(temp);
            moveDir.Copy(Vec3_Normalized(&moveDir));
            FaceDirection(model, moveDir);
            if (model.motionFrame == 0x2b5 || model.motionFrame == 0x2ad)
            {
                Matrix4 clawPose;
                EffectOptions clawOptions;
                Node *foot = Bs001BoneNode(model.motionFrame == 0x2b5 ? BS001_BONE_LEFT_FOOT : BS001_BONE_RIGHT_FOOT);
                clawPose = RotationZ(model.heading);
                if (foot != 0)
                {
                    clawPose.SetTranslation(foot->worldMatrix.GetTranslation());
                    clawOptions.m_0004 = DAT_10daaae8[index].stats[3];
                    clawOptions.m_0000 = 0;
                    clawOptions.m_000e = 5;
                    BossEffect(index, BS001_EFFECT_CLAW_FLASH, clawPos, 0, &clawPose, &clawOptions);
                }
                FUN_10065b10(index, BS001_SOUND_CLAW, 1, 0);
            }
            if (self.frame > 10 && self.frame < 0x5dc)
                DAT_1021f6c0[index] = 1;
            self.frame++;
            if (model.motionFrame == 0x2b7 && FUN_1013f880() % 0x32 <= self.frame / 0xf)
                self.frame = 0x5dc;
            if (self.frame > 0x5db)
            {
                Vec4 chase;
                if (self.frame == 0x5dc)
                    restartMotion = 1;
                self.motion = 0x17;
                chase.Fill(0.0f);
                temp = moveDir - selfPos;
                moveDir.Copy(temp);
                moveDir.Copy(Vec3_Normalized(&moveDir));
                FaceDirection(model, moveDir);
                moveDir.y = -moveDir.Length2D();
                moveDir.x = 0.0f;
                moveDir.RotateZ(-model.heading);
                chase.Copy(chase + moveDir * 5.0f);
                chase.Normalized3();
                chase.Copy(chase * 0.2f);
                if (DAT_104bfd08 == 0)
                    model.Push(chase.x, chase.y, 0.0f);
                if (model.motionFrame == 0x2c1)
                {
                    self.motion = BS001_MOTION_IDLE;
                    self.state = BS001_CHOOSE;
                    self.frame = 0;
                    self.finished = 1;
                }
            }
        }
        else if (self.state == BS001_SMASH || self.state == BS001_SMASH_FAST)
        {
            bool fast = self.state == BS001_SMASH_FAST;
            if (fast)
                DAT_10daaae8[index].m_0141 = 1;
            if (self.frame == 0)
            {
                restartMotion = 1;
                if (fast)
                    quickBlend = 1;
                FUN_100b9bb0(index, BS001_VOICE_SMASH, 0, 1);
            }
            int swing = model.motionFrame;
            self.motion = BS001_MOTION_SMASH;
            if (swing > BS001_SMASH_FRAME_FIRST && swing < BS001_SMASH_FRAME_LAST)
            {
                Vec4 push;
                push.Fill(0.0f);
                if (fast)
                    moveDir.Copy(partnerModel.matrix.GetTranslation());
                temp = moveDir - selfPos;
                moveDir.Copy(temp);
                moveDir.Copy(Vec3_Normalized(&moveDir));
                FaceDirection(model, moveDir);
                moveDir.y = -moveDir.Length2D();
                moveDir.x = 0.0f;
                moveDir.RotateZ(-model.heading);
                moveDir.Copy(moveDir * 12.0f);
                if (model.motionFrame < 0x249)
                    moveDir.Copy(moveDir * 0.8f + lastMove.position * 0.2f);
                else
                    moveDir.Copy(lastMove.position * 0.8f);
                lastMove.position.Copy(moveDir);
                if (DAT_104bfd08 == 0)
                    model.Push(moveDir.x, moveDir.y, 0.0f);
            }
            swing = model.motionFrame;
            if (swing > 0x23a && swing < 0x256)
                DAT_1021f6c0[index] = 1;
            if (swing > BS001_SMASH_FRAME_FIRST)
                DAT_10daaae8[index].m_0141 = 1;
            if (self.frame == 3)
                FUN_10065b10(index, BS001_SOUND_SMASH_HIT, 1, 0);
            swing = model.motionFrame;
            self.frame++;
            if (!fast && swing > 0x280)
                DAT_10daaae8[index].m_0141 = 0;
            if (swing == BS001_SMASH_END_FRAME)
            {
                self.motion = BS001_MOTION_IDLE;
                self.state = BS001_CHOOSE;
                self.frame = 0;
                self.finished = 1;
                DAT_10daaae8[index].m_0141 = 0;
            }
        }
        else if (self.state == BS001_CHASE)
        {
            self.motion = BS001_MOTION_RUN;
            moveDir.Copy(DAT_10db6ee8[PLAYER_SLOT].matrix.GetTranslation());
            temp = moveDir - selfPos;
            moveDir.Copy(temp);
            if (moveDir.Length() >= 150.0f)
            {
                moveDir.Copy(Vec3_Normalized(&moveDir));
                moveDir.Copy(moveDir * 10.0f);
                FaceDirection(model, moveDir);
                if (DAT_104bfd08 == 0)
                {
                    moveDir.y = -moveDir.Length2D();
                    moveDir.x = 0.0f;
                    moveDir.RotateZ(-model.heading);
                    model.Push(moveDir.x, moveDir.y, 0.0f);
                    float travelled = BS001_PI - FUN_10001f80(moveDir.x, moveDir.y);
                    float expected = BS001_PI - FUN_10001f80(model.matrix._41 - selfPos.x, model.matrix._42 - selfPos.y);
                    float skew = travelled - expected;
                    while (skew > BS001_PI)
                        skew -= BS001_TWO_PI;
                    while (skew < -BS001_PI)
                        skew += BS001_TWO_PI;
                    bool pickNext = model.IsClearAny(&moveDir) == 0;
                    if (!pickNext)
                        pickNext = (DAT_10db6ee8[PLAYER_SLOT].matrix.GetTranslation() - selfPos).Length2D() < 500.0f &&
                                   fabsf(skew) < 0.7854f;
                    if (pickNext)
                    {
                        if (FUN_1013f880() % 100 < 0x1e)
                            self.state = BS001_SMASH_FAST;
                        else
                        {
                            self.state = BS001_CHOOSE;
                            aux.waitFrames = FUN_1013f880() % ((self.phase != 0) * 2 + 2) + 1;
                        }
                        self.frame = 0;
                    }
                    if (model.IsClearOfMapLimited(&moveDir) == 0)
                    {
                        moveDir.Copy(model.matrix.GetTranslation());
                        moveDir.Copy(moveDir * -1.0f);
                        moveDir.x = 0.0f;
                        moveDir.y = -10.0f;
                        moveDir.RotateZ(-model.heading);
                        model.Push(moveDir.x, moveDir.y, 0.0f);
                    }
                }
            }
            else
            {
                self.state = BS001_RETREAT;
                DAT_10daaae8[index].m_0141 = 1;
                self.frame = 0;
                restartMotion = 1;
                self.motion = BS001_MOTION_DOWN;
                partnerModel.PlayMotion(0x11, 1, 0, 1.0f);
            }
        }
        else if (self.state == BS001_HURT_STAGGER)
        {
            int frameNow = self.frame;
            if (frameNow == 0)
            {
                restartMotion = 1;
                self.phase = 2;
            }
            self.motion = BS001_MOTION_HURT;
            if (frameNow > 0x3c)
            {
                self.state = BS001_DOWN;
                self.frame = 0;
            }
            self.frame++;
        }
        else if (self.state == BS001_CHOOSE)
        {
            aux.attackCount++;
            aux.streak++;
            int phaseNow = self.phase;
            self.frame = 0;
            if (phaseNow == 0)
            {
                if (DAT_10daaae8[index].m_00a8 != 0)
                {
                    int last = aux.lastState;
                    if (last == BS001_REST || last == BS001_STAND_TURN || last == BS001_WALK)
                    {
                        int pick = FUN_1013f880() % 3;
                        self.state = pick == 0 ? BS001_WALK : pick == 1 ? BS001_LUNGE : BS001_SHOUT;
                    }
                    else
                    {
                        self.state = BS001_STAND_TURN;
                        aux.attackCount = 0;
                    }
                }
                else
                {
                    if (aux.attackCount < 7 || FUN_1013f880() % 100 >= (aux.attackCount - 4) * 5)
                    {
                        int pick = FUN_1013f880() % 4;
                        if (aux.attackCount != 1 || (moveDir - selfPos).Length2D() <= 650.0f)
                        {
                            switch (pick)
                            {
                            case 1:
                                self.state = BS001_LUNGE;
                                break;
                            case 2:
                                self.state = BS001_SHOUT;
                                break;
                            case 3:
                                if ((moveDir - selfPos).Length2D() >= 400.0f)
                                    self.state = BS001_SHOUT;
                                else
                                    self.state = BS001_SMASH;
                                break;
                            default:
                                self.state = BS001_WALK;
                            }
                        }
                    }
                    else
                    {
                        self.state = BS001_REST;
                        aux.attackCount = 0;
                    }
                    if (aux.streak > 6 && FUN_1013f880() % 0x32 < aux.streak * 2 - 12)
                        self.state = BS001_STAGGER;
                }
            }
            else if (phaseNow == 1)
            {
                self.state = BS001_HURT_STAGGER;
                self.phase = 2;
                aux.streak = 0;
            }
            else if (phaseNow == 2)
            {
                if (DAT_10daaae8[index].m_00a8 == 0)
                {
                    if (aux.attackCount < 7 || FUN_1013f880() % 100 >= (aux.attackCount - 5) * 5)
                    {
                        int pick = FUN_1013f880() % 9;
                        if (pick == 0 && FUN_1013f880() % 5 != 0)
                            pick = FUN_1013f880() % 8 + 1;
                        if (aux.attackCount != 1 || (moveDir - selfPos).Length2D() <= 450.0f)
                        {
                            switch (pick)
                            {
                            case 1:
                                self.state = BS001_LUNGE;
                                break;
                            case 2:
                                self.state = BS001_SHOUT;
                                break;
                            case 3:
                            case 6:
                                self.state = BS001_PUNCH;
                                break;
                            case 4:
                            case 7:
                                self.state = BS001_CLAW;
                                break;
                            case 5:
                            case 8:
                                self.state = BS001_SMASH;
                                break;
                            default:
                                if (aux.lastState == BS001_WALK)
                                {
                                    if ((moveDir - selfPos).Length2D() >= 500.0f)
                                        self.state = BS001_PUNCH;
                                    else
                                        self.state = BS001_CLAW;
                                }
                                else
                                    self.state = BS001_WALK;
                            }
                        }
                    }
                    else
                    {
                        self.state = BS001_REST;
                        aux.attackCount = 0;
                    }
                    if (aux.streak >= 4 && FUN_1013f880() % 0x32 < aux.streak * 4 - 12)
                        self.state = BS001_STAGGER;
                }
                else if (aux.attackCount > 2)
                {
                    self.state = BS001_REST;
                    aux.attackCount = 0;
                }
                else
                {
                    int pick = FUN_1013f880() % 7;
                    if (aux.attackCount != 1 || (moveDir - selfPos).Length2D() <= 650.0f)
                    {
                        switch (pick)
                        {
                        case 1:
                            self.state = BS001_LUNGE;
                            break;
                        case 2:
                            self.state = BS001_SHOUT;
                            break;
                        case 3:
                        case 5:
                            self.state = BS001_PUNCH;
                            break;
                        case 4:
                        case 6:
                            self.state = BS001_CLAW;
                            break;
                        default:
                            if (aux.lastState == BS001_RUN)
                            {
                                if ((moveDir - selfPos).Length2D() >= 500.0f)
                                    self.state = BS001_PUNCH;
                                else
                                    self.state = BS001_CLAW;
                            }
                            else
                                self.state = BS001_RUN;
                        }
                    }
                }
            }
            if (DAT_10daaae8[index].m_00a8 == 0 && aux.partnerCooldown == 0)
            {
                if (aux.waitFrames == 0)
                {
                    self.state = BS001_CHASE;
                    aux.streak = 0;
                }
                else
                    aux.waitFrames--;
            }
        }
    }
    else if (DAT_10daaae8[index].m_00a8 == 0 || DAT_1042d9e8[index] == 0)
    {
        if (hit == 1)
        {
            if (self.state != BS001_STAGGER && self.state != BS001_HURT_STAGGER)
                self.state = BS001_CHOOSE;
            if (DAT_10daaae8[index].m_00a8 == 0 && self.phase == 0 &&
                DAT_10daaae8[index].stats[STAT_HEALTH] < DAT_10daaae8[index].stats[STAT_MAX_HEALTH] / 2)
                self.phase = 1;
            if (self.state == BS001_CHASE && FUN_1013f880() % 100 < 0x32)
            {
                if (FUN_1013f880() % 100 < 0x3c)
                    self.state = BS001_SMASH_FAST;
                else
                {
                    self.state = BS001_CHOOSE;
                    aux.waitFrames = FUN_1013f880() % ((self.phase != 0) * 2 + 2) + 1;
                }
                self.frame = 0;
            }
        }
        DAT_1042d9e8[index] = 0;
    }
    else
    {
        self.state = BS001_KNOCKED;
        self.frame = 0;
        aux.streak = 0;
        aux.partnerCooldown = 200;
        DAT_102550e0[index] = 1;
        DAT_1042d9e8[index] = 0;
    }

    if (self.state != BS001_CHOOSE)
        aux.lastState = self.state;
    if (self.motion >= 0)
    {
        model.PlayMotion(self.motion, restartMotion, quickBlend == 0 ? 5 : 0, 1.0f);
        if (DAT_10daaae8[index].m_00a8 != 0)
            partnerModel.PlayMotion(self.motion, restartMotion, quickBlend == 0 ? 4 : 0, 1.0f);
    }
    if (self.motion != self.shownMotion)
    {
        self.shownMotion = self.motion;
        if (self.motion == 0 || self.motion == 1 || self.motion == 4)
            self.motionFrames = -8;
        else if (self.motion == BS001_MOTION_FALL)
            self.motionFrames = 0;
    }
    self.motionFrames++;

    Vec4 effectPos;
    effectPos.Fill(0.0f);
    Matrix4 boneMatrix;
    boneMatrix.SetIdentity();
    switch (self.motion)
    {
    case 0:
        if (self.motionFrames % 0xf == 0)
            Footstep(index, Bs001BoneNode(self.motionFrames % 0x1e != 0 ? BS001_BONE_RIGHT_TOE : BS001_BONE_LEFT_TOE),
                     effectPos, boneMatrix);
        break;
    case 1:
    {
        int stride = self.motionFrames;
        if (stride < 0xd && stride % 10 == 0)
            Footstep(index, Bs001BoneNode(stride % 0x14 != 0 ? BS001_BONE_RIGHT_TOE : BS001_BONE_LEFT_TOE), effectPos,
                     boneMatrix);
        else if (stride > 0xc && (stride - 0xc) % 0xf == 0)
            Footstep(index, Bs001BoneNode((stride - 0xc) % 0x1e != 0 ? BS001_BONE_RIGHT_TOE : BS001_BONE_LEFT_TOE),
                     effectPos, boneMatrix);
        break;
    }
    case 4:
        if (self.motionFrames % 10 == 0)
        {
            Node *foot = Bs001BoneNode(self.motionFrames % 0x14 != 0 ? BS001_BONE_RIGHT_TOE : BS001_BONE_LEFT_TOE);
            if (foot != 0)
                boneMatrix = foot->worldMatrix;
            BossEffect(index, BS001_EFFECT_FOOTSTEP, effectPos, 0, &boneMatrix, 0);
            Vec4 stepPosition = boneMatrix.GetTranslation();
            FUN_10054550(BS001_SOUND_STEP_FIRST, 5, &stepPosition);
        }
        break;
    case BS001_MOTION_FALL:
    {
        int stride = self.motionFrames;
        Node *spine = Bs001BoneNode(BS001_BONE_FOOTSTEPS);
        if (stride < 0x11 || stride > 0x1b || (stride - 0x11) % 5 != 0)
        {
            if (stride == 0x23 || stride == 0x4a)
            {
                FUN_10065b10(index, BS001_SOUND_STEP_SECOND, 1, 0);
                if (spine != 0)
                {
                    boneMatrix = spine->worldMatrix;
                    BossEffect(index, BS001_EFFECT_FOOTSTEP, effectPos, 0, &boneMatrix, 0);
                }
            }
        }
        else
        {
            FUN_10065b10(index, BS001_SOUND_STEP_FIRST, 1, 0);
            if (spine != 0)
            {
                boneMatrix = spine->worldMatrix;
                BossEffect(index, BS001_EFFECT_FOOTSTEP, effectPos, 0, &boneMatrix, 0);
            }
        }
        break;
    }
    case BS001_MOTION_SMASH:
    {
        int swing = model.motionFrame;
        if (swing > 0x244 && (swing & 1) != 0 && swing < BS001_SMASH_FRAME_LAST && Bs001BoneNode(BS001_BONE_SPINE) != 0)
        {
            boneMatrix.SetTranslation(Bs001BoneNode(BS001_BONE_SPINE)->worldMatrix.GetTranslation());
            boneMatrix._43 -= 50.0f;
            BossEffect(index, BS001_EFFECT_SMASH_DUST, effectPos, 0, &boneMatrix, 0);
        }
        if (model.motionFrame == BS001_SMASH_FRAME_LAST)
        {
            FUN_10065b10(index, BS001_SOUND_SMASH_END, 1, 0);
            FUN_1016b230("52915");
            FUN_10055ee0();
            if (Bs001BoneNode(BS001_BONE_SPINE) == 0)
                boneMatrix.SetTranslation(model.matrix.GetTranslation());
            else
                boneMatrix.SetTranslation(Bs001BoneNode(BS001_BONE_SPINE)->worldMatrix.GetTranslation());
            boneMatrix._43 = 0.0f;
            BossEffect(index, BS001_EFFECT_SHOUT_CLOUD, effectPos, 0, &boneMatrix, 0);
            EffectOptions options;
            options.m_0004 = DAT_10daaae8[index].stats[3];
            options.m_000e = 5;
            options.m_0000 = 0;
            BossEffect(index, BS001_EFFECT_SMASH_FLASH, effectPos, 0, &boneMatrix, &options);
        }
        break;
    }
    case 0x15:
    case 0x17:
    {
        int swing = model.motionFrame;
        if (swing == 0x2b7 || swing == 0x2a5)
            StepSound(index, Bs001BoneNode(BS001_BONE_LEFT_TOE), effectPos, boneMatrix);
        else if (swing == 0x2af)
            StepSound(index, Bs001BoneNode(BS001_BONE_RIGHT_TOE), effectPos, boneMatrix);
        break;
    }
    }

    EffectOptions trailOptions;
    if (self.idle == 0 && DAT_102550e0[index] == 0 && DAT_1021f6c0[index] != 0)
    {
        trailOptions.m_0004 = DAT_10daaae8[index].stats[3];
        trailOptions.flags |= 2;
        trailOptions.m_0000 = 0;
        trailOptions.m_000e = 5;
        if (self.motion == 0x14)
        {
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, DAT_1021bdf8[index][BS001_NODE_RIGHT_UPPER_ARM], &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, DAT_1021bdf8[index][BS001_NODE_LEFT_UPPER_ARM], &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, DAT_1021bdf8[index][BS001_NODE_LEFT_HAND], &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, DAT_1021bdf8[index][BS001_NODE_RIGHT_HAND], &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, Bs001BoneNode(BS001_BONE_LEFT_FOOT), &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, Bs001BoneNode(BS001_BONE_RIGHT_FOOT), &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_SMASH, effectPos, Bs001BoneNode(BS001_BONE_SPINE), &trailOptions);
        }
        else if (self.motion != BS001_MOTION_FALL)
        {
            TrailEffect(index, BS001_EFFECT_TRAIL_STANDARD, effectPos, DAT_1021bdf8[index][BS001_NODE_LEFT_HAND], &trailOptions);
            TrailEffect(index, BS001_EFFECT_TRAIL_STANDARD, effectPos, DAT_1021bdf8[index][BS001_NODE_RIGHT_HAND], &trailOptions);
        }
        DAT_1021f6c0[index] = 0;
    }
    if (model.motion->blending == 0 && DAT_10daaae8[index].m_00a8 != 0)
    {
        int layer = model.motion->layer;
        if (partnerModel.motion->layer != layer)
        {
            partnerModel.motion->layer = layer;
            partnerModel.motion->m_0004 = model.motion->m_0004;
            partnerModel.motionFrame = model.motionFrame;
            partnerModel.motion->frame = model.motion->frame;
            if (DAT_1020b430[index + 1].bs001Aux.advancePartner != 0)
                partnerModel.motion->Advance();
        }
    }
    float target = WrapAngle(model.targetHeading);
    float current = WrapAngle(model.heading);
    float turn = WrapAngle(current - target);
    if (DAT_1020b430[index].bs001.state == 0x21b)
        turn = turn / 3.5f;
    else if (turn != 0.0f && DAT_1020b430[index].bs001.state != BS001_LUNGE)
        turn = turn / 2.5f;
    else
        turn = 0.0f;
    if (!(turn == 0.0f && DAT_1020b430[index].bs001.state == BS001_LUNGE))
        model.targetHeading = model.heading - turn;
    if (DAT_1020b430[index + 2].bs001Fade.fadeOut != 0)
        DAT_10daaae8[index].m_0141 = 1;
    return 0;
}
