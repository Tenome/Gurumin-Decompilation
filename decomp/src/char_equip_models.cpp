// Putting on a character's equipment models (0x100404e0). A character's worn pieces are four two-digit fields of
// its script code, AABBCCDD = weapon, mount, clothes, head piece. This builds the models of the pieces asked for and
// hangs them on the character's bones, also warming the model cache with the matching "d"/"br" drop pieces.

#include <string.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "anim_model.h"

#include "char_state.h"

#include "char_slot.h"

#include "char_tables.h"

#include "char_vars.h"
#include "map_vars.h"
#include "game_vars.h"

#include "texture_load.h"
#include "char_equip_models.h"

#define BASE_KIND_PACKED_GROUP 0x10
#define BASE_KIND_PAIRED_WEAPONS 0x31
#define EQUIP_FIELD_UNIT 100
#define MOUNT_FIELD_UNIT 10000
#define WEAPON_FIELD_UNIT 1000000
#define PIECE_NODE_CHILDREN 0x40
#define EQUIP_MARK_1 1
#define EQUIP_MARK_2 2
#define EQUIP_MARK_4 4
#define BODY_SCALE_PAIRED_WEAPONS 1.2f
#define BODY_SCALE_PACKED_GROUP 2.0f
#define BODY_SCALE_SMALL 0.8f
#define MOUNT_COLOUR_MASK 0x400
#define MOUNT_COLOUR_GREEN 0xfe
#define MOUNT_COLOUR_BLUE 0x2f
#define CLOTHES_SPLIT_A 7
#define CLOTHES_SPLIT_B 9
#define ROUNDING_HALF 0.5f
#define MODEL_DEBUG_FLAGS 0x1f

// Loads the model file `name` into the model cache through the scratch node and lets it go again.
static __forceinline void PreloadModel(Node &scratch, const char *name)
{
    scratch.Load(FUN_1013f8a0(name, 0), 0, 0, 0);
    scratch.Release();
}

