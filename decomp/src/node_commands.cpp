// Node commands: the text a map or model file attaches to a node ("hide", "setlightcolor 1 1 1", "priority 3", ...).
// Scene callback 0x458 passes it to every node of that name.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vec4.h"

#include "matrix4.h"

#include "key_track.h"
#include "node_data.h"
#include "node_channel.h"
#include "texture_ref.h"
#include "node.h"
#include "engine_memory.h"

// Longest word of a command text.
#define COMMAND_WORD_SIZE 256

// Node::flags bits the commands set and clear.
#define FLAG_LOOKAT 0x1
#define FLAG_ZSORT 0x2
#define FLAG_GROUND 0x4
#define FLAG_LENS_EFFECT 0x8
#define FLAG_RECEIVE_SHADOW 0x10
#define FLAG_HIDE 0x20
#define FLAG_NO_CLIP 0x40
#define FLAG_CHARACTER 0x80
#define FLAG_COLLISION_ATARI 0x100
#define FLAG_COLLISION_BOX 0x200
#define FLAG_COLLISION_SPHERE 0x400
#define FLAG_COLLISION_CYLINDER 0x800
#define FLAG_IK_NODE 0x1000
#define FLAG_IK_GOAL 0x2000
#define FLAG_IK_COLLISION 0x4000
#define FLAG_LIGHT 0x10000
#define FLAG_MIRROR 0x20000
#define FLAG_KEY_MODE 0x100000
#define FLAG_IK_ORIGIN 0x200000
#define FLAG_NO_WRITE_Z 0x800000
#define FLAG_IK_PLANE 0x2000000
#define FLAG_STAIRS 0x4000000
#define FLAG_USE_ROTATION 0x8000000
#define FLAG_NO_KSP 0x10000000
#define FLAG_LIGHT_NO_BAKE 0x20000000
#define FLAG_LIGHT_NO_ASSIGN 0x40000000
#define FLAG_LIGHT_TYPE_MASK 0x1c000000
#define FLAG_LIGHT_TYPE_1 0x0
#define FLAG_LIGHT_TYPE_0 0x4000000
#define FLAG_LIGHT_TYPE_3 0x8000000
#define FLAG_LIGHT_TYPE_2 0xc000000

// Bits of Node::m_1878.
#define STYLE_LIGHT_TYPE_MASK 0x0f
#define STYLE_LIGHT_SHADOW 0x10
#define STYLE_DROP_SHADOW 0x20
#define STYLE_RECEIVE_SHADOW 0x40
#define STYLE_BAKEABLE 0x80

// Values of Node::pathKind.
#define PATH_OBJECT 0
#define PATH_CAMERA 1
#define PATH_TARGET_CAMERA 2

// The two colour plugs (slots 6 and 7) and their plug type.
#define PLUG_COLOUR_TYPE 0x4ee33
#define COLOUR_PLUG_FIRST 6
#define COLOUR_PLUG_COUNT 2

#define PI_APPROX 3.1415f

#define MAX_OF(a, b) ((a) < (b) ? (b) : (a))

