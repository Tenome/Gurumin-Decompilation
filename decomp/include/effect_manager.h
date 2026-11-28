// The effect manager (DAT_10daaa80): starts, steps, draws and removes the running particle and sprite effects.
// Methods: char_effects.cpp (Reset, Init, Update, Remove*, ClearTables, Draw), effect_start.cpp (Start),
// effect_sprites.cpp (DrawSpriteA/B); EffectOptions' constructor is in model_state.cpp.
// This is the one declaration of these types: include this header, never redeclare them.
#ifndef GURUMIN_EFFECT_MANAGER_H
#define GURUMIN_EFFECT_MANAGER_H

#include "screen_vertex.h"

#include "vec4.h"
#include "matrix4.h"
#include "key_track.h"

// Number of effect lists (render groups) the manager keeps.
#define EFFECT_GROUP_COUNT 6
// Number of effect lists the running records are linked into (DAT_1101d820 / DAT_11018604).
#define EFFECT_LIST_COUNT 5
// Number of effect definitions (DAT_1101dfdc), running effect records (DAT_1115cfe8), sprite records in each
// sprite table (DAT_11134cc8, DAT_118a8888) and effect lights (DAT_118e99f8).
#define EFFECT_DEFINITION_COUNT 0x47e
#define EFFECT_RECORD_COUNT 0x348
#define EFFECT_SPRITE_COUNT 0x348
#define EFFECT_LIGHT_COUNT 0x80
// The two lights after the manager's own that DrawRecord builds its ribbons in (kinds 8/9 and 0x17). The ribbon
// of kinds 8/9 keeps its jitter offsets in the second half of its points (entry RIBBON_JITTER_BASE + i, i >= 1).
#define EFFECT_LIGHT_RIBBON 0x80
#define EFFECT_LIGHT_STREAK 0x81
#define RIBBON_JITTER_BASE 0x7f
// A free definition, record or sprite has id (or type) -1; -1 also ends a record list.
#define EFFECT_NONE (-1)
// EffectDef::kind values EffectManager::DrawLight tells apart (1 and 0x12 are sprites). Kinds 7, 9, 0x16 and 0x17
// are drawn by their own functions; kind 0x11 is the trail made of two interleaved ribbons; any other kind with a
// light is drawn as a plain ribbon strip.
#define EFFECT_KIND_LIGHT_7 7
#define EFFECT_KIND_LIGHT_9 9
#define EFFECT_KIND_LIGHT_INTERLEAVED 0x11
#define EFFECT_KIND_LIGHT_22 0x16
#define EFFECT_KIND_LIGHT_23 0x17
// Bits of EffectOptions::flags as the running record's update pass reads them (char_effects.cpp).
#define EFFECT_FLAG_RESTRICTED_PASS 1   // also updated in Update's restricted pass (the followAttachment bit)
#define EFFECT_FLAG_HIDDEN 2            // only updated when Update includes hidden effects (the followOwner bit)

// Options a caller may pass to EffectManager::Start (0x44 bytes, copied into the effect's record). The constructor
// (0x101668b0, model_state.cpp) fills in the defaults.
struct EffectOptions
{
    int m_0000;
    int m_0004;                                      // 0x04: 0xa2c2a by default
    float m_0008;                                    // 0x08: 1.0 by default
    unsigned char list;                              // 0x0c: effect list when the definition leaves it open
    union                                            // 0x0d
    {
        unsigned char flags;                         // all flags (switch_effects.cpp stores 1 = follow attachment)
        struct
        {
            unsigned char followAttachment : 1;      // bit 0: the effect follows the matrix it was attached to
            unsigned char followOwner : 1;           // bit 1: the effect follows its owner (npc_frozen.cpp)
            unsigned char m_000d_2 : 6;
        };
    };
    char m_000e;
    char pad_000f;
    union                                            // 0x10
    {
        int m_0010;
        float angle;                                 // 0x10: roll of the quad kinds 5, 0xb and 0x13 (a record reads it as a float)
        struct
        {
            char m_0010_0;
            char m_0010_1;
            char drawn;                              // 0x12: in a running record: 1 once drawn this frame (list 1)
            char m_0010_3;
        };
    };
    int m_0014;                                      // 0x14: character slot the aim hook locked onto (0 = none; FUN_10010040)
    int m_0018;
    int m_001c;
    int m_0020;
    char settings[8];                                // 0x24..0x2b: byte-sized settings (settings[2] = 1 by default).
                                                     // An array: locals of this type get a /GS cookie
                                                     // (npc_frozen.cpp, char_move.cpp, switch_effects.cpp)
    int owner;                                       // 0x2c: owner (character index + 100), 0 = none
    Vec4 *m_0030;                                    // 0x30: where a hook writes a point (the aim hook: 60 units above its target)
    const Vec4 *m_0034;                              // 0x34: first end of the box a hook tests
    const Vec4 *m_0038;                              // 0x38: second end of the box
    int m_003c;
    int m_0040;
    EffectOptions() throw();                         // 0x101668b0 (model_state.cpp); callers know it cannot throw
};