// 0x100404e0 (Char_PutOnEquipment; C++ linkage, the original has an EH frame). Character `index` wears the pieces
// `mount`, `clothes`, `head` and `weapon` (the model numbers of the four fields; 0 = leave that part alone, an
// unknown number = nothing). For each part the old node ("@norisou", "@hukusou", "@atamasou", "@migitesou" and
// "@hidaritesou") is cut off, the model files of the piece are loaded (EEQ_nn.it3 for the body, the "d"/"br"
// variants only to warm the cache) and the new model node is hung under the right bone ("@Bip01 Spine", the root
// for a mount, "@Bip01 R Hand" and "@Bip01 L Hand"), scaled up for the big kinds 0x31 and 0x10 and down for the
// "EN_023x0" model. Clothes 7 and 9 are made of two nested nodes. Some pieces add to the character's two stat values
// (STAT_EFFECT_PARAM and STAT_VALUE_4), to its marks (CharSlot::m_0143) or set its twoHanded state. The first call
// for a plain character saves its marks, kind and the two stat values in DAT_104679a0, DAT_1030b668, DAT_10314ec0
// and DAT_1021f440. The packed script code DAT_102548c0 is rebuilt from the fields that were put on, the stat
// values and marks are stored back and, unless `skipKindSelect` is set, Char_SelectKind runs for the character
// (`alternative` is the second weapon set: only the right hand model is made). Returns 0.
int FUN_100404e0(int index, int mount, int clothes, int head, int weapon, int skipKindSelect, int alternative)
{
    Node scratch;
    char file[0x100];
    char leftFile[0x100];
    if (DAT_10db6ee8[index].root == 0)
        return 0;
    if ((mount != 0 || clothes != 0 || head != 0 || weapon != 0) && DAT_102548c0[index] == 0 && DAT_1030b668[index] == 0)
    {
        // the character's own kind, read once: it is stored and tested for "no kind" (stored as -1)
        int savedKind = DAT_10daaae8[index].kind;
        DAT_104679a0[index] = DAT_10daaae8[index].m_0143;
        DAT_1030b668[index] = savedKind;
        DAT_10314ec0[index] = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
        DAT_1021f440[index] = DAT_10daaae8[index].stats[STAT_VALUE_4];
        if (savedKind == 0)
            DAT_1030b668[index] = -1;
    }
    int marks = DAT_104679a0[index];
    float effectParam = (float)DAT_10314ec0[index];
    DAT_102548c0[index] = 0;
    float value4 = (float)DAT_1021f440[index];
    if (head != 0)
    {
        Node *old = DAT_10db6ee8[index].root->Find("@atamasou", 0);
        if (old != 0)
            old->parent->RemoveChild(old, 0);
        switch (head)
        {
        case 1:
            strcpy(file, "EEQ_07.it3");
            PreloadModel(scratch, "EEQ_07.it3");
            break;
        case 2:
            marks |= EQUIP_MARK_4;
            strcpy(file, "EEQ_08.it3");
            PreloadModel(scratch, "EEQ_08d.it3");
            break;
        case 3:
            strcpy(file, "EEQ_09.it3");
            PreloadModel(scratch, "EEQ_09d.it3");
            break;
        case 4:
            value4 += 1.0f;
            strcpy(file, "EEQ_10.it3");
            PreloadModel(scratch, "EEQ_10d.it3");
            break;
        case 5:
            strcpy(file, "EEQ_11.it3");
            PreloadModel(scratch, "EEQ_11d.it3");
            break;
        case 6:
            marks |= EQUIP_MARK_1;
            strcpy(file, "EEQ_16.it3");
            PreloadModel(scratch, "EEQ_16d.it3");
            break;
        case 7:
            marks |= EQUIP_MARK_4;
            strcpy(file, "EEQ_17.it3");
            PreloadModel(scratch, "EEQ_17d.it3");
            break;
        case 8:
            marks |= EQUIP_MARK_1;
            strcpy(file, "EEQ_18.it3");
            PreloadModel(scratch, "EEQ_18d.it3");
            break;
        case 9:
            marks |= EQUIP_MARK_2;
            strcpy(file, "EEQ_14.it3");
            PreloadModel(scratch, "EEQ_14d.it3");
            break;
        case 10:
            effectParam += 1.0f;
            strcpy(file, "EEQ_07b.it3");
            PreloadModel(scratch, "EEQ_07b.it3");
            break;
        case 11:
            marks |= EQUIP_MARK_4;
            strcpy(file, "EEQ_08b.it3");
            PreloadModel(scratch, "EEQ_08bd.it3");
            break;
        case 12:
            strcpy(file, "EEQ_09b.it3");
            PreloadModel(scratch, "EEQ_09bd.it3");
            break;
        case 13:
            value4 += 2.0f;
            strcpy(file, "EEQ_10b.it3");
            PreloadModel(scratch, "EEQ_10bd.it3");
            break;
        case 14:
            strcpy(file, "EEQ_11b.it3");
            PreloadModel(scratch, "EEQ_11bd.it3");
            break;
        case 15:
            marks |= EQUIP_MARK_2;
            strcpy(file, "EEQ_14b.it3");
            PreloadModel(scratch, "EEQ_14bd.it3");
            break;
        case 16:
            marks |= EQUIP_MARK_1;
            strcpy(file, "EEQ_16b.it3");
            PreloadModel(scratch, "EEQ_16bd.it3");
            break;
        case 17:
            marks |= EQUIP_MARK_1;
            strcpy(file, "EEQ_18b.it3");
            PreloadModel(scratch, "EEQ_18bd.it3");
            break;
        default:
            goto headDone;
        }
        Node *spine = DAT_10db6ee8[index].root->Find("@Bip01 Spine", 0);
        if (spine != 0)
        {
            Node *piece = spine->AddChild(0, PIECE_NODE_CHILDREN);
            piece->Load(FUN_1013f8a0(file, 0), 0, 0, 0);
            strcpy(piece->name, "atamasou");
            piece->parent = spine;
            if (DAT_10daaae8[index].kind == BASE_KIND_PAIRED_WEAPONS || DAT_1030b668[index] == BASE_KIND_PAIRED_WEAPONS)
                piece->matrix = Scaling(BODY_SCALE_PAIRED_WEAPONS, BODY_SCALE_PAIRED_WEAPONS, BODY_SCALE_PAIRED_WEAPONS);
            else if (DAT_10daaae8[index].kind == BASE_KIND_PACKED_GROUP || DAT_1030b668[index] == BASE_KIND_PACKED_GROUP)
                piece->matrix = Scaling(BODY_SCALE_PACKED_GROUP, BODY_SCALE_PACKED_GROUP, BODY_SCALE_PACKED_GROUP);
            else if (DAT_10db6ee8[index].root != 0 && _strnicmp(DAT_10db6ee8[index].root->folder, "EN_023x0", 8) == 0)
                piece->matrix = Scaling(BODY_SCALE_SMALL, BODY_SCALE_SMALL, BODY_SCALE_SMALL);
            piece->BuildMesh(1, 0);
        }
        DAT_102548c0[index] = head;
    }
headDone:
    if (clothes != 0)
    {
        Node *old = DAT_10db6ee8[index].root->Find("@hukusou", 0);
        if (old != 0)
            old->parent->RemoveChild(old, 0);
        switch (clothes)
        {
        case 1:
            strcpy(file, "EEQ_01.it3");
            PreloadModel(scratch, "EEQ_01br.it3");
            PreloadModel(scratch, "EEQ_01d.it3");
            if (DAT_10daaae8[index].twoHanded == 0)
                DAT_10daaae8[index].twoHanded = 1;
            break;
        case 2:
            strcpy(file, "EEQ_02.it3");
            PreloadModel(scratch, "EEQ_02br.it3");
            PreloadModel(scratch, "EEQ_01d.it3");
            if (DAT_10daaae8[index].twoHanded == 0)
                DAT_10daaae8[index].twoHanded = 1;
            break;
        case 3:
            strcpy(file, "EEQ_03.it3");
            PreloadModel(scratch, "EEQ_03br.it3");
            PreloadModel(scratch, "EEQ_01d.it3");
            if (DAT_10daaae8[index].twoHanded == 0)
                DAT_10daaae8[index].twoHanded = 1;
            break;
        case 4:
            strcpy(file, "EEQ_04.it3");
            PreloadModel(scratch, "EEQ_04br.it3");
            PreloadModel(scratch, "EEQ_04d.it3");
            DAT_10daaae8[index].twoHanded = 2;
            break;
        case 5:
            strcpy(file, "EEQ_05.it3");
            PreloadModel(scratch, "EEQ_05br.it3");
            PreloadModel(scratch, "EEQ_05d.it3");
            DAT_10daaae8[index].twoHanded = 2;
            break;
        case 6:
            strcpy(file, "EEQ_06.it3");
            PreloadModel(scratch, "EEQ_06br.it3");
            PreloadModel(scratch, "EEQ_06d.it3");
            effectParam += 1.0f;
            value4 += 1.0f;
            DAT_10daaae8[index].twoHanded = 2;
            break;
        case 7:
            strcpy(file, "EEQ_12.it3");
            PreloadModel(scratch, "EEQ_12br.it3");
            PreloadModel(scratch, "EEQ_12d.it3");
            value4 += 1.0f;
            break;
        case 8:
            strcpy(file, "EEQ_13.it3");
            PreloadModel(scratch, "EEQ_13br.it3");
            PreloadModel(scratch, "EEQ_13d.it3");
            effectParam += 1.0f;
            break;
        case 9:
            strcpy(file, "EEQ_12b.it3");
            PreloadModel(scratch, "EEQ_12bb.it3");
            PreloadModel(scratch, "EEQ_12d.it3");
            value4 += 2.0f;
            break;
        case 10:
            strcpy(file, "EEQ_13b.it3");
            PreloadModel(scratch, "EEQ_13bb.it3");
            PreloadModel(scratch, "EEQ_13bd.it3");
            effectParam += 2.0f;
            break;
        case 11:
            strcpy(file, "EEQ_01b.it3");
            PreloadModel(scratch, "EEQ_01bb.it3");
            PreloadModel(scratch, "EEQ_01d.it3");
            if (DAT_10daaae8[index].twoHanded == 0)
                DAT_10daaae8[index].twoHanded = 1;
            break;
        case 12:
            strcpy(file, "EEQ_02b.it3");
            PreloadModel(scratch, "EEQ_02bb.it3");
            PreloadModel(scratch, "EEQ_01d.it3");
            if (DAT_10daaae8[index].twoHanded == 0)
                DAT_10daaae8[index].twoHanded = 1;
            break;
        case 13:
            strcpy(file, "EEQ_03b.it3");
            PreloadModel(scratch, "EEQ_03bb.it3");
            PreloadModel(scratch, "EEQ_01d.it3");
            if (DAT_10daaae8[index].twoHanded == 0)
                DAT_10daaae8[index].twoHanded = 1;
            break;
        case 14:
            strcpy(file, "EEQ_04b.it3");
            PreloadModel(scratch, "EEQ_04bb.it3");
            PreloadModel(scratch, "EEQ_04bd.it3");
            DAT_10daaae8[index].twoHanded = 2;
            break;
        case 15:
            strcpy(file, "EEQ_05b.it3");
            PreloadModel(scratch, "EEQ_05bb.it3");
            PreloadModel(scratch, "EEQ_05bd.it3");
            DAT_10daaae8[index].twoHanded = 2;
            break;
        case 16:
            strcpy(file, "EEQ_06b.it3");
            PreloadModel(scratch, "EEQ_06bb.it3");
            PreloadModel(scratch, "EEQ_06bd.it3");
            effectParam += 1.0f;
            value4 += 1.0f;
            DAT_10daaae8[index].twoHanded = 2;
            break;
        default:
            goto clothesDone;
        }
        Node *spine = DAT_10db6ee8[index].root->Find("@Bip01 Spine", 0);
        if (spine != 0)
        {
            Node *piece = spine->AddChild(0, PIECE_NODE_CHILDREN);
            if (clothes == CLOTHES_SPLIT_A || clothes == CLOTHES_SPLIT_B)
            {
                Node *inner = piece->AddChild(0, PIECE_NODE_CHILDREN);
                inner->Load(FUN_1013f8a0(file, 0), 0, 0, 0);
                strcpy(inner->name, "hukusou2");
                inner->parent = piece;
                strcpy(piece->name, "hukusou");
                piece->parent = spine;
                piece = inner;
            }
            else
            {
                piece->Load(FUN_1013f8a0(file, 0), 0, 0, 0);
                strcpy(piece->name, "hukusou");
                piece->parent = spine;
            }
            if (DAT_10daaae8[index].kind == BASE_KIND_PAIRED_WEAPONS || DAT_1030b668[index] == BASE_KIND_PAIRED_WEAPONS)
                piece->matrix = Scaling(BODY_SCALE_PAIRED_WEAPONS, BODY_SCALE_PAIRED_WEAPONS, BODY_SCALE_PAIRED_WEAPONS);
            else if (DAT_10daaae8[index].kind == BASE_KIND_PACKED_GROUP || DAT_1030b668[index] == BASE_KIND_PACKED_GROUP)
                piece->matrix = Scaling(BODY_SCALE_PACKED_GROUP, BODY_SCALE_PACKED_GROUP, BODY_SCALE_PACKED_GROUP);
            else if (DAT_10db6ee8[index].root != 0 && _strnicmp(DAT_10db6ee8[index].root->folder, "EN_023x0", 8) == 0)
                piece->matrix = Scaling(BODY_SCALE_SMALL, BODY_SCALE_SMALL, BODY_SCALE_SMALL);
            if (clothes == CLOTHES_SPLIT_A || clothes == CLOTHES_SPLIT_B)
                piece->parent->BuildMesh(1, 0);
            else
                piece->BuildMesh(1, 0);
        }
        DAT_102548c0[index] += clothes * EQUIP_FIELD_UNIT;
    }
clothesDone:
    if (mount != 0)
    {
        Node *old = DAT_10db6ee8[index].root->Find("@norisou", 0);
        if (old != 0)
            old->parent->RemoveChild(old, 0);
        switch (mount)
        {
        case 1:
            strcpy(file, "EEQ_15.it3");
            PreloadModel(scratch, "EEQ_15br.it3");
            PreloadModel(scratch, "EEQ_15d.it3");
            break;
        case 2:
            strcpy(file, "EEQ_25.it3");
            PreloadModel(scratch, "EEQ_25br.it3");
            PreloadModel(scratch, "EEQ_25d.it3");
            break;
        case 3:
            strcpy(file, "EEQ_25b.it3");
            PreloadModel(scratch, "EEQ_25bb.it3");
            PreloadModel(scratch, "EEQ_25bd.it3");
            break;
        default:
            goto mountDone;
        }
        Node *root = DAT_10db6ee8[index].root;
        if (root != 0)
        {
            float scale = 1.0f;
            Node *piece = root->AddChild(0, PIECE_NODE_CHILDREN);
            piece->Load(FUN_1013f8a0(file, 0), 0, 0, 0);
            strcpy(piece->name, "norisou");
            piece->parent = root;
            if (DAT_10daaae8[index].kind == BASE_KIND_PAIRED_WEAPONS || DAT_1030b668[index] == BASE_KIND_PAIRED_WEAPONS)
            {
                piece->matrix = Scaling(BODY_SCALE_PAIRED_WEAPONS, BODY_SCALE_PAIRED_WEAPONS, BODY_SCALE_PAIRED_WEAPONS);
                scale = BODY_SCALE_PAIRED_WEAPONS;
            }
            else if (DAT_10daaae8[index].kind == BASE_KIND_PACKED_GROUP || DAT_1030b668[index] == BASE_KIND_PACKED_GROUP)
            {
                piece->matrix = Scaling(BODY_SCALE_PACKED_GROUP, BODY_SCALE_PACKED_GROUP, BODY_SCALE_PACKED_GROUP);
                scale = BODY_SCALE_PACKED_GROUP;
            }
            else if (DAT_10db6ee8[index].root != 0 && _strnicmp(DAT_10db6ee8[index].root->folder, "EN_023x0", 8) == 0)
            {
                piece->matrix = Scaling(BODY_SCALE_SMALL, BODY_SCALE_SMALL, BODY_SCALE_SMALL);
                scale = BODY_SCALE_SMALL;
            }
            piece->BuildMesh(1, 0);
            Node *rider = piece->Find("@rider", 0);
            DAT_10db6ee8[index].root->matrix.SetTranslation(rider->matrix.GetTranslation() * scale);
            float inverseScale = -scale;
            piece->matrix.SetTranslation(rider->matrix.GetTranslation() * inverseScale);
        }
        DAT_10daaae8[index].held = 1;
        DAT_10db6ee8[index].root->M_10194630(index, MOUNT_COLOUR_MASK);
        DAT_10db6ee8[index].root->M_101946b0(MOUNT_COLOUR_GREEN, MOUNT_COLOUR_MASK);
        DAT_10db6ee8[index].root->M_10194730(MOUNT_COLOUR_BLUE, MOUNT_COLOUR_MASK);
        DAT_102548c0[index] += mount * MOUNT_FIELD_UNIT;
    }
mountDone:
    if (weapon != 0)
    {
        int hands = 0;
        Node *oldRight = DAT_10db6ee8[index].root->Find("@migitesou", 0);
        if (oldRight != 0)
            oldRight->parent->RemoveChild(oldRight, 0);
        Node *oldLeft = DAT_10db6ee8[index].root->Find("@hidaritesou", 0);
        if (oldLeft != 0)
            oldLeft->parent->RemoveChild(oldLeft, 0);
        switch (weapon)
        {
        case 0x32:
            strcpy(file, "EEQ_19.it3");
            PreloadModel(scratch, "EEQ_19d.it3");
            hands = 1;
            break;
        case 0x33:
            strcpy(file, "EEQ_19b.it3");
            PreloadModel(scratch, "EEQ_19bd.it3");
            hands = 1;
            break;
        case 0x34:
            strcpy(file, "EEQ_20.it3");
            PreloadModel(scratch, "EEQ_20d.it3");
            hands = 1;
            break;
        case 0x35:
            strcpy(file, "EEQ_20b.it3");
            PreloadModel(scratch, "EEQ_20bd.it3");
            hands = 1;
            break;
        case 0x36:
            strcpy(file, "EEQ_21.it3");
            strcpy(leftFile, "EEQ_21t.it3");
            PreloadModel(scratch, "EEQ_21d.it3");
            PreloadModel(scratch, "EEQ_21td.it3");
            if (DAT_10daaae8[index].twoHanded == 0 && alternative == 0)
                DAT_10daaae8[index].twoHanded = 1;
            hands = 2;
            break;
        case 0x37:
            strcpy(file, "EEQ_21b.it3");
            strcpy(leftFile, "EEQ_21bt.it3");
            PreloadModel(scratch, "EEQ_21bd.it3");
            PreloadModel(scratch, "EEQ21btd.it3");
            if (DAT_10daaae8[index].twoHanded == 0 && alternative == 0)
                DAT_10daaae8[index].twoHanded = 1;
            hands = 2;
            break;
        case 0x38:
            strcpy(file, "EEQ_22.it3");
            PreloadModel(scratch, "EEQ_22d.it3");
            hands = 1;
            break;
        case 0x39:
            strcpy(file, "EEQ_22b.it3");
            PreloadModel(scratch, "EEQ_22bd.it3");
            hands = 1;
            break;
        case 0x3a:
            strcpy(file, "EEQ_23.it3");
            PreloadModel(scratch, "EEQ_23d.it3");
            hands = 1;
            break;
        case 0x3b:
            strcpy(file, "EEQ_23b.it3");
            PreloadModel(scratch, "EEQ_23bd.it3");
            hands = 1;
            break;
        case 0x3c:
            strcpy(file, "EEQ_24r.it3");
            strcpy(leftFile, "EEQ_24l.it3");
            PreloadModel(scratch, "EEQ_24rd.it3");
            PreloadModel(scratch, "EEQ_24ld.it3");
            hands = 2;
            break;
        case 0x3d:
            strcpy(file, "EEQ_24b.it3");
            strcpy(leftFile, "EEQ_24b.it3");
            PreloadModel(scratch, "EEQ_24bd.it3");
            hands = 2;
            break;
        default:
            weapon = 0;
            break;
        }
        if (alternative != 0)
            hands = 1;
        if (weapon != 0)
        {
            switch (hands)
            {
            case 1:
            {
                Node *hand = DAT_10db6ee8[index].root->Find("@Bip01 R Hand", 0);
                if (hand != 0)
                {
                    Node *model = hand->AddChild(0, PIECE_NODE_CHILDREN);
                    model->Load(FUN_1013f8a0(file, 0), 0, 0, 0);
                    strcpy(model->name, "migitesou");
                    model->parent = hand;
                    model->BuildMesh(1, 0);
                }
                break;
            }
            case 2:
            {
                Node *hand = DAT_10db6ee8[index].root->Find("@Bip01 R Hand", 0);
                if (hand != 0)
                {
                    Node *model = hand->AddChild(0, PIECE_NODE_CHILDREN);
                    model->Load(FUN_1013f8a0(file, 0), 0, 0, 0);
                    strcpy(model->name, "migitesou");
                    model->parent = hand;
                    model->BuildMesh(1, 0);
                }
                hand = DAT_10db6ee8[index].root->Find("@Bip01 L Hand", 0);
                if (hand != 0)
                {
                    Node *model = hand->AddChild(0, PIECE_NODE_CHILDREN);
                    model->Load(FUN_1013f8a0(leftFile, 0), 0, 0, 0);
                    strcpy(model->name, "hidaritesou");
                    model->parent = hand;
                    model->BuildMesh(1, 0);
                }
                break;
            }
            }
            DAT_102548c0[index] += weapon * WEAPON_FIELD_UNIT;
        }
    }
    DAT_10daaae8[index].stats[STAT_EFFECT_PARAM] = (int)(effectParam + ROUNDING_HALF);
    DAT_10daaae8[index].stats[STAT_VALUE_4] = (int)(value4 + ROUNDING_HALF);
    DAT_10daaae8[index].m_0143 = marks;
    if (DAT_10f654b0 != 0)
        DAT_10db6ee8[index].root->M_1019b0c0(0, 0, MODEL_DEBUG_FLAGS);
    if (skipKindSelect == 0)
        FUN_1001b0a0(index, 0, 0);
    return 0;
}