// 0x101a3000 (Node::ApplyCommandText). Reads `commands` word by word and applies each command to this node;
// a word that is not a command is skipped. Commands with arguments read them as the next words. Returns 1.
//   convkeytoani / convanitokey / anidelete: turn the key tracks into frame matrices, the frame matrices into key
//     tracks, or drop the frame matrices.
//   reset, [!]keymode, [!]nowritez, [!]bakeable, ..., [!]hide, [!]mirr, ...: set or clear flag bits (a leading ! clears).
//   object, camera, tcamera: the node's path kind.
//   cgmat <frame> <12 floats>: sets the rotation and translation rows of the matrix of a frame.
//   iklimx/y/z <min> <max>: rotation limits in degrees.  cliplevel <margin>.
//   setlightcolor/intens/range/cone/type, priority, setvs, event, shader, rename, movecamera0/1, setanitb, setaniset.
int Node::ApplyCommandText(const char *commands) throw()
{
    char word[COMMAND_WORD_SIZE];
    int position = 0;
    while (NextToken(word, commands, &position))
    {
        if (_strnicmp(word, "convkeytoani", 12) == 0 && extra != 0)
        {
            if (m_028c != 0)
                M_10191210();
            else if (frameMatrixCount != 0)
                fread(0, 1, 1, 0);
            int frames = MAX_OF(MAX_OF(extra->tracks[NODE_TRACK_TRANSLATION].LastFrame(),
                                       extra->tracks[NODE_TRACK_SCALE].LastFrame()),
                                extra->tracks[NODE_TRACK_ROTATION].LastFrame()) + 1;
            if (frames <= 1)
                matrix = extra->AnimatedMatrix(0);
            else
            {
                frameMatrices = (Matrix4 *)DAT_11b0b938(frames * sizeof(Matrix4) + 10, "node->m_animat");
                frameMatrixCount = frames;
                m_028c = (TextureRef *)DAT_11b0b938(4, "motion");
                m_028c->slot = -1;
                for (int i = 0; i < frames; i++)
                    frameMatrices[i] = extra->AnimatedMatrix(i);
            }
        }
        if (_strnicmp(word, "convanitokey", 12) == 0)
        {
            if (m_028c != 0)
            {
                if (extra == 0)
                    M_101949d0();
                Vec4 scale, rotation, translation;
                extra->tracks[NODE_TRACK_SCALE].Free();
                extra->tracks[NODE_TRACK_ROTATION].Free();
                extra->tracks[NODE_TRACK_TRANSLATION].Free();
                for (int i = 0; i < frameMatrixCount; i++)
                {
                    DecomposeMatrix(&frameMatrices[i], &scale, &rotation, &translation);
                    extra->tracks[NODE_TRACK_SCALE].Set(i, scale.x, scale.y, scale.z, scale.w);
                    extra->tracks[NODE_TRACK_ROTATION].Set(i, rotation.x, rotation.y, rotation.z, rotation.w);
                    extra->tracks[NODE_TRACK_TRANSLATION].Set(i, translation.x, translation.y, translation.z, translation.w);
                }
            }
            else
            {
                if (extra == 0)
                    M_101949d0();
                Vec4 scale, rotation, translation;
                extra->tracks[NODE_TRACK_SCALE].Free();
                extra->tracks[NODE_TRACK_ROTATION].Free();
                extra->tracks[NODE_TRACK_TRANSLATION].Free();
                DecomposeMatrix(&matrix, &scale, &rotation, &translation);
                extra->tracks[NODE_TRACK_SCALE].Set(0, scale.x, scale.y, scale.z, scale.w);
                extra->tracks[NODE_TRACK_ROTATION].Set(0, rotation.x, rotation.y, rotation.z, rotation.w);
                extra->tracks[NODE_TRACK_TRANSLATION].Set(0, translation.x, translation.y, translation.z, translation.w);
            }
        }
        if (_strnicmp(word, "anidelete", 9) == 0)
        {
            if (m_028c != 0)
                M_10191210();
            else if (frameMatrixCount != 0)
                fread(0, 1, 1, 0);
        }
        else if (_strnicmp(word, "reset", 5) == 0)
            flags = 0;
        else if (_strnicmp(word, "keymode", 7) == 0)
        {
            flags |= FLAG_KEY_MODE;
            if (extra == 0)
                M_101949d0();
        }
        else if (_strnicmp(word, "!keymode", 8) == 0)
            flags &= ~FLAG_KEY_MODE;
        else if (_strnicmp(word, "nowritez", 8) == 0)
            flags |= FLAG_NO_WRITE_Z;
        else if (_strnicmp(word, "!nowritez", 9) == 0)
            flags &= ~FLAG_NO_WRITE_Z;
        else if (_strnicmp(word, "bakeable", 8) == 0)
            m_1878 |= STYLE_BAKEABLE;
        else if (_strnicmp(word, "recshadow", 9) == 0)
            m_1878 |= STYLE_RECEIVE_SHADOW;
        else if (_strnicmp(word, "dropshadow", 10) == 0)
            m_1878 |= STYLE_DROP_SHADOW;
        else if (_strnicmp(word, "!bakeable", 9) == 0)
            m_1878 &= ~STYLE_BAKEABLE;
        else if (_strnicmp(word, "!recshadow", 10) == 0)
            m_1878 &= ~STYLE_RECEIVE_SHADOW;
        else if (_strnicmp(word, "!dropshadow", 11) == 0)
            m_1878 &= ~STYLE_DROP_SHADOW;
        else if (_strnicmp(word, "object", 6) == 0)
        {
            pathKind = PATH_OBJECT;
            m_1878 &= ~STYLE_LIGHT_TYPE_MASK;
        }
        else if (_strnicmp(word, "tcamera", 7) == 0)
            pathKind = PATH_TARGET_CAMERA;
        else if (_strnicmp(word, "camera", 6) == 0)
            pathKind = PATH_CAMERA;
        else if (_strnicmp(word, "light", 5) == 0)
            flags |= FLAG_LIGHT;
        else if (_strnicmp(word, "!light", 6) == 0)
            flags &= ~FLAG_LIGHT;
        else if (_strnicmp(word, "ground", 6) == 0)
            flags |= FLAG_GROUND;
        else if (_strnicmp(word, "!ground", 7) == 0)
            flags &= ~FLAG_GROUND;
        else if (_strnicmp(word, "kaidan", 6) == 0)
            flags |= FLAG_STAIRS;
        else if (_strnicmp(word, "!kaidan", 7) == 0)
            flags &= ~FLAG_STAIRS;
        else if (_strnicmp(word, "userot", 6) == 0)
            flags |= FLAG_USE_ROTATION;
        else if (_strnicmp(word, "!userot", 7) == 0)
            flags &= ~FLAG_USE_ROTATION;
        else if (_strnicmp(word, "noksp", 5) == 0)
            flags |= FLAG_NO_KSP;
        else if (_strnicmp(word, "!noksp", 6) == 0)
            flags &= ~FLAG_NO_KSP;
        else if (_strnicmp(word, "noclip", 6) == 0)
            flags |= FLAG_NO_CLIP;
        else if (_strnicmp(word, "!noclip", 7) == 0)
            flags &= ~FLAG_NO_CLIP;
        else if (_strnicmp(word, "lookat", 6) == 0)
            flags |= FLAG_LOOKAT;
        else if (_strnicmp(word, "!lookat", 7) == 0)
            flags &= ~FLAG_LOOKAT;
        else if (_strnicmp(word, "zsort", 5) == 0)
            flags |= FLAG_ZSORT;
        else if (_strnicmp(word, "!zsort", 6) == 0)
            flags &= ~FLAG_ZSORT;
        else if (_strnicmp(word, "lenseffect", 10) == 0)
            flags |= FLAG_LENS_EFFECT;
        else if (_strnicmp(word, "!lenseffect", 11) == 0)
            flags &= ~FLAG_LENS_EFFECT;
        else if (_strnicmp(word, "rshadow", 7) == 0)
            flags |= FLAG_RECEIVE_SHADOW;
        else if (_strnicmp(word, "!rshadow", 8) == 0)
            flags &= ~FLAG_RECEIVE_SHADOW;
        else if (_strnicmp(word, "hide", 4) == 0)
            flags |= FLAG_HIDE;
        else if (_strnicmp(word, "!hide", 5) == 0)
            flags &= ~FLAG_HIDE;
        else if (_strnicmp(word, "mirr", 4) == 0)
            flags |= FLAG_MIRROR;
        else if (_strnicmp(word, "!mirr", 5) == 0)
            flags &= ~FLAG_MIRROR;
        else if (_strnicmp(word, "char", 4) == 0)
            flags |= FLAG_CHARACTER;
        else if (_strnicmp(word, "!char", 5) == 0)
            flags &= ~FLAG_CHARACTER;
        else if (_strnicmp(word, "atari", 5) == 0)
            flags |= FLAG_COLLISION_ATARI;
        else if (_strnicmp(word, "!atari", 6) == 0)
            flags &= ~FLAG_COLLISION_ATARI;
        else if (_strnicmp(word, "cbox", 4) == 0)
            flags |= FLAG_COLLISION_BOX;
        else if (_strnicmp(word, "!cbox", 5) == 0)
            flags &= ~FLAG_COLLISION_BOX;
        else if (_strnicmp(word, "csp", 3) == 0)
            flags |= FLAG_COLLISION_SPHERE;
        else if (_strnicmp(word, "!csp", 4) == 0)
            flags &= ~FLAG_COLLISION_SPHERE;
        else if (_strnicmp(word, "ccy", 3) == 0)
            flags |= FLAG_COLLISION_CYLINDER;
        else if (_strnicmp(word, "!ccy", 4) == 0)
            flags &= ~FLAG_COLLISION_CYLINDER;
        else if (_strnicmp(word, "ikg", 3) == 0)
            flags |= FLAG_IK_GOAL;
        else if (_strnicmp(word, "ikp", 3) == 0)
            flags |= FLAG_IK_PLANE;
        else if (_strnicmp(word, "ikn", 3) == 0)
            flags |= FLAG_IK_NODE;
        else if (_strnicmp(word, "iko", 5) == 0)
            flags |= FLAG_IK_ORIGIN;
        else if (_strnicmp(word, "ikcoll", 6) == 0)
            flags |= FLAG_IK_COLLISION;
        else if (_strnicmp(word, "cgmat", 5) == 0)
        {
            NextToken(word, commands, &position);
            int index = atol(word);
            Matrix4 target;
            target = matrix;
            NextToken(word, commands, &position);
            target._11 = (float)atof(word);
            NextToken(word, commands, &position);
            target._12 = (float)atof(word);
            NextToken(word, commands, &position);
            target._13 = (float)atof(word);
            NextToken(word, commands, &position);
            target._21 = (float)atof(word);
            NextToken(word, commands, &position);
            target._22 = (float)atof(word);
            NextToken(word, commands, &position);
            target._23 = (float)atof(word);
            NextToken(word, commands, &position);
            target._31 = (float)atof(word);
            NextToken(word, commands, &position);
            target._32 = (float)atof(word);
            NextToken(word, commands, &position);
            target._33 = (float)atof(word);
            NextToken(word, commands, &position);
            target._41 = (float)atof(word);
            NextToken(word, commands, &position);
            target._42 = (float)atof(word);
            NextToken(word, commands, &position);
            target._43 = (float)atof(word);
            if (m_028c != 0)
            {
                if (frameMatrixCount < index && frameMatrixCount >= 0)
                {
                    target._14 = frameMatrices[index]._14;
                    target._24 = frameMatrices[index]._24;
                    target._34 = frameMatrices[index]._34;
                    target._44 = frameMatrices[index]._44;
                    frameMatrices[index] = target;
                }
            }
            else
                matrix = target;
        }
        else if (_strnicmp(word, "iklimx", 6) == 0)
        {
            AllocSaved();
            NextToken(word, commands, &position);
            saved->limitMin.x = (float)atof(word) * PI_APPROX / 180.0f;
            NextToken(word, commands, &position);
            saved->limitMax.x = (float)atof(word) * PI_APPROX / 180.0f;
        }
        else if (_strnicmp(word, "iklimy", 6) == 0)
        {
            AllocSaved();
            NextToken(word, commands, &position);
            saved->limitMin.y = (float)atof(word) * PI_APPROX / 180.0f;
            NextToken(word, commands, &position);
            saved->limitMax.y = (float)atof(word) * PI_APPROX / 180.0f;
        }
        else if (_strnicmp(word, "iklimz", 6) == 0)
        {
            AllocSaved();
            NextToken(word, commands, &position);
            saved->limitMin.z = (float)atof(word) * PI_APPROX / 180.0f;
            NextToken(word, commands, &position);
            saved->limitMax.z = (float)atof(word) * PI_APPROX / 180.0f;
        }
        else if (_strnicmp(word, "cliplevel", 9) == 0)
        {
            NextToken(word, commands, &position);
            cullMargin = (float)atof(word);
            flags &= ~FLAG_NO_CLIP;
        }
        else if (_strnicmp(word, "setlightbake", 12) == 0)
            flags &= ~FLAG_LIGHT_NO_BAKE;
        else if (_strnicmp(word, "!setlightbake", 13) == 0)
            flags |= FLAG_LIGHT_NO_BAKE;
        else if (_strnicmp(word, "setlightas", 10) == 0)
            flags &= ~FLAG_LIGHT_NO_ASSIGN;
        else if (_strnicmp(word, "!setlightas", 11) == 0)
            flags |= FLAG_LIGHT_NO_ASSIGN;
        else if (_strnicmp(word, "setlightshadow", 14) == 0)
            m_1878 |= STYLE_LIGHT_SHADOW;
        else if (_strnicmp(word, "!setlightshadow", 15) == 0)
            m_1878 &= ~STYLE_LIGHT_SHADOW;
        else if (_strnicmp(word, "setobjkaidan", 12) == 0)
            flags |= FLAG_STAIRS;
        else if (_strnicmp(word, "setlightdxo", 11) == 0)
        {
            NextToken(word, commands, &position);
            long lightType = atol(word);
            NextToken(word, commands, &position);
            long noKsp = atol(word);
            flags &= ~FLAG_LIGHT_TYPE_MASK;
            if (lightType == 1)
                flags |= FLAG_LIGHT_TYPE_1;
            else if (lightType == 2)
                flags |= FLAG_LIGHT_TYPE_2;
            else if (lightType == 0)
                flags |= FLAG_LIGHT_TYPE_0;
            else if (lightType == 3)
                flags |= FLAG_LIGHT_TYPE_3;
            if (noKsp != 0)
                flags |= FLAG_NO_KSP;
        }
        else if (_strnicmp(word, "setlightcolor", 13) == 0)
        {
            NextToken(word, commands, &position);
            fogColour.x = (float)atof(word);
            NextToken(word, commands, &position);
            fogColour.y = (float)atof(word);
            NextToken(word, commands, &position);
            fogColour.z = (float)atof(word);
            fogColour.w = 1.0f;
        }
        else if (_strnicmp(word, "setlightintens", 14) == 0)
        {
            NextToken(word, commands, &position);
            fogDensity = (float)atof(word);
        }
        else if (_strnicmp(word, "setlightrange", 13) == 0)
        {
            NextToken(word, commands, &position);
            fogNear = (float)atof(word);
            NextToken(word, commands, &position);
            fogFar = (float)atof(word);
        }
        else if (_strnicmp(word, "setlightcone", 12) == 0)
        {
            NextToken(word, commands, &position);
            lightConeInner = (float)atof(word);
            NextToken(word, commands, &position);
            m_1888 = (float)atof(word);
        }
        else if (_strnicmp(word, "priority", 8) == 0)
        {
            NextToken(word, commands, &position);
            long priority = atol(word);
            m_187e = priority > 0 && atol(word) > 0xff ? 0xff : atol(word) > 0 ? atol(word) : 0;
        }
        else if (_strnicmp(word, "setlighttype", 12) == 0)
        {
            m_1878 &= ~STYLE_LIGHT_TYPE_MASK;
            NextToken(word, commands, &position);
            m_1878 |= (char)atol(word) + 1;
        }
        else if (_strnicmp(word, "setvs", 5) == 0)
        {
            NextToken(word, commands, &position);
            long shader = atol(word);
            if (shader == 0)
                drawMode = 8;
            else if (shader == 1)
                drawMode = 0x1a;
            else if (shader == 2)
                drawMode = 2;
            else if (shader == 3)
                drawMode = 10;
            else if (shader == 4)
                drawMode = 0x1e;
            else if (shader == 5)
                drawMode = 0x1d;
        }
        else if (_strnicmp(word, "event", 5) == 0)
        {
            NextToken(word, commands, &position);
            group = atol(word);
        }
        else if (_strnicmp(word, "shader", 6) == 0)
        {
            NextToken(word, commands, &position);
            drawMode = atol(word);
        }
        else if (_strnicmp(word, "rename", 6) == 0)
        {
            NextToken(word, commands, &position);
            Rename(word, 0);
        }
        else if (_strnicmp(word, "movecamera0", 11) == 0)
        {
            NextToken(word, commands, &position);
            m_187c = (char)atol(word);
        }
        else if (_strnicmp(word, "movecamera1", 11) == 0)
        {
            NextToken(word, commands, &position);
            m_187d = (char)atol(word);
        }
        else if (_strnicmp(word, "setanitb", 8) == 0)
        {
            NextToken(word, commands, &position);
            char motion[64];
            int c = 0;
            do
                motion[c] = word[c];
            while (word[c++] != 0);
            NextToken(word, commands, &position);
            long first = atol(word);
            NextToken(word, commands, &position);
            long last = atol(word);
            NextToken(word, commands, &position);
            unsigned long loop = atol(word);
            NextToken(word, commands, &position);
            long mode = atol(word);
            for (int i = 0; i < COLOUR_PLUG_COUNT; i++)
            {
                if (plugType[COLOUR_PLUG_FIRST + i] != 0 && plugType[COLOUR_PLUG_FIRST + i] == PLUG_COLOUR_TYPE)
                    plugs[COLOUR_PLUG_FIRST + i]->keys.AddEntry(first, last, motion, mode * 2 | loop);
            }
        }
        else if (_strnicmp(word, "setaniset", 9) == 0)
        {
            NextToken(word, commands, &position);
            long motion = atol(word);
            NextToken(word, commands, &position);
            long frames = atol(word);
            for (int i = 0; i < COLOUR_PLUG_COUNT; i++)
            {
                if (plugType[COLOUR_PLUG_FIRST + i] != 0 && plugType[COLOUR_PLUG_FIRST + i] == PLUG_COLOUR_TYPE)
                    plugs[COLOUR_PLUG_FIRST + i]->keys.Insert(motion, frames, 0);
            }
        }
    }
    return 1;
}
