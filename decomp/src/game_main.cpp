// The game's entry point: start-up, main loop and shutdown (GuruMain jumps to Game_Main).

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_main.h"
#include "menu_sound.h"

#include "action_select_vars.h"
#include "bgm_track.h"
#include "camera_vars.h"
#include "field_item_menu.h"
#include "map_placement.h"
#include "texture_load.h"
#include "char_slot.h"
#include "char_state.h"
#include "char_tables.h"
#include "char_vars.h"
#include "debris.h"
#include "dinput_min.h"
#include "effect_manager.h"
#include "font_renderer.h"
#include "game.h"
#include "game_file.h"
#include "game_load_data.h"
#include "game_vars.h"
#include "ground_height.h"
#include "map_setup.h"
#include "map_vars.h"
#include "matrix4.h"
#include "menu_main.h"
#include "menu_save_load.h"
#include "menu_scene.h"
#include "mesh_shader.h"
#include "mmv_player.h"
#include "model_load_vars.h"
#include "render_state.h"
#include "render_vars.h"
#include "scene_draw_vars.h"
#include "scene_reset.h"
#include "script_system.h"
#include "shop_menu.h"
#include "sound_vars.h"
#include "texture_load_iti.h"
#include "texture_manager.h"
#include "ui_vars.h"
#include "vec4.h"
#include "win_vars.h"
#include "world_renderer.h"
#include "frame_vars.h"
#include "item_dictionary.h"
#include "render_overlay.h"
#include "demo_recording.h"
#include "tracker.h"
#include "anim_model.h"
#include "model_bank.h"

#define MEGABYTES(count) ((count) << 20)

// Loading bar values shown while the data files are checked.
#define PROGRESS_APP_DLL 2.0f
#define PROGRESS_APP8_DLL 4.0f
#define PROGRESS_MAP_ITM 6.0f
#define PROGRESS_SOUND3D_DLL 7.0f
#define PROGRESS_TEST_DAT 8.0f
#define PROGRESS_GAME_EXE 9.0f
#define PROGRESS_SYSTEM_SAV 10.0f
#define PROGRESS_DONE 15.0f

#define PROJECTION_NEAR 20.0f
#define PROJECTION_FAR 10000.0f
#define PROJECTION_FIELD_OF_VIEW 0.785375f
#define PROJECTION_ASPECT 1.333f
#define PREVIEW_NEAR 200.0f
#define PREVIEW_FAR 45000.0f
#define SKY_FAR 90000.0f

#define NOISE_SLOTS 16
#define CURSOR_OFFSET 0x30

#define READ_CONFIG(key) FUN_1015fe70(DAT_10f65370, "CONFIG", key, &value)
#define READ_KEY(key, destination) FUN_1015fe70(DAT_10f65370, "KEYCONFIG", key, destination)

// Writes a random value into a random slot of the anti-tamper noise table.
#define TAMPER_NOISE()                                                         \
    do                                                                         \
    {                                                                          \
        int noise = rand() > 1 ? rand() : 1;                                   \
        DAT_104c0270[rand() % NOISE_SLOTS] = noise;                            \
    } while (0)

// Checks the data file `format` (relative to the game folder) against record `record` of soundarc.dll; a mismatch
// drops noise into the anti-tamper table. Then advances the loading bar to `progress`.
#define VERIFY_DATA_FILE(format, record, progress)                                                         \
    sprintf(checkedPath, format, DAT_10255d08);                                                            \
    sprintf(tableDll, "%sbin\\soundarc.dll", DAT_10255d08);                                                \
    FUN_101602d0(checkedPath, &sum, &check);                                                               \
    FUN_10160360(tableDll, DATA_CHECK_TABLE_OFFSET, record, &expectedSum, &expectedCheck);                 \
    if (expectedSum != sum || expectedCheck != check)                                                      \
        TAMPER_NOISE();                                                                                    \
    DAT_104c0320 = progress;                                                                               \
    FUN_1000fb80(0, 0);

static __forceinline void ClearTarget()
{
    FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
    FUN_101439e0();
    FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
    FUN_1013f070(RENDER_CLEAR, 0, 0);
    FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
}

static __forceinline void ClearTargets()
{
    FUN_101439e0();
    FUN_1013f070(RENDER_CLEAR, 0, 0);
    FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
    FUN_1014b480();
    FUN_1013f070(RENDER_CLEAR, 0, 0);
    FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
}

