// Game_Main (src/game_main.cpp, 0x1013b700): the globals and engine calls that only the game's start-up, main loop and
// shutdown use. This is the one declaration of each of them: include this header, never redeclare one.
#ifndef GURUMIN_GAME_MAIN_H
#define GURUMIN_GAME_MAIN_H

#include <windows.h>

#include "anim_model.h"
#include "game.h"
#include "vec4.h"
#include "demo_recording.h"
#include "tracker.h"
#include "anim_draw_vars.h"
#include "anim_step_vars.h"
#include "bgm_spectrum.h"
#include "game_load_data.h"
#include "node_collision.h"
#include "script_game_builtin.h"

// The game's entry point (GuruMain 0x1013dfb0 jumps here). `instance` is the icon's module, `commandLine` may hold
// "/DEBUG" and `moduleInstance` is kept as the module instance. Reads the settings, creates the device and window,
// checks the data files, builds the projection matrices, then runs the main loop until the frame step reports the end
// of the game and releases everything. Returns 0, or 0x1f5 when DAT_104c033d is 1.
int Game_Main(HINSTANCE instance, const char *commandLine, HINSTANCE moduleInstance);

// Game modes (DAT_104bf8c0) that Game_Main sets or tests (the modes the frame step runs are in frame_step.cpp).
enum GameMainMode
{
    GAME_MODE_TITLE = 0,       // the title screen
    GAME_MODE_PLAY = 1,        // playing a map
    GAME_MODE_15 = 0xf,        // set when the map to enter is 999
    GAME_MODE_NEW_GAME = 0x32, // set while a new session is cleared
    GAME_MODE_DEMO_ENDED = 0x63   // set when a demo has run to its last frame
};

// The map number that is not a map: entering it starts GAME_MODE_15 instead of setting up a map.
#define MAP_NONE 999

// Models released at shutdown: the preloaded nodes DAT_104c4c38[0..7] are the map models, the nodes in DAT_104d2a08 end
// with NODE_SPSCOP.
#define MAP_MODEL_COUNT 8
#define PRELOADED_NODE_COUNT (NODE_SPSCOP + 1)

// Record of system.sav (and of the sound.itm copy) that holds the costume unlock bits.
#define SYSTEM_RECORD_COSTUMES_MAIN 0x80

// Base of the anti-tamper records in soundarc.dll (File_ReadRecord): record N holds the check values of data file N.
#define DATA_CHECK_TABLE_OFFSET 0x17000

extern "C" char DAT_10255d08[0x100];   // the game's folder, with the trailing backslash (from path.txt or the program)
extern "C" char DAT_10253f88[0x100];   // path of bin\sound.itm
extern "C" char DAT_10255e08[0x100];   // name of the marker node the map setup starts at (Map_SetupByNumber)

extern "C" DemoRecording *DAT_104b7894;    // the input recorder
extern "C" Tracker *DAT_104b7898;          // the mouse and key tracker
extern "C" int *DAT_104be850;              // the game's first state word (DAT_104bfb00)
extern "C" int (*DAT_104be860)(int event); // the sound event callback (0x1003b160)

extern "C" char DAT_104c010d;   // WJUMP2 setting
extern "C" char DAT_104c010e;   // WJUMP3 setting
extern "C" char DAT_104c010f;   // WROT setting
extern "C" int DAT_104c0150;    // 1 when the program was started with /DEBUG
extern "C" int DAT_104c0174;
extern "C" int DAT_104c017c;    // 1 = automatic updates off (NOAUTOUP is 0)
extern "C" int DAT_104c0184;
extern "C" int DAT_104c0188;    // DAT_104c0184 as it was when the session was cleared
extern "C" char DAT_104c033d;   // 1 = the program returns 0x1f5
extern "C" int DAT_10f654a0;    // BGMDOUKI setting (music sync)
extern "C" int DAT_10f654ec;    // size of the off-screen render targets (0x100, 0x200 or 0x400)
extern "C" int DAT_10f65518;    // GAMMA setting
extern "C" int DAT_10f6551c;    // GAMMA2 setting
extern "C" HICON DAT_10f657c0;  // the window icon

// Engine calls that have no header yet.
extern "C" void FUN_101b2670(void);                             // Packs_Load: loads bin\packs.dll
extern "C" int FUN_1013dfc0(char *path, const char *user, const char *suffix, const char *base);   // Path_UserData
extern "C" int FUN_1013e0c0(const char *path);                  // Path_CreateDirectories
extern "C" void FUN_1013f8f0(const char *text, int index);      // SetStringSlot
extern "C" void FUN_1013f950(const char *dir);                  // SetDataDir
extern "C" int FUN_1000beb0(void);                              // TestDat_Load
extern "C" int FUN_1013eeb0(void);                              // TextureManager: FrameTimer_Reset (`this` unused)
extern "C" int FUN_1013eed0(void);                              // TextureManager: Engine_StubTrue (`this` unused)
extern "C" int FUN_10161970(void);                              // TextureManager: Render_InitDevice (`this` unused)
extern "C" int FUN_101649c0(void);                              // ModelBank: Init (`this` unused)
extern "C" int FUN_1015fda0(char *buffer);                      // TextInput_Begin
extern "C" int FUN_1016a8a0(AnimModel *target);                 // Scene_SetParameter: the model the camera follows
extern "C" void FUN_1019bdb0(void);                             // NodePool_Clear
extern "C" int FUN_101a5860(void);                              // NodePool_ReleaseAll
extern "C" int FUN_10001f30(void);                              // SoundArc_Shutdown
extern "C" int FUN_1000d9d0(void);                              // MeshSlots_ReleaseAll
extern "C" int FUN_10023d30(void);                              // Map_Unload
extern "C" int FUN_100fc3e0(void);                              // Script_RunQueued
extern "C" int FUN_10137b10(void);                              // Game_FrameStep (frame_step.cpp)
extern "C" int FUN_1013fb10(int active, int target, int frames);   // FrameBlend_Start
int Event_RunTrigger(GameTrigger *trigger);                     // 0x101008e0 (event_trigger_run.cpp, C++ linkage: EH frame)
void Footstep_Play(int a, int b, Vec4 *position);                // 0x1005c2e0 (footstep_callback.cpp, C++ linkage: EH frame)
extern "C" void __cdecl FUN_10091e10(int event, int code, int flag);   // model contact callback
extern "C" int FUN_101602d0(const char *path, int *sum, unsigned int *check);   // File_Checksum
extern "C" int FUN_10160360(const char *path, int base, int index, void *first, void *second);   // File_ReadRecord

extern "C" int FUN_1015fe70(const char *file, const char *section, const char *key, int *value);   // Config_GetInt
extern "C" void FUN_1013ad90(void);   // ListBank_Reset
extern "C" int FUN_1015fea0(const char *path, int record, unsigned int value);   // SystemSav_WriteRecord
int FUN_100fc4a0(int map, const char *startMarker, int heightOffset);   // Map_SetupByNumber
int FUN_100fda40(int map);
int FUN_1003b2c0(const char *path, int mode);   // Save_Load (a demo's .sav)   // Map_SetupEntry

#endif