// Where an effect moves to (EffectDef::m_0114, 16 bytes each); EffectManager::Step reads the first two. `type`:
// 1 follows the owner matrix, 2 follows the target matrix, 3 and 4 steer towards them (`value` / 100 of the turn
// per frame), 5 steers towards a direction the host gives, 6 sets the height.
#define EFFECT_TARGET_FOLLOW_OWNER 1
#define EFFECT_TARGET_FOLLOW_TARGET 2
#define EFFECT_TARGET_STEER_OWNER 3
#define EFFECT_TARGET_STEER_TARGET 4
#define EFFECT_TARGET_STEER_HOST 5
#define EFFECT_TARGET_SET_HEIGHT 6
struct EffectTarget
{
    int type;
    int value;
    int pad_0008[2];
};

// An event of an effect (EffectDef::m_0174, 16 bytes each). `type` 1: ends the effect when its hit test hit;
// 3 and 4: ends it inside the visible area above (3) or below (4) a height; 5, 6, 7 and 11: at `frame` ask the host
// for a screen effect with `value`; 8: ends it when its parent has ended; 10: ends it when the host refuses
// `frame` at its position; 9 is only tested by the hit handling.
#define EFFECT_TRIGGER_HIT_ENDS 1
#define EFFECT_TRIGGER_AREA_ABOVE_ENDS 3
#define EFFECT_TRIGGER_AREA_BELOW_ENDS 4
#define EFFECT_TRIGGER_SCREEN_FLASH 5
#define EFFECT_TRIGGER_SCREEN_TINT 6
#define EFFECT_TRIGGER_SCREEN_FADE 7
#define EFFECT_TRIGGER_PARENT_ENDS 8
#define EFFECT_TRIGGER_HIT_FLAG 9
#define EFFECT_TRIGGER_HOST_ENDS 10
#define EFFECT_TRIGGER_SCREEN_SHAKE 11
struct EffectTrigger
{
    int type;
    int frame;
    int value;
    int pad_000c;
};

// A child effect an effect starts (EffectDef::m_01b4, 16 bytes each): `count` effects `effect` at `frame`. `type`:
// 1 when it ends, 2 when its hit test hits, 3 every `frame` frames, 4 at `frame`.
#define EFFECT_CHILD_AT_END 1
#define EFFECT_CHILD_ON_HIT 2
#define EFFECT_CHILD_EVERY 3
#define EFFECT_CHILD_AT_FRAME 4
struct EffectChild
{
    int type;
    int frame;
    int effect;
    int count;
};

