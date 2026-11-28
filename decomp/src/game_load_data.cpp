// Loading the game's data (Game_LoadData 0x100cb0c0), called by the frame step while a game load is pending: creates
// the render targets, reads the map tables, plays the intro movie while the models load, preloads the effect and
// weapon models and textures and starts the game.

#include <stdio.h>
#include <string.h>

#include "anim_model.h"
#include "char_slot.h"
#include "debris.h"
#include "dinput_min.h"
#include "effect_manager.h"
#include "font_renderer.h"
#include "game.h"
#include "mmv_player.h"
#include "model_bank.h"
#include "node.h"
#include "texture_load.h"
#include "texture_manager.h"
#include "texture_object.h"
#include "tracker.h"
#include "vec4.h"

#include "matrix4.h"

#include "game_load_data.h"
#include "camera_vars.h"
#include "char_tables.h"
#include "char_vars.h"
#include "frame_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "menu_scene.h"
#include "text_window.h"
#include "map_placement.h"
#include "render_state.h"
#include "render_vars.h"
#include "scene_command.h"
#include "sound_vars.h"
#include "ui_vars.h"
#include "win_vars.h"

extern "C" void FUN_1014b480(void);      // Render_ResetSwapTarget
extern "C" int FUN_101439e0(void);       // Render_ResetTarget
extern "C" int FUN_10001b40(void);       // SoundArc_Message221

// The original copies its strings with an open-coded loop (the intrinsic strcpy compares the character differently).
#define CopyText(destination, source)     {         int index = 0;         char character;         do         {             character = (source)[index];             (destination)[index] = character;             index++;         } while (character != 0);     }

// Loading progress (DAT_104c0320, percent) at each step.
#define PROGRESS_TARGETS 20.0f
#define PROGRESS_SOUND 30.0f
#define PROGRESS_MAP 40.0f
#define PROGRESS_TABLES 50.0f
#define PROGRESS_OBJECTS 60.0f
#define PROGRESS_WEAPONS 70.0f
#define PROGRESS_TUTORIAL 75.0f
#define PROGRESS_EFFECTS 80.0f
#define PROGRESS_EFFECTS_2 82.0f
#define PROGRESS_EFFECTS_3 84.0f
#define PROGRESS_EFFECTS_4 87.0f
#define PROGRESS_EFFECTS_5 88.0f
#define PROGRESS_EFFECTS_6 89.0f
#define PROGRESS_COSTUME 90.0f
#define PROGRESS_PLAYER 95.0f
#define PROGRESS_DONE 100.0f

// Version of the map data file this build reads.
#define MAP_DATA_VERSION 0x898

// Intro movie: frame on which the sound starts, frame on which the fade out starts, last frame, frame from which no
// new movie frame is decoded.
#define MOVIE_SOUND_FRAME 1
#define MOVIE_FADE_FRAME 0x96
#define MOVIE_LAST_FRAME 0xb6
#define MOVIE_DECODE_END 0x7f
#define MOVIE_FRAME_BOX_END 0xb4
#define MOVIE_SOUND_VOLUME 0.8f
#define MOVIE_SOUND_VOLUME_QUIET 0.52f

// The camera and the model of the intro movie.
#define MOVIE_CAMERA_X 200
#define MOVIE_MODEL_Z -10
#define MOVIE_CAMERA_OFFSET_X 1300.0f
#define MOVIE_CAMERA_Y 100.0f
#define MOVIE_MODEL_OFFSET_Z 40.0f

// Size of the movie texture and its place on the 640x480 screen.
#define MOVIE_TEXTURE_WIDTH 1024.0f
#define MOVIE_TEXTURE_HEIGHT 512.0f
#define SCREEN_CENTRE_X 0x140
#define SCREEN_CENTRE_Y 0xdc