// Sets the quit flag when any entry of the anti-tamper noise table is non-zero.
static __forceinline void QuitIfNoiseTableUsed()
{
    for (int first = 0; first < NOISE_SLOTS; first += 8)
    {
        if (DAT_104c0270[first + 0] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 1] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 2] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 3] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 4] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 5] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 6] != 0)
            DAT_10f6552c = 1;
        if (DAT_104c0270[first + 7] != 0)
            DAT_10f6552c = 1;
    }
}

// Clears the game state for a new session and unloads every character model.
static __forceinline void ClearSession()
{
    FUN_1003e710();
    FUN_10050460();
    int previousBank = DAT_104c0184;
    memset(DAT_1021f3c0, 0, sizeof(DAT_1021f3c0));
    memset(DAT_1020e338, 0, CHAR_SLOT_COUNT * sizeof(Matrix4 *));
    memset(DAT_1021a3c0, 0, sizeof(DAT_1021a3c0));
    memset(DAT_102192c0, 0, sizeof(DAT_102192c0));
    DAT_104bfd54 = 0;
    DAT_104bfd58 = 0;
    DAT_10202358 = 0xff;
    DAT_104bfd5c = 0;
    DAT_104c0114 = 0;
    DAT_104c0118 = 0;
    DAT_104c0119 = 0;
    DAT_104c011a = 0;
    DAT_104c011b = 0;
    DAT_104c011c = 0;
    DAT_104c0120 = 0;
    DAT_104c0121 = 0;
    DAT_104c0124 = 0;
    DAT_104c0128 = 0;
    DAT_104c012c = 0;
    DAT_104c0130 = 0;
    DAT_104c0131 = 0;
    DAT_104c0132 = 0;
    DAT_104c0133 = 0;
    DAT_104c0156 = 0;
    DAT_104c0158[0] = 0;
    DAT_104c0158[1] = 0;
    DAT_104c0158[2] = 0;
    DAT_104c0158[3] = 0;
    DAT_104c015c = 0;
    DAT_104c0160 = 0;
    DAT_104c0164 = 0;
    DAT_104c0168 = 0;
    DAT_104c016c = 0;
    DAT_104c0170 = 0;
    DAT_104c0171 = 0;
    DAT_104c0172 = 0;
    DAT_104c0173 = 0;
    DAT_104c0188 = previousBank;
    DAT_104c0184 = 0;
    DAT_102026d0 = 1;
    DAT_104bf8c0 = GAME_MODE_NEW_GAME;
    DAT_104bf8bc = 0;
    for (int i = 0; i < CHAR_SLOT_COUNT; i++)
        DAT_10db6ee8[i].Unload();
}

// Puts the player at the saved position and clears the fade and the per-session character state.
static __forceinline void EnterSavedPosition()
{
    DAT_104bfd71 = 1;
    FUN_1003e710();
    FUN_10050460();
    DAT_10c62ed0 = DAT_1030b660;
    DAT_104bf8f0 = 100.0f;
    DAT_104bf8e8 = 0.0f;
    DAT_104bf8ec = 0;
    DAT_1032d4b0 = -3.3333333f;
    DAT_104bf8e4 = 30;
    DAT_10db6ee8[0].Teleport(DAT_108029b8.x, DAT_108029b8.y, DAT_108029b8.z);
    DAT_10db6ee8[0].heading = DAT_108029b8.w;
    DAT_10db6ee8[0].targetHeading = DAT_108029b8.w;
    DAT_104bfe4c = 0;
    DAT_104c0300 = 0;
    DAT_104bfe50 = 0;
    DAT_104c00b0 = 0;
    DAT_102024bc = -1;
    DAT_104bfe30 = 0;
    DAT_10db6ee8[0].PlayMotion(5, 1, 0, 1.0f);
    DAT_10db6ee8[0].PlayMotion(2, 1, 0, 1.0f);
    DAT_104686b0[0] = 0;
    DAT_10db6ee8[0].alpha = 1.0f;
    DAT_10db6ee8[0].mode = 1;
    DAT_104bfeb0 = 0;
    DAT_104bfeb4 = 0;
    FUN_1013fb10(0, 0, 0);
}

// Clears the player's guard state for a started demo.
static __forceinline void ClearPlayerGuards()
{
    DAT_104c0128 = 0;
    DAT_10253e98[0] = 0;
    DAT_10db6ee8[0].m_0075 = 0;
    DAT_104bfe78 = 0;
}