// An effect definition (0x3d8 bytes; EFFECT_DEFINITION_COUNT of them at DAT_1101dfdc), one per effect type the
// scripts can create. effect_defs.cpp makes new ones, effect_start.cpp starts them, char_effects.cpp and
// effect_sprites.cpp read their settings.
struct EffectDef
{
    int id;                                          // 0x00: script id, EFFECT_NONE = free
    int kind;                                        // 0x04: 1 and 0x12 are sprites; 6, 7, 0x11 and 0x16 have lights
    int m_0008;                                      // 0x08: non-zero = the effect is parked (Step does nothing)
    int m_000c;                                      // 0x0c
    int m_0010;                                      // 0x10
    int m_0014;                                      // 0x14
    int m_0018[16];                                  // 0x18: [0] non-zero = hit test with the host hook, [2], [4] its options
    int m_0058[4];                                   // 0x58: random turn of the direction: interval, then x/y/z amount
    int m_0068;                                      // 0x68: first of 0x10 bytes read from the definition file
    int depthTest;                                   // 0x6c: non-zero = drawn with render state 9 on
    char pad_0070[0x8];
    int startFrame;                                  // 0x78: frame the effect starts at
    int m_007c;                                      // 0x7c: first of 0x20 (old files) or 0x80 bytes of the file
    int lightFrames;                                 // 0x80: how long its light lasts (fades out over lightFrames + 10)
    char pad_0084[0x4];
    int fullAlphaB;                                  // 0x88: sprite table B: non-zero = starts at full alpha (else half)
    int spawnWorldSprite;                            // 0x8c: non-zero = also leaves a sprite in table A
    int worldSpriteLifetime;                         // 0x90: frames that sprite lives
    char pad_0094[4];
    int fullAlphaA;                                  // 0x98: the same for sprite table A
    char pad_009c[0x5c];
    int fixedList;                                   // 0xf8: non-zero = always started in the definition's list
    int texture;                                     // 0xfc: texture slot, -1 = not looked up yet (-1 after Init)
    int blend;                                       // 0x100: sprite blend mode 0-6
    int m_0104[4];                                   // 0x104: [0], [1] position track used (and added), [2], [3] the same for the turn track
    EffectTarget m_0114[4];                          // 0x114: where the effect moves (Step reads the first two)
    int m_0154;
    char pad_0158[0xc];
    int m_0164;                                      // 0x164: frames the effect lives (50 for a new definition)
    char pad_0168[0xc];
    EffectTrigger m_0174[4];                         // 0x174: events at a frame or when something happens
    EffectChild m_01b4[4];                           // 0x1b4: effects it starts
    int spawnKind;                                   // 0x1f4: 2 and 4 are effects whose Start returns 1
    char trackFlags[7];                              // 0x1f8: one byte after each of tracks 3 to 9 in the file
    char name[0x20];                                 // 0x1ff: display name
    char textureName[0x40];                          // 0x21f: texture looked up into `texture`
    unsigned char list;                              // 0x25f: effect list (0 = the caller's choice)
    char flat;                                       // 0x260: sprite table A: drawn at depth 0
    char m_0261;                                     // 0x261
    char pad_0262[2];
    Vec4 m_0264[8];                                  // 0x264: spawn offset, direction, turn ranges, accelerations (see Step)
    KeyTrack tracks[12];                             // 0x2e4
    float m_03d4;                                    // 0x3d4: random spread of the start position
};

// A running effect (0x1dc bytes; EFFECT_RECORD_COUNT of them at DAT_1115cfe8), linked into one of the
// EFFECT_LIST_COUNT lists by prev/next. EffectManager::Start (effect_start.cpp) fills it in; char_effects.cpp
// updates, draws and removes it.
struct EffectRecord
{
    int prev;                                        // 0x00: previous record of its list, -1 = first
    int next;                                        // 0x04: next record of its list, -1 = last
    int id;                                          // 0x08: effect definition, EFFECT_NONE = free
    int parent;                                      // 0x0c: record this effect is attached to, -1 = none
    int m_0010;                                      // 0x10: frames it has run (-1 before its first update)
    int frame;                                       // 0x14: start frame (the definition's plus the caller's offset)
    int m_0018;                                      // 0x18: latest frame at which a child effect starts
    int m_001c;                                      // 0x1c: frames it has been updated
    int serial;                                      // 0x20: value of the start counter when it started
    Vec4 partPositions[2];                           // 0x24: positions of its two parts, indexed by `part`
    Vec4 origin;                                     // 0x44: where it started (the parent's position for kinds 2 and 4)
    Vec4 direction;                                  // 0x54: the start direction
    Vec4 step;                                       // 0x64: moved by every frame (turned and steered)
    Vec4 acceleration;                               // 0x74: adds to localVelocity every frame
    Vec4 localVelocity;                              // 0x84: adds to the local offset every frame
    Vec4 velocity;                                   // 0x94: rotation angles, advanced every frame
    Vec4 spin;                                       // 0xa4: second rotation, advanced every frame
    Vec4 position;                                   // 0xb4
    Matrix4 worldMatrix;                             // 0xc4: the matrix the effect is drawn with
    Matrix4 matrix;                                  // 0x104: the caller's matrix, identity without one
    Matrix4 startMatrix;                             // 0x144: the matrix at the start (its parent's for kinds 2 and 4)
    const Matrix4 *matrixOwner;                      // 0x184: matrix it follows, 0 = none
    int m_0188;                                      // 0x188: the caller's value (the address of a Matrix4 to aim at)
    short m_018c;                                    // 0x18c: running child effects
    char part;                                       // 0x18e: 0 or 1, the part whose position decides visibility
    char spriteLeft[2];                              // 0x18f: 1 once the effect left a sprite in table A / table B
    char pad_0191;
    short light;                                     // 0x192: entry of DAT_118e99f8 it uses, 0 = none
    EffectOptions options;                           // 0x194: the start options (options.owner = starting character)
    int *pause;                                      // 0x1d8: paused while it points to a non-zero value
};