// Messages shown when the load fails (Shift-JIS): "the script file version is wrong.", "creating the render
// textures failed.", "s064 initialisation failed." and "the effect file version is wrong.".
static const char MESSAGE_SCRIPT_VERSION[] = "\x83X\x83N\x83\x8a\x83v\x83g\x83t\x83@\x83""C\x83\x8b\x82\xcc\x83o\x81[\x83W\x83\x87\x83\x93\x82\xaa\x88\xd9\x8f\xed\x81""B";
static const char MESSAGE_RENDER_TEXTURES[] = "\x83\x8c\x83\x93\x83_\x83\x8a\x83\x93\x83O\x97p\x83""e\x83N\x83X\x83`\x83\x83\x81[\x82\xcc\x8d\xec\x90\xac\x8e\xb8\x94s\x81""B";
static const char MESSAGE_S064[] = "s064\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x81""B";
static const char MESSAGE_EFFECT_VERSION[] = "\x83G\x83t\x83""F\x83N\x83g\x83t\x83@\x83""C\x83\x8b\x82\xcc\x83o\x81[\x83W\x83\x87\x83\x93\x82\xaa\x88\xd9\x8f\xed\x81""B";

// 0x100cb0c0 (Game_LoadData). Runs when a game load is pending (DAT_102026d4): creates the render targets and the sound
// archive on the first call, then loads bin\map.txt and copies its character tables into the character slots, plays
// the intro movie (logo.mmv with the draw.it3 model, any key skips it) while the textures decode, preloads the map
// object, weapon, tutorial and effect models, starts the game (Game_Reset) and reports its progress through
// FUN_1000fb80. Returns 1 when everything loaded; 0 when the window was closed or a render step failed (the message is
// then shown until a key is pressed). Not byte-identical yet: see open.tsv.
int Game_LoadData(void)
{
    if (DAT_102026d4 != 0)
    {
        FUN_1013f070(0xc, 1, 0);
        FUN_1013f070(0x13, 1, 0);
        FUN_1013f070(9, 1, 0);
        FUN_1013f070(0, 0, 0);
        FUN_1013f070(3, 0, 0);
        FUN_10017d30();
        if (DAT_10f654cc == 0)
        {
            DAT_10203434 = 0;
            DAT_10203438 = 0;
            DAT_102022b8 = 1;
            DAT_10f654e4 = 0;
            DAT_102022c4 = 0;
            DAT_10203418 = 0;
            DAT_102027f0 = 0;
        }
        else if (DAT_10804bf8.CreateTextures() != 0)
        {
            DAT_104c0320 = PROGRESS_TARGETS;
            if (DAT_10f65484 != 0 || DAT_102027f0 == 0)
                DAT_10f655a0 = FUN_10173550("blue.bmp", 0, 0);
            DAT_102027fc = FUN_10173550("colorms.bmp", 0, 0);
            DAT_104c0320 = PROGRESS_SOUND;
            if (DAT_104c0178 == 0 && DAT_104c01b0 == 0)
            {
                FUN_10001ee0((int)DAT_10f6569c, FUN_1013f910("\bin"));
                if (DAT_104c01b0 == 0)
                    FUN_10001660(DAT_104c01bc, DAT_104c01c0 + DAT_104c01c4 * 10);
            }
            DAT_104c0320 = PROGRESS_SOUND;
            FUN_1000fb80(0, 0);
            goto loadData;
        }
        DAT_10836c70.PaceFrame();
        DAT_10836c70.PresentFrame();
        if (DAT_10f654cc == 0)
        {
            CopyText(&DAT_104bfec0, MESSAGE_RENDER_TEXTURES);
        }
        else
        {
            CopyText(&DAT_104bfec0, MESSAGE_S064);
        }
        DAT_10210f30 = 0;
        char keyPressed = 0;
        while (DAT_10836c70.PumpAndRecover() != 0 && keyPressed == 0 && DAT_10210f30 == 0)
        {
            DAT_1020d9e0.M_10001550();
            DAT_1020d9e0.M_10001110(0, &keyPressed, 0, 0, 0, 0);
            DAT_10f08008.Update();
            DAT_10210f30 = DAT_10f08008.ReadControls(0);
            FUN_101439e0();
            FUN_1013f070(4, 0, 0);
            DAT_1020e538.DrawText(0x8d, 0xc1, &DAT_104bfec0, 0xffffff, 0.0f, 1.0f, 1.2f);
            DAT_10836c70.PaceFrame();
            DAT_10836c70.PresentSimple();
        }
        return 0;
    }
loadData:
    if (DAT_10f6552c == 0)
    {
        for (int i = 0; i < 8; i++)
            DAT_1030baf8[i][0] = 0;
        DAT_102026d0 = 0;
        DAT_10874848.LoadMapData(FUN_1013f910("bin\\map.txt"), FUN_1000fb80);
        if (DAT_108ed458 != MAP_DATA_VERSION)
        {
            CopyText(&DAT_104bfec0, MESSAGE_SCRIPT_VERSION);
        }
        DAT_104c0320 = PROGRESS_MAP;
        FUN_1000fb80(0, 0);
        for (int i = 0; i < DAT_109276e0; i++)
        {
            int id = DAT_10918e60[i].id;
            CharSlot &slot = DAT_10daaae8[id];
            sprintf(slot.m_0165, DAT_10918e60[i].label);
            DAT_1021f3c0[id] = DAT_10918e60[i].slotKind;
            slot.speedScale = (float)DAT_10918e60[i].speedPercent / 100.0f;
            CopyText(DAT_1021a3c0[id], DAT_10918e60[i].modelNode);
            DAT_102108e8[id] = DAT_10918e60[i].m_00b4;
            DAT_102192c0[id] = DAT_10918e60[i].m_00b0;
            slot.faceSet = DAT_10918e60[i].faceSet;
            slot.kind = DAT_10918e60[i].kind;
            slot.m_00a0 = DAT_10918e60[i].m_008c;
            slot.params[2] = DAT_10918e60[i].params[2];
            slot.params[0] = DAT_10918e60[i].params[0];
            slot.params[1] = DAT_10918e60[i].params[1];
            slot.params[3] = DAT_10918e60[i].params[3];
            CopyText(slot.name, DAT_10918e60[i].name);
            slot.m_0144 = DAT_10927744[id].flagA;
            slot.m_0143 = DAT_10927744[id].flagB;
            slot.stats[0] = slot.stats[1] = DAT_10927744[id].health;
            slot.stats[2] = DAT_10927744[id].m_0004;
            slot.stats[3] = DAT_10927744[id].m_0008;
            slot.stats[4] = DAT_10927744[id].m_000c;
            slot.stats[5] = DAT_10927744[id].m_0010;
            slot.stats[6] = DAT_10927744[id].m_0014;
            slot.stats[7] = DAT_10927744[id].m_0018;
            slot.money = DAT_10927744[id].money;
            slot.m_00c0 = DAT_10927744[id].m_0024;
            slot.m_00c4 = DAT_10927744[id].m_0028;
            slot.m_00c8 = DAT_10927744[id].m_002c;
            slot.coins = DAT_10927744[id].coins;
            slot.m_00ac = DAT_10927744[id].m_0030;
            slot.twoHanded = DAT_10927744[id].twoHanded;
        }
        DAT_104c0320 = PROGRESS_TABLES;
        FUN_1000fb80(0, 0);
        FUN_1000e750();
        FUN_1000e810();
        DAT_10202478 = FUN_10175c90("videotex", 0x400, 0x200, 0, 0, 0);
        if (DAT_104c0180 == 0 && DAT_1065cb98 == 0 && DAT_1065cb94 == 0 && (DAT_1032d86c == 0 || DAT_104c0200 != 0))
        {
            FUN_10016580("logo.mmv");
            int cameraX = MOVIE_CAMERA_X;
            int modelZ = MOVIE_MODEL_Z;
            int player = PLAYER_SLOT;
            DAT_10c655f0 = 7;
            FUN_1003b6e0(player, 0);
            DAT_10c655f0 = 0;
            FUN_10012510(2, player);
            Matrix4 view;
            Matrix4 camera;
            Matrix4 projection;
            Vec4 eye;
            Vec4 target;
            target.Fill(0.0f);
            eye.Set((float)cameraX - MOVIE_CAMERA_OFFSET_X, 0.0f, MOVIE_CAMERA_Y, 0.0f);
            FUN_10194360(&eye, &target, &camera, &projection, 0);
            view = camera * projection;
            AnimModel *model = &DAT_10db6ee8[player];
            DAT_104bfe70 = 0;
            DAT_104bf8f0 = 100.0f;
            DAT_104bf8e8 = 0.0f;
            DAT_104bf8ec = 0;
            DAT_1032d4b0 = -10.0f / 3.0f;
            DAT_104bf8e4 = 0x1e;
            model->LoadModel(FUN_1013f8a0("draw.it3", 0));
            FUN_1013faf0();
            DAT_10f65538 = 0;
            if (DAT_10836c70.PumpAndRecover() != 0)
            {
                while (true)
                {
                    FUN_1013f070(4, 0, 0);
                    FUN_1013f070(0xc, 1, 0);
                    FUN_1013f070(0x13, 0, 0);
                    FUN_1013f070(9, 1, 0);
                    FUN_1013f070(0, 0, 0);
                    FUN_1013f070(3, 0, 0);
                    FUN_1013f070(10, 0, 0);
                    FUN_1013f070(0, 0, 0);
                    FUN_1013f070(2, 0, 0);
                    FUN_1013f070(7, 1, 0);
                    FUN_1013f070(3, 1, 0xf);
                    FUN_10199860();
                    if (DAT_10f6552c != 0)
                        break;
                    if (DAT_10f65504 != DAT_104bf8b8)
                    {
                        DAT_1020d9e0.M_10001550();
                        char keyPressed = 0;
                        DAT_1020d9e0.M_10001110(0, &keyPressed, 0, 0, 0, 0);
                        FUN_1003a9d0();
                        DAT_104bf8b8 = DAT_10f65504;
                        DAT_10f08008.Update();
                        DAT_10210f30 = DAT_10f08008.ReadControls(0);
                        if (DAT_10f65484 == 0)
                            FUN_1014b480();
                        else
                            FUN_101439e0();
                        if (DAT_104bfe70 == MOVIE_SOUND_FRAME)
                        {
                            float volume = DAT_104bfe6c != 0 ? MOVIE_SOUND_VOLUME : MOVIE_SOUND_VOLUME_QUIET;
                            if (DAT_104c01b0 == 0)
                            {
                                char file[0x100];
                                sprintf(file, "wav\\%s.%s", "grogo", &DAT_108db428);
                                FUN_100016c0(FUN_1013f910(file), 0, 0, 0, 0, 0, volume);
                            }
                        }
                        if (DAT_104bfe70 == MOVIE_FADE_FRAME)
                        {
                            DAT_104bf8e8 = 100.0f;
                            DAT_104bf8ec = 0;
                            DAT_104bf8e4 = 0x1e;
                            DAT_1032d4b0 = (100.0f - DAT_104bf8f0) / 30.0f;
                        }
                        else if (DAT_104bfe70 < MOVIE_DECODE_END)
                        {
                            DAT_10f08ab0.M_10161050();
                            DAT_10f08ab0.DrawToTexture(&DAT_118d0ba8[DAT_10202478]);
                        }
                        DAT_104bfe70 = DAT_104bfe70 + 1;
                        if (DAT_10210f30 != 0 || keyPressed != 0 || DAT_104bfe70 > MOVIE_LAST_FRAME)
                            break;
                        model->SetPosition(0.0f, 0.0f, (float)modelZ - MOVIE_MODEL_OFFSET_Z);
                        model->motionFrame = DAT_104bfe70 > 0 ? DAT_104bfe70 : 0;
                        model->m_0294 = 1;
                        model->UpdateFrame(&view, 1, 1, 0, 0);
                    }
                    int width = DAT_10f08af0;
                    int height = DAT_10f08af2;
                    int left = SCREEN_CENTRE_X - width / 2;
                    int top = SCREEN_CENTRE_Y - height / 2;
                    if (DAT_104bfe70 < MOVIE_FRAME_BOX_END)
                        DAT_118d0ba8[DAT_10202478].DrawTinted(left, top, left + width, top + height, 0.001953125f, 0.00390625f,
                                                              (float)(width - 2) / MOVIE_TEXTURE_WIDTH,
                                                              (float)(height - 2) / MOVIE_TEXTURE_HEIGHT, 0.99f, 1.0f, 0.0f, 0.0f);
                    if (model->motionFrame > 0)
                    {
                        Vec4 *lights = DAT_10804bf8.LightDirection(0, &eye, &target);
                        model->Draw(&view, lights, 0);
                    }
                    FUN_1000dba0();
                    DAT_10836c70.PaceFrame();
                    DAT_10836c70.PresentFrame();
                    if (DAT_10836c70.PumpAndRecover() == 0)
                        break;
                }
            }
            DAT_10f65538 = FUN_1000fb80;
            FUN_10012510(0, 0);
            model->Unload();
            DAT_1032d4b0 = -DAT_104bf8f0 / 29.0f;
            DAT_104bf8e8 = 0.0f;
            DAT_104bf8ec = 0;
            DAT_104bf8e4 = 0x1d;
        }
        if (DAT_104c01b0 == 0)
        {
            for (int i = 0; i < 0x44c; i++)
                DAT_10447338[i] = -11;
            if (FUN_10001b40() != 0)
                FUN_10001b40();
        }
        if (DAT_10f6552c != 0)
            return 0;
        for (int pass = 0; pass < 2; pass++)
        {
            if (DAT_10f65484 == 0)
                FUN_1014b480();
            else
                FUN_101439e0();
            FUN_1013f070(4, 0, 0);
            DAT_10836c70.PaceFrame();
            DAT_10836c70.PresentFrame();
            DAT_10836c70.PumpAndRecover();
        }
        for (int i = 0; i < CHAR_SLOT_COUNT; i++)
        {
            DAT_10db6ee8[i].Teleport(-200000.0f, 0.0f, 0.0f);
            DAT_10db6ee8[i].M_10161b60();
            DAT_10db6ee8[i].M_10161a60();
        }
        DAT_118d0ba8[DAT_108360f4].UpdateImage(0);
        FUN_1013f070(6, 0, 0);
        FUN_1013f070(4, 0xc8c8fa, 0);
        if (DAT_10f65484 == 0)
            FUN_1014b480();
        else
            FUN_101439e0();
        FUN_1013f070(6, 0, 0);
        DAT_104c0320 = PROGRESS_OBJECTS;
        FUN_1000fb80(0, 0);
        for (int i = 0; i < MAP_OBJECT_COUNT; i++)
        {
            if (DAT_1088e6f4[i].model != 0)
            {
                DAT_104d2a08[i].Load(FUN_1013f8a0(DAT_10894698[DAT_1088e6f4[i].model], 0), 0, 0, 0);
                DAT_104d2a08[i].M_101ab980();
                DAT_104d2a08[i].BuildMesh(1, 0);
            }
        }
        DAT_104c0320 = PROGRESS_WEAPONS;
        FUN_1000fb80(0, 0);
        DAT_104d2a08[NODE_BETYO].Load(FUN_1013f8a0("betyo.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_BETYO].BuildMesh(1, 0);
        DAT_104d2a08[NODE_BETYO2].Load(FUN_1013f8a0("betyo2.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_BETYO2].BuildMesh(1, 0);
        DAT_104d2a08[NODE_DRILL2].Load(FUN_1013f8a0("drill2.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_DRILL2].M_1019b050(8);
        DAT_104d2a08[NODE_DRILL2].BuildMesh(1, 0);
        DAT_104d2a08[NODE_DRILL3].Load(FUN_1013f8a0("drill3.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_DRILL3].M_1019b050(8);
        DAT_104d2a08[NODE_DRILL3].BuildMesh(1, 0);
        DAT_104d2a08[NODE_DRILL4].Load(FUN_1013f8a0("drill4.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_DRILL4].M_1019b050(8);
        DAT_104d2a08[NODE_DRILL4].BuildMesh(1, 0);
        DAT_104d2a08[NODE_BETYO3].Load(FUN_1013f8a0("betyo3.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_BETYO3].BuildMesh(1, 0);
        DAT_104d2a08[NODE_BETYO4].Load(FUN_1013f8a0("betyo4.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_BETYO4].BuildMesh(1, 0);
        DAT_104d2a08[NODE_BETYO5].Load(FUN_1013f8a0("betyo5.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_BETYO5].BuildMesh(1, 0);
        DAT_104d2a08[NODE_BETYO6].Load(FUN_1013f8a0("betyo6.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_BETYO6].BuildMesh(1, 0);
        DAT_104d2a08[NODE_SPSCOP].Load(FUN_1013f8a0("spscop.it3", 0), 0, 0, 0);
        DAT_104d2a08[NODE_SPSCOP].M_1019b050(8);
        DAT_104d2a08[NODE_SPSCOP].BuildMesh(1, 0);
        DAT_104c0320 = PROGRESS_TUTORIAL;
        FUN_1000fb80(0, 0);
        FUN_10174ce0(FUN_1013f8a0("tuti1.iti", 0), 0, 0);
        FUN_10174ce0(FUN_1013f8a0("tuti2.iti", 0), 0, 0);
        FUN_10174ce0(FUN_1013f8a0("tuti3.iti", 0), 0, 0);
        DAT_10db4408 = DAT_10daaae8[0];
        DAT_108654f8.LoadIt3(FUN_1013f8a0("ita.it3", 0), 0, 0);
        DAT_108654f8.Rebuild();
        DAT_108654f8.BuildMesh(1, 0);
        DAT_104c0320 = PROGRESS_EFFECTS;
        FUN_1000fb80(0, 0);
        DAT_10daaa80.ClearNodePool();
        DAT_10daaa80.Init();
        if (DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("effects.efx", 0)) == 0)
        {
            CopyText(&DAT_104bfec0, MESSAGE_EFFECT_VERSION);
        }
        DAT_104c0320 = PROGRESS_EFFECTS_2;
        FUN_1000fb80(0, 0);
        DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("tokaron.efx", 0));
        DAT_104c0320 = PROGRESS_EFFECTS_3;
        FUN_1000fb80(0, 0);
        DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("rashi5.efx", 0));
        DAT_104c0320 = PROGRESS_EFFECTS_4;
        FUN_1000fb80(0, 0);
        DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("ef_hou.efx", 0));
        DAT_104c0320 = PROGRESS_EFFECTS_5;
        FUN_1000fb80(0, 0);
        DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("ef_hakai.efx", 0));
        DAT_104c0320 = PROGRESS_EFFECTS_6;
        FUN_1000fb80(0, 0);
        DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("kigae.efx", 0));
        DAT_10daaa80.LoadDefinitions(FUN_1013f8a0("ef_oka.efx", 0));
        DAT_104c0320 = PROGRESS_COSTUME;
        FUN_1000fb80(0, 0);
        DAT_1101de40[0] = (HostCallback)FUN_10093cc0;
        DAT_1101de40[1] = (HostCallback)FUN_1005c250;
        DAT_1101de40[3] = FUN_10010040;
        DAT_1101de40[4] = FUN_1000ff80;
        if (DAT_102026d4 != 0)
        {
            FUN_101618f0();
            DAT_102026d4 = 0;
        }
        FUN_1003b6e0(0, 0);
        FUN_100154c0(DAT_10daaae8[0].helmet);
        if (DAT_10db6ee8[0].root != 0)
        {
            DAT_10db6ee8[0].root->M_101946b0(0x1f, 0xffffffff);
            DAT_10db6ee8[0].root->M_10194730(0x1f, 0xffffffff);
        }
        DAT_104c0320 = PROGRESS_PLAYER;
        FUN_1000fb80(0, 0);
        DAT_10db6ee8[0].m_0080 = 1;
        DAT_104bf8c0 = 0;
        FUN_1003e710();
        FUN_10050460();
        DAT_10f65538 = 0;
        DAT_104c0320 = PROGRESS_DONE;
        FUN_1000fb80(0, 0);
        return 1;
    }
    return 0;
}
