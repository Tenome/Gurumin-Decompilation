// The footstep callback of the animated models: plays the step sound for the surface the player walks on and starts
// the dust, splash or skid effect that goes with it. Game_Main installs it in DAT_1101d854; AnimModel::Step calls it
// each time a foot touches the ground.

#include "vec4.h"
#include "matrix4.h"
#include "effect_manager.h"
#include "char_vars.h"
#include "doppler_loops.h"
#include "game_frame.h"
#include "game_main.h"
#include "game_vars.h"
#include "map_setup.h"
#include "ui_vars.h"

// No steps are heard during the first frames of a run or while a script scene with these ids plays.
#define FOOTSTEP_FIRST_FRAME 15
#define FOOTSTEP_SILENT_SCENE_FIRST 30000
#define FOOTSTEP_SILENT_SCENE_END 31000

// Step sounds: four variants (the footstep counter picks one) per surface; plain ground uses the variants' own ids.
#define FOOTSTEP_VARIANTS 4
#define FOOTSTEP_SOUND_SURFACE_D0 0x227   // while DAT_104c01d0 counts down
#define FOOTSTEP_SOUND_SURFACE_CC 0x25c   // while DAT_104c01cc counts down
#define FOOTSTEP_SOUND_SURFACE_C8 0x258   // while DAT_104c01c8 counts down
#define FOOTSTEP_SOUND_SURFACE_D4 0x3c0   // while DAT_104c01d4 counts down

// Effects started with a step.
#define FOOTSTEP_EFFECT_DUST 0x2da        // the plain dust puff (also used while DAT_104c01d4 counts down)
#define FOOTSTEP_EFFECT_SURFACE_CC 0x2e0
#define FOOTSTEP_EFFECT_SURFACE_C8 0x2dc
#define FOOTSTEP_EFFECT_SKID 0x88bd       // placed with a matrix turned against the player's heading
#define FOOTSTEP_SKID_DEPTH -5.0f         // its offset along the matrix's z axis
#define FOOTSTEP_SKID_TURN 3.1415f        // added to the heading: the skid points backwards

#define FOOTSTEP_VOLUME 1.0f

// 0x1005c2e0. Does nothing in the first FOOTSTEP_FIRST_FRAME frames, during the silent scenes, while sounds are muted
// (DAT_104c0318 or DAT_104bfe6c) and outside the playing mode. Otherwise advances the footstep counter DAT_104bfafc
// and, depending on which surface timer is running, plays that surface's variant of the step sound at `position`
// and starts its effect there (the DAT_104c01c8 surface also starts a skid effect behind the player). `a` and `b` are
// the callers' sound numbers and are not used. Called by AnimModel::Step.
void Footstep_Play(int a, int b, Vec4 *position)
{
    if (DAT_104bf890 < FOOTSTEP_FIRST_FRAME)
        return;
    if (DAT_104c00d0 >= FOOTSTEP_SILENT_SCENE_FIRST && DAT_104c00d0 < FOOTSTEP_SILENT_SCENE_END)
        return;
    if (DAT_104c0318 != 0)
        return;
    if (DAT_104bfe6c != 0)
        return;
    if (DAT_104bf8c0 != GAME_MODE_PLAY)
        return;
    DAT_104bfafc++;
    if (DAT_104c01d0 != 0)
        FUN_1003ad20(FOOTSTEP_SOUND_SURFACE_D0 + (DAT_104bfafc & (FOOTSTEP_VARIANTS - 1)), position, FOOTSTEP_VOLUME);
    else if (DAT_104c01d8 != 0)
        FUN_1003ad20(DAT_104bfafc & (FOOTSTEP_VARIANTS - 1), position, FOOTSTEP_VOLUME);
    else if (DAT_104c01cc != 0)
    {
        Vec4 at;
        at.Copy(*position);
        DAT_10daaa80.Start(FOOTSTEP_EFFECT_SURFACE_CC, at, -1, 0, 0, 0, 0, 0, 0, 0);
        FUN_1003ad20(FOOTSTEP_SOUND_SURFACE_CC + (DAT_104bfafc & (FOOTSTEP_VARIANTS - 1)), position, FOOTSTEP_VOLUME);
    }
    else if (DAT_104c01c8 != 0)
    {
        Vec4 at;
        at.Copy(*position);
        DAT_10daaa80.Start(FOOTSTEP_EFFECT_SURFACE_C8, at, -1, 0, 0, 0, 0, 0, 0, 0);
        Matrix4 skidMatrix;
        skidMatrix = RotationZ(DAT_10db9364 + FOOTSTEP_SKID_TURN);
        skidMatrix.Translate(at.x, at.y, at.z);
        at.Fill(0.0f);
        at.z = FOOTSTEP_SKID_DEPTH;
        DAT_10daaa80.Start(FOOTSTEP_EFFECT_SKID, at, -1, 0, 0, 0, 0, &skidMatrix, 0, 0);
        FUN_1003ad20(FOOTSTEP_SOUND_SURFACE_C8 + (DAT_104bfafc & (FOOTSTEP_VARIANTS - 1)), position, FOOTSTEP_VOLUME);
    }
    else if (DAT_104c01d4 != 0)
    {
        Vec4 at;
        at.Copy(*position);
        DAT_10daaa80.Start(FOOTSTEP_EFFECT_DUST, at, -1, 0, 0, 0, 0, 0, 0, 0);
        FUN_1003ad20(FOOTSTEP_SOUND_SURFACE_D4 + (DAT_104bfafc & (FOOTSTEP_VARIANTS - 1)), position, FOOTSTEP_VOLUME);
    }
    else
    {
        Vec4 at;
        at.Copy(*position);
        DAT_10daaa80.Start(FOOTSTEP_EFFECT_DUST, at, -1, 0, 0, 0, 0, 0, 0, 0);
        FUN_1003ad20(DAT_104bfafc & (FOOTSTEP_VARIANTS - 1), position, FOOTSTEP_VOLUME);
    }
}