// A sprite record (0xc4 bytes; EFFECT_SPRITE_COUNT in each of the tables DAT_11134cc8 and DAT_118a8888), drawn by
// effect_sprites.cpp.
struct EffectSprite
{
    int type;                                        // 0x00: effect definition of the sprite, EFFECT_NONE = free
    Vec4 position;                                   // 0x04: world position (table A) or screen position (table B)
    float colour[3];                                 // 0x14
    float alpha;                                     // 0x20
    float colour2[3];                                // 0x24
    char pad_0030[0x44];
    float size;                                      // 0x74: half the width of the square
    float m_0078;
    float u0;                                        // 0x7c
    float u1;                                        // 0x80
    char pad_0084[0x18];
    float v0;                                        // 0x9c
    float v1;                                        // 0xa0
    char pad_00a4[0x18];
    short lifetime;                                  // 0xbc: frames the record lives
    short age;                                       // 0xbe: frames it has lived
    char list;                                       // 0xc0: effect list of the record that left it
    char pad_00c1[3];
};

// Number of points a light's trail can hold, and of vertices of the trail strip DAT_110187c0 (two per point).
#define EFFECT_LIGHT_POINT_COUNT 0x100
#define EFFECT_TRAIL_VERTEX_COUNT 0x200

// The light of an effect (0x4014 bytes; EFFECT_LIGHT_COUNT of them at DAT_118e99f8): a trail of up to
// EFFECT_LIGHT_POINT_COUNT points, each with a width and two colours, drawn as a strip by EffectManager::DrawLight
// (effect_light.cpp). The frame counter shortens the trail once the light stops being fed.
struct EffectLight
{
    union
    {
        int active;                                  // 0x00: non-zero while in use
        int pointCount;                              // 0x00: points of the trail so far (3 are needed to draw it)
    };
    int type;                                        // 0x04: effect definition (DAT_1101dfdc) the light belongs to
    int lastFrame;                                   // 0x08: effect frame of the point added last (-1 = none)
    int frame;                                       // 0x0c: frames since the light started (0 = not counting)
    float fade;                                      // 0x10: 1 at the start, falling to 0
    Vec4 points[EFFECT_LIGHT_POINT_COUNT];           // 0x14: world position of each point
    Vec4 widths[EFFECT_LIGHT_POINT_COUNT];           // 0x1014: x = width of the trail at the point, in 1/1000
    Vec4 colours[EFFECT_LIGHT_POINT_COUNT];          // 0x2014: r, g, b, a of each point (1.0 = full)
    Vec4 specular[EFFECT_LIGHT_POINT_COUNT];         // 0x3014: second colour r, g, b of each point
};

// Callbacks of the host that EffectManager::Step calls (slots 0, 1, 3 and 4 of DAT_1101de40, game_load_data.h).
// Slot 0 tests the effect's hit box (-1 = blocked), slot 1 asks for a screen effect (`command` 0 flash, 1 fade,
// 2 tint, 4 shake), slot 3 aims the effect at a point it writes through options->m_0030 (> 0 = it did) and
// slot 4 tells whether the position is still wanted (0 = not).
typedef int (*EffectHitHook)(float x, float y, float z, float size, EffectOptions *options);
typedef int (*EffectScreenHook)(float command, float a, float b, float c, float d);
typedef int (*EffectWantedHook)(float x, float y, float z, float value, float unused);
#define EFFECT_HOST_HIT_TEST 0
#define EFFECT_HOST_SCREEN 1
#define EFFECT_HOST_AIM 3
#define EFFECT_HOST_WANTED 4

