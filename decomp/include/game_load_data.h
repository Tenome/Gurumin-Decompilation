// Game_LoadData (game_load_data.cpp): the tables it reads from bin\map.txt and the globals and functions that only
// it uses. This is the one declaration of these types and globals: include this header, never redeclare them.
#ifndef GURUMIN_GAME_LOAD_DATA_H
#define GURUMIN_GAME_LOAD_DATA_H

#include <windows.h>

#include "char_slot.h"
#include "map_tables.h"
#include "matrix4.h"
#include "vec4.h"

// One 0xc0-byte record of the character statistics table of map.txt (DAT_10927744), indexed by the character id.
struct MapCharacterStats
{
    int health;               // 0x00: CharSlot::stats[0] and stats[1]
    int m_0004;               // 0x04: CharSlot::stats[2]
    int m_0008;
    int m_000c;
    union
    {
        int m_0010;           // 0x10: CharSlot::stats[5]
        char flagA;           // 0x10: also CharSlot::m_0144
    };
    int m_0014;
    union
    {
        int m_0018;           // 0x18: CharSlot::stats[7]
        char flagB;           // 0x18: also CharSlot::m_0143
    };
    int m_001c;
    int money;                // 0x20: CharSlot::money
    int m_0024;               // 0x24: CharSlot::m_00c0
    int m_0028;               // 0x28: CharSlot::m_00c4
    int m_002c;               // 0x2c: CharSlot::m_00c8
    int m_0030;               // 0x30: CharSlot::m_00ac
    int coins;                // 0x34: CharSlot::coins
    int twoHanded;            // 0x38: CharSlot::twoHanded
    char pad_003c[0x84];
};

extern "C" MapCharacter DAT_10918e60[];
extern "C" MapCharacterStats DAT_10927744[];
extern "C" int DAT_109276e0;                              // number of records in DAT_10918e60
extern "C" int DAT_102108e8[CHAR_SLOT_COUNT];             // per-slot value copied from the map character table

int Game_LoadData(void);                                  // 0x100cb0c0 (game_load_data.cpp); 0 while the data is still loading

extern "C" int DAT_102026d4;                              // non-zero = the render targets still have to be created
extern "C" int DAT_102027fc;                              // texture slot of colorms.bmp
extern "C" char DAT_1030baf8[8][0x20];                    // eight 32-byte records (first byte cleared at load)
extern "C" int DAT_108ed458;                              // version of the map data file (0x898 = the supported one)
extern "C" int DAT_104c0178;                              // non-zero = no sound archive
extern "C" int DAT_104c01bc;
extern "C" int DAT_104c01c0;
extern "C" int DAT_104c01c4;
extern "C" int DAT_104c0200;                              // non-zero = the intro movie is shown although DAT_1032d86c is set
extern "C" char DAT_1032d86c;
extern "C" int DAT_1065cb94;
extern "C" int DAT_1065cb98;

// Number of map objects (nodes DAT_104d2a08[0..99]); the preloaded effect models follow them in the same table.
#define MAP_OBJECT_COUNT 100
// A map object record (0xc0 bytes): the index of its model path (DAT_10894698), 0 = none.
struct MapObject
{
    int model;
    char pad_0004[0xbc];
};
extern "C" MapObject DAT_1088e6f4[MAP_OBJECT_COUNT];

// Indexes of the preloaded models in DAT_104d2a08 (debris.h).
enum PreloadedNode
{
    NODE_BETYO = MAP_OBJECT_COUNT,
    NODE_BETYO2,
    NODE_UNUSED,
    NODE_DRILL2,
    NODE_DRILL3,
    NODE_DRILL4,
    NODE_BETYO3,
    NODE_BETYO4,
    NODE_BETYO5,
    NODE_BETYO6,
    NODE_SPSCOP
};
extern "C" int DAT_10f654e4;                              // 1 = screen-size crossfade targets

// Callbacks of the host (0x1101de40..0x1101de50), set once the data is loaded.
typedef int (*HostCallback)();
extern "C" HostCallback DAT_1101de40[5];

struct EffectOptions;
#include "effect_hit_hook.h"   // FUN_10093cc0, the effect hit test (host callback 0)
struct Vec4;
extern "C" int FUN_1005c250(float command, float a, float b, float c, const Vec4 *position);   // screen effect hook (host callback 1)
extern "C" int FUN_10010040(float x, float y, float z, float variant, EffectOptions *options);   // aim hook (host callback 3, effect_aim.cpp)
extern "C" int FUN_1000ff80();

// Loading_Tick (loading_tick.cpp): shows the progress in DAT_104c0320; also the host's load notification.
extern "C" void FUN_1000fb80(const char *path, int a);
extern "C" int FUN_10012510(int mode, int index);   // attach/remove the drill tool
extern "C" int FUN_10017d30(void);                  // RenderTarget_Probe
extern "C" int FUN_10016580(const char *name);      // Movie_PlayToTexture
extern "C" char *FUN_1013f910(const char *name);    // MakeDataPath
extern "C" int FUN_10001ee0(int enable, const char *dir);   // SoundArc_Init
extern "C" LRESULT FUN_10001660(int first, int second);     // SoundArc_Command230
extern "C" int FUN_100016c0(const char *file, const float *x, const float *y, const float *z, int mode, int loop,
                            float volume);   // SoundArc_PlayFile
extern "C" void FUN_1003e710(void);                 // Game_Reset
extern "C" int FUN_10050460(void);                  // map unload
extern "C" void FUN_1003a9d0(void);                 // Fade_Step
extern "C" int FUN_1000e750(void);                  // AntiTamper_Store
extern "C" void FUN_1000e810(void);
extern "C" void FUN_10199860(void);                 // RenderQueue_Clear
extern "C" void FUN_1003b6e0(int index, int variant);   // Costume_LoadPlayerModel
extern "C" int FUN_10175c90(const char *name, int width, int height, const unsigned char *pixels, int alpha, int a);
extern "C" void FUN_10194360(const Vec4 *eye, const Vec4 *target, Matrix4 *view, Matrix4 *projection, int a);  // Camera_SetMatrices
extern "C" int FUN_100154c0(int value);             // Char_WearHelmet
extern "C" int FUN_10001b40(void);                  // SoundArc_Message221
extern "C" int FUN_1013faf0(void);                  // FrameTimer_ResetNext2
extern "C" void FUN_1000dba0(void);                 // Fade_Draw
extern "C" int FUN_101618f0(void);                  // TextureSlots_MarkKept

#endif
