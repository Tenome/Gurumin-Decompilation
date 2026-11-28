// The model bank (DAT_10804bf8, 0x31624 bytes): up to 16 extra models (map objects, effects) with their own model
// records, the scene's light nodes, the shadow casters and their render targets, and the lens-flare points. Loaded
// per map; only active while `enabled` is set. Methods: model_bank.cpp.
// This is the one declaration of this type: include this header, never redeclare it.
#ifndef GURUMIN_MODEL_BANK_H
#define GURUMIN_MODEL_BANK_H

#include "vec4.h"
#include "matrix4.h"
#include "anim_model.h"

struct Node;

// Number of model records in the bank.
#define MODEL_BANK_SIZE 16
// AnimModel::m_2390 of a bank model that never times out.
#define MODEL_BANK_NO_TIMEOUT 99999
// Each lit model has LIGHT_VECTORS light vectors in `lighting`; the ones LightDirection sets:
#define LIGHT_VECTORS 22
#define LIGHT_KEY_DIRECTION 6          // the key light, from the target towards the viewer (length 2)
#define LIGHT_KEY_COLOUR 7             // its colour
#define LIGHT_AMBIENT 16               // the ambient light
#define LIGHT_SPARE_ENTRY 0x7f         // the entry used for the default lights (models that are not lit)
#define LIGHT_FADE_DISTANCE 1500.0f    // the key light gets 1 brighter per this distance ...
#define LIGHT_BASE_BRIGHTNESS 1.2f     // ... on top of this

struct ModelBank
{
    Node *lights[0x20];     // 0x0000: light nodes of the scene (Omn..., then Spot...)
    AnimModel *litModels[0x80];  // 0x0080: models that take part in the scene lighting
    int shadows[10];        // 0x0280: the shadow render targets "shadow0".."shadow9"
    char pad_02a8[0x1d8];
    int shadowTexture;      // 0x0480: kage.bmp
    Vec4 m_0484[0x80];      // 0x0484: per shadow caster (passed to the shadow drawing)
    Vec4 m_0c84[0x80];      // 0x0c84: per shadow caster
    float casterSize[0x80]; // 0x1484: per shadow caster
    int casters[0x80];      // 0x1684: shadow casters (indexes into litModels)
    int m_1884;             // 0x1884: number of shadow casters
    Vec4 lighting[0xb00];   // 0x1888: 22 vectors of light parameters for each entry of litModels
    Vec4 m_c888[0x16];      // 0xc888
    AnimModel models[MODEL_BANK_SIZE];  // 0xc9e8: the bank's model records (root = 0: free)
    const Vec4 *params[MODEL_BANK_SIZE];  // 0x313a8: per model: the position it is placed at
    int m_313e8;            // 0x313e8
    int enabled;            // 0x313ec: 1 when the current map uses the bank
    int lens[4];            // 0x313f0: lens01.bmp .. lens04.bmp (lens flare)
    char pad_31400[0x70];
    float flareAlpha[0x20]; // 0x31470: brightness of each flare point (0..1)
    int crossfade[3];       // 0x314f0: "crossfade0".."crossfade2" render targets (screen size)
    int mirror;             // 0x314fc: 256x256 render targets
    int saveBuffer;         // 0x31500
    int minimap;            // 0x31504
    int mirror2;            // 0x31508
    int saveBuffer2;        // 0x3150c
    int minimap2;           // 0x31510
    char pad_31514[0xc];
    int flareCount;         // 0x31520: flare points collected this frame
    Vec4 flareScreen[0x20]; // 0x31524: screen position of each flare point (x, y, depth, w)
    ~ModelBank();                                                        // 0x10037b20: destroys the 16 model records
    ModelBank();                                                         // 0x1017c8e0
    // 0x10164c10: a light direction for `model` seen from `position` towards `target` (0 when the bank is off); overlay_draw.cpp writes into the returned vector.
    Vec4 *LightDirection(AnimModel *model, Vec4 *position, Vec4 *target);
    // 0x1017c5f0: LightDirection for the bank model with node tree `root`, else the default lights.
    Vec4 *LightForRoot(Node *root, Vec4 *position, Vec4 *target);
    int Reset();
    int Init();  // 0x101649c0
    int CollectLights(Node *root);
    int Add(Node *root, const Vec4 *position);
    int AddLitModel(AnimModel *model);
    int AddFlarePoint(Vec4 position, const Matrix4 *viewProjection);
    int CreateTextures();
    int DrawLensFlares();
    int DrawShadows(Node *scene, const Matrix4 *parent, const Matrix4 *view, int a3);
    int DrawShadowsInto(Node *scene, const Matrix4 *parent, const Matrix4 *view, int a3);
    // 0x10165240: per frame, collects the lens flares and the lit models in view, gives each one its three nearest
    // lights and its shadow caster entries, and draws the shadow textures.
    int PrepareFrame(const Matrix4 *world, const Matrix4 *view, int a3, int renderShadowTextures);
};

// The model bank.
extern "C" ModelBank DAT_10804bf8;

// Per lit model entry: its squared distance to the camera target (the sort key of PrepareFrame).
extern "C" float DAT_10fa6bb8[0x80];

#endif