// Starts the map the demo is recorded on (or mode 15 for map 999).
static __forceinline void StartDemoMap()
{
    DAT_104bfe80 = 3;
    DAT_10202b20 = 3;
    DAT_104bfd18 = 1;
    if (DAT_10c62ed0 == MAP_NONE)
    {
        DAT_104bf8c0 = GAME_MODE_15;
        DAT_104bf8bc = 0;
        DAT_104bfd50 = 1;
        DAT_1020d9e0.M_10001550();
        DAT_1065cb28.M_1013edb0();
        Script_ValueCommand(0x271b, 10);
        DAT_10c6356c = 0;
        Script_ValueCommand(0x2774, 0);
        Script_ValueCommand(0x1869f, 0);
        DAT_10c63518 = 0;
    }
    else
    {
        FUN_100fc4a0(DAT_10c62ed0, 0, 0);
        DAT_104bfd50 = 1;
        DAT_104c00d0 = 0;
        DAT_104bf8c0 = GAME_MODE_PLAY;
        DAT_104bf8bc = 0;
    }
}

// 0x1013b700 (Game_Main). The program's main(): reads config.ini and the key bindings, creates the window and the
// device, checks the data files against soundarc.dll, builds the projection matrices, then runs Game_FrameStep until
// it reports the end and releases everything. C++ linkage on purpose: the GameFile and Matrix4 temporaries give the
// function an exception frame.
// Returns 0, or 0x1f5 when DAT_104c033d is 1.
int Game_Main(HINSTANCE instance, const char *commandLine, HINSTANCE moduleInstance)
{
    DAT_104c0180 = 0;
    int debugMode = 0;
    if (strstr(commandLine, "/DEBUG") != 0)
    {
        debugMode = 1;
        DAT_104c0150 = 1;
    }

    {
    char userPathText[0x100];
    userPathText[0] = 0;
    char executableFolder[0x100];
    GetModuleFileNameA(0, executableFolder, 0x100);
    char *end = executableFolder + strlen(executableFolder) - 1;
    while (*end != '\\')
    {
        *end = 0;
        end--;
    }
    char pathFileName[0x100];
    sprintf(pathFileName, "%spath.txt", executableFolder);
    {
        GameFile pathFile;
        pathFile.Open(pathFileName, "rb");
        if (pathFile.handle != 0)
        {
            pathFile.Read(userPathText, 1, 0x100);
            pathFile.Close();
        }
    }
    if (userPathText[0] == 0)
    {
        GetModuleFileNameA(0, DAT_10255d08, 0x100);
        char *folderEnd = DAT_10255d08 + strlen(DAT_10255d08) - 1;
        while (*folderEnd != '\\')
        {
            *folderEnd = 0;
            folderEnd--;
        }
    }
    else
        strcpy(DAT_10255d08, userPathText);
    FUN_1013f950(DAT_10255d08);
    }

    {
    char dataFolder[0x100];
    strcpy(dataFolder, DAT_10255d08);
    strcat(dataFolder, "3ddata");
    FUN_1013f8f0(dataFolder, 0);
    }

    DAT_1101d878 = 1;
    DAT_1101d854 = Footstep_Play;
    DAT_10f140c8 = moduleInstance;
    char userFolder[0x100];
    char userName[0x100];
    DWORD userNameSize = 0x100;
    userName[0] = 0;
    if (!GetUserNameA(userName, &userNameSize))
        userName[0] = 0;

    char appDllPath[0x100];
    sprintf(appDllPath, "%sbin\\app.dll", DAT_10255d08);
    if (FUN_1013dfc0(userFolder, userName, "GURUMIN", appDllPath) < 0)
    {
        if (GetUserNameA(userName, &userNameSize))
        {
            sprintf(userFolder, "%sSaveData", DAT_10255d08, userName);
            CreateDirectoryA(userFolder, 0);
            sprintf(userFolder, "%sSaveData\\%s", DAT_10255d08, userName);
            CreateDirectoryA(userFolder, 0);
        }
        else
        {
            sprintf(userFolder, "%sSaveData", DAT_10255d08, userName);
            CreateDirectoryA(userFolder, 0);
            sprintf(userFolder, "%sSaveData\\Default", DAT_10255d08, userName);
            CreateDirectoryA(userFolder, 0);
        }
    }
    else
        FUN_1013e0c0(userFolder);

    sprintf(DAT_10f65370, "%s\\config.ini", userFolder);
    FUN_1000beb0();
    FILE *configFile = fopen(DAT_10f65370, "rb");
    if (configFile != 0)
        fclose(configFile);
    else
        sprintf(DAT_10f65370, "%sbin\\gurumin.def", DAT_10255d08);
    sprintf(DAT_10f60590, "%s\\", userFolder);
    int value = 0;
    sprintf(DAT_10256110, "%ssystem.sav", DAT_10f60590);
    sprintf(DAT_10253f88, "%sbin\\sound.itm", DAT_10255d08);

    READ_CONFIG("VRAM");
    if (value == 0)
        DAT_1020340c = MEGABYTES(128);
    else if (value == 1)
        DAT_1020340c = MEGABYTES(4);
    else if (value == 2)
        DAT_1020340c = MEGABYTES(8);
    else if (value == 3)
        DAT_1020340c = MEGABYTES(12);
    else if (value == 4)
        DAT_1020340c = MEGABYTES(16);
    else if (value == 5)
        DAT_1020340c = MEGABYTES(24);
    READ_CONFIG("VRAM2");
    if (value == 0)
        DAT_10203410 = MEGABYTES(128);
    else if (value == 1)
        DAT_10203410 = MEGABYTES(2);
    else if (value == 2)
        DAT_10203410 = MEGABYTES(4);
    else if (value == 3)
        DAT_10203410 = MEGABYTES(8);
    else if (value == 4)
        DAT_10203410 = MEGABYTES(12);
    DAT_104c02e8 = 0;
    FUN_1015fe70(DAT_10f65370, "CONFIG", "LSF", &DAT_104c02e8);
    READ_CONFIG("RESOLUTION");
    switch (value)
    {
    case 2:
        DAT_10202b24 = 640;
        DAT_10202b28 = 480;
        DAT_10f654ec = 0x400;
        break;
    case 1:
        DAT_10202b24 = 640;
        DAT_10202b28 = 480;
        DAT_10f654ec = 0x200;
        break;
    case 4:
        DAT_10202b24 = 800;
        DAT_10202b28 = 600;
        DAT_10f654ec = 0x400;
        break;
    case 6:
        DAT_10202b24 = 1280;
        DAT_10202b28 = 960;
        DAT_10f654ec = 0x400;
        break;
    case 0:
        DAT_10202b24 = 640;
        DAT_10202b28 = 480;
        DAT_10f654ec = 0x100;
        break;
    case 3:
        DAT_10202b24 = 800;
        DAT_10202b28 = 600;
        DAT_10f654ec = 0x200;
        break;
    case 5:
        DAT_10202b24 = 1280;
        DAT_10202b28 = 960;
        DAT_10f654ec = 0x200;
        break;
    }
    READ_CONFIG("FULLSCREEN");
    DAT_10f65514 = value == 0;
    READ_CONFIG("TEX4");
    DAT_1101d840 = value;
    READ_CONFIG("ANTI");
    DAT_10f654c8 = value;
    READ_CONFIG("HDVS");
    DAT_10f65550 = value;
    READ_CONFIG("TEXD");
    DAT_1101d844 = value;
    READ_CONFIG("SOUND");
    DAT_104c01b0 = value;
    READ_CONFIG("DOF");
    DAT_10f654e4 = value;
    READ_CONFIG("NOAVI");
    DAT_104c0340 = value;
    READ_CONFIG("MIRR");
    DAT_102022c4 = value;
    DAT_10203418 = value;
    READ_CONFIG("FOG");
    DAT_10202b10 = value;
    READ_CONFIG("FADE");
    DAT_10203344 = value;
    if (value != 0)
        DAT_10202b0c = 2;
    READ_CONFIG("EFFECT");
    DAT_104bfe80 = value;
    DAT_10202b20 = value;
    DAT_10f6558c = value >= 2;
    READ_CONFIG("BLUR");
    DAT_10f654f4 = value;
    if (value != 0)
        DAT_10202b0c = 2;
    READ_CONFIG("CMAKE");
    READ_CONFIG("NOAUTOUP");
    DAT_104c017c = value == 0;
    READ_CONFIG("TIEN");
    DAT_104c00c0 = (short)(value * 44100 / 1000);
    READ_CONFIG("GAMMA");
    DAT_10f65518 = value;
    READ_CONFIG("GAMMA2");
    DAT_10f6551c = value;
    READ_CONFIG("WROT");
    DAT_104c010f = value;
    READ_CONFIG("WJUMP2");
    DAT_104c010d = value;
    READ_CONFIG("WJUMP3");
    DAT_104c010e = value;
    READ_CONFIG("KANDO");
    DAT_10203408 = (unsigned char)value;
    READ_CONFIG("KANDO2");
    DAT_10f65560 = value - 0x32;
    READ_CONFIG("NOUDXI");
    DAT_10f65574 = value;
    READ_CONFIG("USEGAMMA");
    DAT_10f654a0 = value;
    READ_CONFIG("BGMDOUKI");
    DAT_104c014c = value;
    READ_CONFIG("VSK");
    DAT_10f654b0 = value == 0;
    READ_CONFIG("VSYNC");
    DAT_10f654a8 = value;
    READ_CONFIG("USEBKB");
    DAT_10f65484 = value == 0;
    READ_CONFIG("BGMVOL");
    {
        int volume = value / 10;
        if (volume < 10)
        {
            if (volume < 1)
                DAT_102027d8 = 1;
            else
                DAT_102027d8 = volume;
        }
        if (volume >= 10)
            DAT_102027d8 = 10;
    }
    READ_CONFIG("SEVOL");
    READ_CONFIG("SOUND3D");
    DAT_104c01b8 = value;
    READ_CONFIG("SOUND3D2");
    DAT_104c01c4 = value;
    READ_CONFIG("SOUNDTP");
    DAT_104c01bc = value;
    READ_CONFIG("SOUNDTP2");
    DAT_104c01c0 = value;
    READ_CONFIG("USE32");
    DAT_10f654a4 = value;
    READ_CONFIG("USETKUSE");
    DAT_102027f0 = value;
    DAT_10202b1c = value;
    READ_CONFIG("USEVOICE");
    DAT_104c0332 = value == 0;
    DAT_1101d870 = value == 0;
    READ_CONFIG("SHADOW");
    if (value == 0)
    {
        DAT_10203434 = 0;
        DAT_10203438 = 0;
        DAT_102022b8 = 1;
    }
    else if (value == 1)
    {
        DAT_10203434 = 0;
        DAT_102022b8 = 1;
        DAT_10203438 = 1;
    }
    else if (value == 2)
    {
        DAT_102022b8 = 1;
        DAT_10203434 = 1;
        DAT_10203438 = 1;
    }
    else if (value == 3)
    {
        DAT_102022b8 = 0;
        DAT_10203434 = 1;
        DAT_10203438 = 1;
    }
    READ_CONFIG("USEJP");
    DAT_10f65570 = value;
    READ_CONFIG("USEVB");
    DAT_10f65564 = value;
    READ_CONFIG("EKEY1");
    int axisZ = value;
    READ_CONFIG("EKEY2");
    int axisRx = value;
    READ_CONFIG("EKEY3");
    int axisRy = value;
    READ_CONFIG("EKEY4");
    int axisRz = value;
    DAT_10f6556c = (((axisRy << 1 | axisRx) << 1 | axisRz) << 1) | axisZ;

    READ_KEY("KEY01", &DAT_10f60c30[13]);
    READ_KEY("KEY02", &DAT_10f60c30[1]);
    READ_KEY("KEY03", &DAT_10f60c30[2]);
    READ_KEY("KEY04", &DAT_10f60c30[3]);
    READ_KEY("KEY05", &DAT_10f60c30[4]);
    READ_KEY("KEY07", &DAT_10f60c30[6]);
    READ_KEY("KEY08", &DAT_10f60c30[7]);
    READ_KEY("KEY09", &DAT_10f60c30[8]);
    READ_KEY("KEY10", &DAT_10f60c30[9]);
    READ_KEY("KEY11", &DAT_10f60c30[0]);
    READ_KEY("KEY12", &DAT_10f60c30[5]);
    READ_KEY("KEY13", &DAT_10f60c30[10]);
    READ_KEY("KEY14", &DAT_10f60c30[11]);
    READ_KEY("KEY15", &DAT_10f60c30[12]);
    READ_KEY("PAD01", &DAT_10f1bef0[13]);
    READ_KEY("PAD02", &DAT_10f1bef0[1]);
    READ_KEY("PAD03", &DAT_10f1bef0[2]);
    READ_KEY("PAD04", &DAT_10f1bef0[3]);
    READ_KEY("PAD05", &DAT_10f1bef0[4]);
    READ_KEY("PAD07", &DAT_10f1bef0[6]);
    READ_KEY("PAD08", &DAT_10f1bef0[7]);
    READ_KEY("PAD09", &DAT_10f1bef0[8]);
    READ_KEY("PAD10", &DAT_10f1bef0[9]);
    READ_KEY("PAD11", &DAT_10f1bef0[0]);
    READ_KEY("PAD12", &DAT_10f1bef0[5]);
    READ_KEY("PAD13", &DAT_10f1bef0[10]);
    READ_KEY("PAD14", &DAT_10f1bef0[11]);
    READ_KEY("PAD15", &DAT_10f1bef0[12]);

    FUN_101b2670();
    DAT_10f08008.Update();
    DAT_10f657c0 = LoadIconA(instance, MAKEINTRESOURCE(0x66));
    DAT_10f65554 = 1;
    if (FUN_10161970() == 0)
    {
        MessageBoxA(0, "DirectX\x82\xcc\x8f\x89\x8a\xfa\x89\xbb\x82\xc9\x8e\xb8\x94s\x82\xb5\x82\xdc\x82\xb5\x82\xbd\x81"
                       "B",
                    "\x83G\x83\x89\x81[", MB_ICONERROR);
        return 0;
    }
    RECT windowRect;
    GetWindowRect(DAT_10f6569c, &windowRect);
    SetCursorPos(windowRect.left + CURSOR_OFFSET, windowRect.top + CURSOR_OFFSET);
    SetWindowTextA(DAT_10f6569c, "\x82\xae\x82\xe9\x82\xdd\x82\xf1 Ver.1.2a.D9");
    ShowWindow(DAT_10f6569c, SW_SHOW);
    FUN_1013eeb0();

    memset(DAT_10daaae8, 0, sizeof(DAT_10daaae8));
    FUN_1015fda0((char *)&DAT_10836c70);
    memset(DAT_11af3fa8, 0xff, 0xc000);
    DAT_1020e538.Init((HWND)&DAT_10836c70, 0);
    FUN_101649c0();
    DAT_1101d878 = 1;
    FUN_1016a8a0(DAT_10db6ee8);
    DAT_104b7200.LoadMenuTextures(&DAT_1020e538);
    DAT_104be850 = &DAT_104bfb00;
    DAT_104b7898 = &DAT_1020d9e0;
    FUN_1013f070(RENDER_FOG, 0, 0);
    FUN_1013ad90();
    FUN_1019bdb0();
    DAT_1020d9e0.M_10001010((int)DAT_10f6569c, (int)FUN_1000fd50, (int)FUN_1013e6e0);
    memset(DAT_1030c500, 0, sizeof(DAT_1030c500));
    for (int i = 0; i < 0x40; i++)
        DAT_10edbe0c[i].mesh = 0;
    DAT_10202b08 = 0;
    DAT_10f654cc = 1;

    RECT presentRect;
    if (DAT_10f65484 != 0)
    {
        if (DAT_10836c70.PumpAndRecover() != 0)
        {
            ClearTarget();
            DAT_10836c70.PaceFrame();
            DAT_10836c70.PresentFrame();
        }
        if (DAT_10836c70.PumpAndRecover() != 0)
        {
            ClearTarget();
            DAT_10836c70.PaceFrame();
            DAT_10836c70.PresentFrame();
        }
        if (DAT_10836c70.PumpAndRecover() != 0)
        {
            ClearTarget();
            DAT_10836c70.PaceFrame();
            presentRect.left = 0;
            presentRect.top = 0;
            presentRect.right = DAT_10202b24;
            presentRect.bottom = DAT_10202b28;
            DAT_10836c70.Present(&presentRect);
        }
        if (DAT_10836c70.PumpAndRecover() != 0)
        {
            DAT_10836c70.PumpAndRecover();
            ClearTarget();
            DAT_10836c70.PaceFrame();
            presentRect.left = 0;
            presentRect.top = 0;
            presentRect.right = DAT_10202b24;
            presentRect.bottom = DAT_10202b28;
            DAT_10836c70.Present(&presentRect);
        }
    }
    else
    {
        DAT_10836c70.PumpAndRecover();
        ClearTargets();
        DAT_10836c70.PresentFrame();
        DAT_10836c70.PumpAndRecover();
        ClearTargets();
        DAT_10836c70.PresentFrame();
    }
    DAT_10836c70.PumpAndRecover();

    DAT_104c0320 = 0;
    DAT_10f65538 = FUN_1000fb80;
    int sum;
    unsigned int check;
    int expectedSum;
    int expectedCheck;
    char checkedPath[0x100];
    char tableDll[0x100];
    VERIFY_DATA_FILE("%sbin\\app.dll", 0, PROGRESS_APP_DLL)
    VERIFY_DATA_FILE("%sbin\\app8.dll", 1, PROGRESS_APP8_DLL)
    VERIFY_DATA_FILE("%sbin\\map.itm", 2, PROGRESS_MAP_ITM)
    VERIFY_DATA_FILE("%sbin\\sound3d.dll", 3, PROGRESS_SOUND3D_DLL)
    VERIFY_DATA_FILE("%s3ddata\\test.dat", 4, PROGRESS_TEST_DAT)
    VERIFY_DATA_FILE("%sgame.exe", 5, PROGRESS_GAME_EXE)

    int costumeBits = 0;
    int result = FUN_101603c0(DAT_10256110, SYSTEM_RECORD_COSTUMES_MAIN, &costumeBits);
    DAT_104c0230 = (unsigned short)costumeBits;
    if (result == -1)
    {
        DAT_104c0230 = 0;
        DeleteFileA(DAT_10256110);
        FUN_1015fea0(DAT_10256110, SYSTEM_RECORD_COSTUMES_MAIN, (unsigned short)DAT_104c0230);
    }
    DAT_104c0320 = PROGRESS_SYSTEM_SAV;
    FUN_1000fb80(0, 0);
    if (FUN_101603c0(DAT_10253f88, SYSTEM_RECORD_COSTUMES_MAIN, &costumeBits) == -1)
        TAMPER_NOISE();
    DAT_104c0320 = PROGRESS_DONE;
    FUN_1000fb80(0, 0);

    DAT_10866da8 = Multiply(Scaling(1.0f, PROJECTION_ASPECT, 1.0f),
                            Perspective(PROJECTION_NEAR, PROJECTION_FAR, PROJECTION_FIELD_OF_VIEW));
    DAT_104c03f8 = Multiply(Scaling(1.0f, PROJECTION_ASPECT, 1.0f),
                            Perspective(PREVIEW_NEAR, SKY_FAR, PROJECTION_FIELD_OF_VIEW));
    Vec4 eye;
    Vec4 target;
    Vec4 up;
    eye.Set(50.0f, -400.0f, 200.0f, 0.0f);
    target.Set(0.0f, 0.0f, 0.0f, 0.0f);
    up.Set(0.0f, 1.0f, 0.0f, 0.0f);
    DAT_10836430 = Multiply(LookAt(eye, target, &up, 0),
                            Multiply(Scaling(1.0f, PROJECTION_ASPECT, 1.0f),
                                     Perspective(PREVIEW_NEAR, PREVIEW_FAR, PROJECTION_FIELD_OF_VIEW)));
    DAT_10836430 = Multiply(DAT_10836430, TranslationMatrix(0.8f, -0.76f, 0.0f));

    DAT_1032d86c = debugMode;
    DAT_104b7894 = &DAT_1065cb28;
    DAT_104be860 = FUN_1003b160;
    DAT_11b044fc = FUN_10091e10;
    DAT_102026d0 = 1;
    DAT_102026d4 = 1;
    DAT_104c0174 = 0;
    DAT_104bfd84 = 0;
    memset(DAT_1020d860, 0, sizeof(DAT_1020d860));
    DAT_104bf8c0 = GAME_MODE_NEW_GAME;
    DAT_104c0178 = 0;

    char demoName[0x100];
    for (;;)
    {
        if (DAT_10f65554 > 0)
            DAT_10f65554--;
        if (DAT_104bfd84 != 0)
        {
            FUN_1003e710();
            FUN_10050460();
        }
        if (DAT_104c0200 != 0 && DAT_1065cb90 != 0 && DAT_1065cb9c == DAT_1065cb28.frameCount)
        {
            Bgm_Play(0, 0);
            DAT_104c012c = 0;
            FUN_1003e710();
            FUN_10050460();
            DAT_104c0200 = 0;
            DAT_1065cb9c = 0;
            DAT_1065cb90 = 0;
            DAT_1065cb98 = 0;
            DAT_104bf8c0 = GAME_MODE_DEMO_ENDED;
        }
        else if (DAT_104bf8c0 != GAME_MODE_TITLE && DAT_104bf8c0 != GAME_MODE_DEMO_ENDED)
            goto demoChecked;
        if (DAT_104c0180 != 0 && (DAT_1065cb90 == 0 || DAT_1065cb9c == DAT_1065cb28.frameCount))
        {
            ClearSession();
            if (FUN_10137b10() == 0)
                break;
            EnterSavedPosition();
            ClearPlayerGuards();
            DAT_104c0200 = 1;
            DAT_104bf8c0 = GAME_MODE_PLAY;
            DAT_104bf8bc = 0;
            DAT_1065cb94 = 0;
            DAT_1065cb98 = 1;
            DAT_1065cb90 = 0;
            {
                sprintf(demoName, "%s.dat", DAT_102026d8);
                DAT_1065cb28.Load(FUN_1013f8a0(demoName, 0));
                sprintf(demoName, "%s.sav", DAT_102026d8);
                FUN_1003b2c0(FUN_1013f8a0(demoName, 0), 0);
            }
            StartDemoMap();
        }
    demoChecked:
        if (DAT_104c0200 == 2)
        {
            int previousMode = DAT_104bf8c0;
            DAT_104c0200 = 1;
            ClearSession();
            if (FUN_10137b10() == 0)
                break;
            EnterSavedPosition();
            ClearPlayerGuards();
            DAT_104bf8c0 = previousMode;
            DAT_1065cb94 = 0;
            DAT_1065cb98 = 1;
            DAT_1065cb90 = 0;
            if (DAT_104c0200 != 0)
            {
                sprintf(demoName, "%s.dat", DAT_102026d8);
                DAT_1065cb28.Load(FUN_1013f8a0(demoName, 0));
                sprintf(demoName, "%s.sav", DAT_102026d8);
                FUN_1003b2c0(FUN_1013f8a0(demoName, 0), 0);
            }
            else
            {
                DAT_1065cb28.Load("demo.dat");
                FUN_1003b2c0("demo.sav", 0);
            }
            StartDemoMap();
        }

        if (DAT_1021cff8 != 0)
        {
            if (DAT_1021cff8 == 1)
            {
                if (DAT_103086a8 == 0)
                    FUN_100fc4a0(DAT_1042de20, 0, DAT_1030c900);
                else
                    FUN_100fc4a0(DAT_1042de20, DAT_10255e08, DAT_1030c900);
            }
            if (DAT_1021cff8 == 2)
                FUN_100fda40(DAT_1042de20);
            DAT_1021cff8 = 0;
        }
        if (DAT_104bfe1c == 0)
            DAT_10874848.RunQueuedEvents(Event_RunTrigger);
        FUN_100fc3e0();
        if (FUN_10137b10() == 0)
            break;

        QuitIfNoiseTableUsed();
        if (rand() % 300 < 10 && DAT_104c034c != 0)
            TAMPER_NOISE();
        if (rand() % 300 < 10 && DAT_104c0344 != 0)
            TAMPER_NOISE();
        if (rand() % 340 < 13 && DAT_104c0348 != 0)
            TAMPER_NOISE();
        if (rand() % 320 < 11 && DAT_104c0344 != 0)
            DAT_10f6552c = 1;
        if (rand() % 330 < 11 && DAT_104c0348 != 0)
            DAT_10f6552c = 1;
        if (rand() % 330 < 11 && DAT_104c034c != 0)
            DAT_10f6552c = 1;
        if (rand() % 360 < 14 && DAT_104c0344 != 0)
            PostMessageA(DAT_10f6569c, WM_CLOSE, 0, 0);
        if (rand() % 370 < 12 && DAT_104c0348 != 0)
            PostMessageA(DAT_10f6569c, WM_CLOSE, 0, 0);
        if (rand() % 370 < 12 && DAT_104c034c != 0)
            PostMessageA(DAT_10f6569c, WM_CLOSE, 0, 0);
        if (rand() % 470 < 12)
            QuitIfNoiseTableUsed();
        if (rand() % 570 < 12)
        {
            for (int i = 0; i < NOISE_SLOTS; i++)
            {
                if (DAT_104c0270[i] != 0)
                    PostMessageA(DAT_10f6569c, WM_CLOSE, 0, 0);
            }
        }
    }

    FUN_1013eed0();
    DAT_10daaa80.M_1016fab0();
    DAT_10c68000.Release();
    FUN_10023d30();
    FUN_10001f30();
    for (int i = 0; i < PRELOADED_NODE_COUNT; i++)
        DAT_104d2a08[i].Release();
    FUN_1000d9d0();
    DAT_1020e538.Release();
    for (int i = 0; i < CHAR_SLOT_COUNT; i++)
        DAT_10db6ee8[i].Unload();
    for (int i = 0; i < MAP_MODEL_COUNT; i++)
        DAT_104c4c38[i].Release();
    DAT_108654f8.Release();
    DAT_104c4c38[MAP_MODEL_COUNT].Release();
    DAT_10f08008.Shutdown();
    FUN_101a5860();
    DAT_10836c70.M_10175e70();
    DAT_10f08ab0.Close();
    return DAT_104c033d == 1 ? 0x1f5 : 0;
}