extern "C" EffectDef DAT_1101dfdc[EFFECT_DEFINITION_COUNT];      // effect definitions
extern "C" EffectRecord DAT_1115cfe8[EFFECT_RECORD_COUNT];       // running effects
extern "C" EffectSprite DAT_11134cc8[EFFECT_SPRITE_COUNT];       // sprite table A: sprites placed in the world
extern "C" EffectSprite DAT_118a8888[EFFECT_SPRITE_COUNT];       // sprite table B: sprites placed on the screen
// effect lights; the two after the last are the ribbons EffectManager::DrawRecord builds for kinds 8/9 and 0x17
extern "C" EffectLight DAT_118e99f8[EFFECT_LIGHT_COUNT + 2];
extern "C" ScreenVertex DAT_110187c0[EFFECT_TRAIL_VERTEX_COUNT];  // vertices of the trail strip being drawn
extern "C" int DAT_1101d820[EFFECT_LIST_COUNT];                  // first record of each effect list
extern "C" int DAT_11018604[EFFECT_LIST_COUNT];                  // last record of each effect list

struct EffectManager
{
    Vec4 colour;                                     // 0x00: overall effect colour (white after Init)
    int m_0010[EFFECT_GROUP_COUNT];                  // 0x10: start offset of each of the six effect groups
    int pad_0028[2];
    int m_0030[EFFECT_GROUP_COUNT];                  // 0x30: current offset of each group (reset to the start)
    int m_0048;
    int m_004c;
    int m_0050;                                      // 0x50: number of running effects

    // 0x101886d0 (effect_definition_loader.cpp): loads the effect definitions from the file `path`. Returns 1, 0 when
    // the file is missing or not an effect definition file.
    int LoadDefinitions(const char *path);
    int Reset();                                     // 0x10184170: removes every running effect
    int Init();                                      // 0x101842a0: Reset plus the effect definitions
    void M_1016fab0();                               // 0x1016fab0 (called by Init)
    int ClearTables();                               // 0x1016fb10
    void ClearNodePool();                            // 0x1016faf0: marks every effect node unused
    int RemoveOwnedBy(int owner);                    // 0x101883b0
    int RemoveChildren(int parent);                  // 0x10172260
    int RemoveOfType(int type);                      // 0x10188490
    // 0x10184440 (effect_step.cpp): advances effect record `record` of list `list` by one frame: counts down its
    // start delay, moves it, runs its events, starts its child effects and feeds its light. Returns 1 while the
    // record is still waiting to start, 0 otherwise.
    int Step(int record, int list);
    int Update(int restricted, int includeHidden, int ignorePause);  // 0x10188580
    void LoadDefinition(int definition);             // 0x1016fa40
    // 0x10172350 (effect_start.cpp): starts the effect with script id `effect` at `position`. `owner` is the matrix
    // it follows (0 = none), `matrix` the matrix it is placed with, `options` the start options (0 = defaults) and
    // `pause` a flag that pauses it while non-zero. Returns the effect handle.
    int Start(int effect, Vec4 position, int parent, int unused, const Matrix4 *owner, int value, int frame,
              const Matrix4 *matrix, const EffectOptions *options, int *pause);
    // 0x10190e30: draws the effects of render pass `pass`; `camera` is passed through to the draw helpers.
    int Draw(const Matrix4 &view, const Matrix4 *camera, unsigned char pass);
    int DrawRecord(int record, const Matrix4 &view, const Matrix4 *camera, int pass);   // 0x10189530
    int DrawSpriteA(int index, const Matrix4 &view, const Matrix4 *camera);            // 0x101890f0
    int DrawSpriteB(int index, const Matrix4 &view, const Matrix4 *camera);            // 0x10189340
    // 0x10187520 (effect_light.cpp): draws effect light `index` as a textured trail; the light kinds 7, 9, 0x16 and
    // 0x17 have their own versions.
    int DrawLight(int index, const Matrix4 &view, const Matrix4 *camera);
    int DrawLightKind7(int index, const Matrix4 &view, const Matrix4 *camera);         // 0x101713f0 (kinds 7 and 9)
    int DrawLightKind22(int index, const Matrix4 &view, const Matrix4 *camera);        // 0x1016fb80 (kind 0x16)
    int DrawLightKind23(int index, const Matrix4 &view, const Matrix4 *camera);        // 0x10170580 (kind 0x17)
};

// The effect manager.
extern "C" EffectManager DAT_10daaa80;

typedef char check_effect_def[sizeof(EffectDef) == 0x3d8 ? 1 : -1];
typedef char check_effect_record[sizeof(EffectRecord) == 0x1dc ? 1 : -1];
typedef char check_effect_sprite[sizeof(EffectSprite) == 0xc4 ? 1 : -1];

#endif
