// Game::LoadMapData: parses bin\map.itm, the map data file, into the tables of the Game object.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "map_front.h"
#include "texture_load_bmp.h"

extern "C" int FUN_10161120(char *out, const char *path);   // Path_GetBaseName

// Lines of the file between two calls of the progress callback.
#define PROGRESS_LINE_INTERVAL 10
// Extension of the data file: map.txt is read from its scrambled copy map.itm.
#define DATA_EXTENSION_LENGTH 3
#define DATA_EXTENSION "itm"
// Bytes of the file after the text: three numbers, the second and third are the descramble key and checksum.
#define ITM_TRAILER_SIZE 12
// Free room after the text.
#define TEXT_EXTRA_SIZE 100
// Script flags cleared at the start (a little more than the 4000 the flags use).
#define FLAG_CLEAR_COUNT 0x1000
// Size of the tables, as the parser checks them.
#define FACE_ROW_LIMIT 0x30
#define FACE_COLUMN_LIMIT 0x40
#define CHAR_POS_LIMIT 0x100
#define CHAR_MONSTER_POS_LIMIT 0x5a0
#define TRIGGER_LIMIT 3400
#define EVENT_LIMIT 2000
#define STAGE_NAME_LIMIT 0x40
#define BGM_LIMIT 0x40
#define SKILL_LIMIT 0x40
#define ABILITY_LIMIT 0x80
#define CC_LIMIT 0x100
#define CHAR_TABLE_LIMIT 160
#define OBJ_ANIME_LIMIT 0x400
#define FACE_FRAME_LIMIT 0x80
// Strings each record is checked for (the last character of a string must stay the terminator).
#define SKILL_LABEL_COUNT 0x1c
#define ABILITY_LABEL_COUNT 0x18
#define TRIGGER_LABEL_COUNT 8
#define CC_LABEL_COUNT 8
#define CHAR_POS_LABEL_COUNT 11
#define OBJ_ANIME_LABEL_COUNT 8
// Numbers read for each of the lines of the tables.
#define SKILL_LINE_COUNT 0x19
#define MOVE_FRAME_STEP 5
#define MOVE_FRAME_END 100

// 0x10002440 (MapData_Load). Reads the file `path` with its extension changed to "itm", descrambles it, and parses
// its sections ("#VERSION", "#MapTable", "#CharTable", ... each ended by its own "#End..." line) into the tables of
// the object. `progress` is called every PROGRESS_LINE_INTERVAL lines while parsing. A malformed section (a table
// that overflows, or the text ending inside a section) crashes on purpose with fread on a null file. Returns 1 when
// the whole text was parsed, 0 when the file does not descramble.
int Game::LoadMapData(const char *path, void (*progress)(const char *, int))
{
    MapFront &front = *(MapFront *)this;
    front.progress = progress;
    memset(m_3ee694, 0, FLAG_CLEAR_COUNT * sizeof(int));
    for (int i = 0; i < FACE_FRAME_LIMIT; i++)
        faceFrame[i].first = MAP_FACE_FRAME_NONE;
    memset(charMot, 0, sizeof(charMot));
    memset(front.skills, 0, sizeof(front.skills));
    memset(battle, 0, sizeof(battle));
    memset(front.faces, 0, sizeof(front.faces));
    memset(ability, 0, sizeof(ability));
    memset(triggers, 0, sizeof(triggers));
    memset(mapEvents, 0, sizeof(mapEvents));
    memset(queuedTriggers, 0, sizeof(queuedTriggers));
    memset(charPos, 0, sizeof(charPos));
    memset(charMonsPos, 0, sizeof(charMonsPos));
    memset(front.cc, 0, sizeof(front.cc));
    memset(charTable, 0, sizeof(charTable));
    memset(objAnime, 0, sizeof(objAnime));
    for (int i = 0; i < SCRIPT_LIMIT; i++)
    {
        if (scripts[i] != 0)
            free(scripts[i]);
        scripts[i] = 0;
    }
    currentMap = 1;
    pad_3ee68c[0] = 0;
    pad_3ee68c[1] = 0;
    pos = 0;
    m_a4614 = 0;
    queuedTriggerCount = 0;
    charCount = 0;
    charPosCount = 0;
    charMonsPosCount = 0;
    mapEventCount = 0;
    triggerCount = 0;
    objAnimeCount = 0;
    front.btMapCount = 0;
    front.ccCount = 0;

    char dataPath[260];
    strcpy(dataPath, path);
    strcpy(dataPath + strlen(dataPath) - DATA_EXTENSION_LENGTH, DATA_EXTENSION);
    FILE *file = fopen(dataPath, "rb");
    if (file != 0)
    {
        char baseName[256];
        FUN_10161120(baseName, path);
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        end = fileSize - ITM_TRAILER_SIZE;
        text = new char[end + TEXT_EXTRA_SIZE];
        fseek(file, 0, SEEK_SET);
        fread(text, 1, end, file);
        int header;
        int key;
        unsigned int checksum;
        fread(&header, 1, 4, file);
        fread(&key, 1, 4, file);
        fread(&checksum, 1, 4, file);
        fclose(file);
        if (FUN_101615d0((unsigned char *)text, end, baseName, key, checksum) == 0)
        {
            free(text);
            text = 0;
            end = 0;
            return 0;
        }
    }

    unsigned int lineCount = 0;
    for (;;)
    {
        if (pos >= end)
            break;
        if (lineCount % PROGRESS_LINE_INTERVAL == 0 && front.progress != 0)
            front.progress(0, 0);
        lineCount++;
        ReadWord();
        if (strcmp(token, "#VERSION") == 0)
        {
            ReadWordInLine();
            mapVersion = atol(token);
        }
        if (strcmp(token, "#USDG") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndUSDG") == 0)
                    break;
                int *value = usdg[atol(token)].values;
                for (int k = 0; k < 5; k++)
                {
                    ReadWordInLine();
                    *value++ = atol(token);
                }
            }
        }
        if (strcmp(token, "#StageName") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndStageName") == 0)
                    break;
                int stage = atol(token);
                if (stage >= 0 && stage < STAGE_NAME_LIMIT)
                {
                    ReadWordInLine();
                    strcpy(front.stageNames[stage], token);
                    ReadWordInLine();
                    strcpy(front.stageCodes[stage], token);
                }
            }
        }
        if (strcmp(token, "#MapTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndMapTable") == 0)
                    break;
                m_95e14[m_a4614].map = atol(token);
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                m_95e14[m_a4614].flags[0] = atol(token);
                ReadWordInLine();
                m_95e14[m_a4614].flags[1] = atol(token);
                ReadWordInLine();
                m_95e14[m_a4614].flags[2] = atol(token);
                ReadWordInLine();
                m_95e14[m_a4614].flags[3] = atol(token);
                ReadWordInLine();
                strcpy(m_95e14[m_a4614].name, token);
                ReadWordInLine();
                strcpy(m_95e14[m_a4614].file, token);
                ReadWordInLine();
                strcpy(m_95e14[m_a4614].title, token);
                ReadWordInLine();
                m_95e14[m_a4614].m_0094[0] = atol(token);
                ReadWordInLine();
                m_95e14[m_a4614].m_0094[1] = atol(token);
                ReadWordInLine();
                m_95e14[m_a4614].m_0094[2] = atol(token);
                ReadWordInLine();
                m_95e14[m_a4614].value = atol(token);
                ReadWordInLine();
                strcpy(m_95e14[m_a4614].text[0], token);
                ReadWordInLine();
                strcpy(m_95e14[m_a4614].text[1], token);
                m_a4614++;
            }
            continue;
        }
        if (strcmp(token, "#BGMTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndBGMTable") == 0)
                    break;
                int track = atol(token);
                ReadWordInLine();
                strcpy(bgm[track].name, token);
                ReadWordInLine();
                strcpy(bgm[track].file, token);
                ReadWordInLine();
                bgm[track].values[0] = atol(token);
                ReadWordInLine();
                bgm[track].values[1] = atol(token);
                ReadWordInLine();
                bgm[track].values[2] = atol(token);
                ReadWordInLine();
                bgm[track].values[3] = atol(token);
            }
            continue;
        }
        if (strcmp(token, "#LVTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#LVTableEnd") == 0)
                    break;
                int *value = front.levels[atol(token)];
                for (int k = 0; k < 5; k++)
                {
                    ReadWordInLine();
                    *value++ = atol(token);
                }
            }
            continue;
        }
        if (strcmp(token, "#FaceFrameTable") == 0)
        {
            int frame = 0;
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndFaceFrameTable") == 0)
                    break;
                faceFrame[frame].first = (short)atol(token);
                ReadWordInLine();
                faceFrame[frame].second = (short)atol(token);
                frame++;
            }
            continue;
        }
        if (strcmp(token, "#SkillTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndSkillTable") == 0)
                    break;
                MapSkillEntry *skill = &front.skills[atol(token)];
                int *value = skill->values;
                char *label = skill->labels[0];
                for (int k = 0; k < SKILL_LINE_COUNT; k++)
                {
                    ReadWordInLine();
                    strcpy(label, token);
                    *value++ = atol(token);
                    label += MAP_LABEL_SIZE;
                }
            }
            continue;
        }
        if (strcmp(token, "#MoveTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndMoveTable") == 0)
                    break;
                KeyBank *motion = &front.skills[atol(token)].motion;
                motion->Clear();
                for (int frame = 0; frame < MOVE_FRAME_END; frame += MOVE_FRAME_STEP)
                {
                    ReadWordInLine();
                    motion->Set(frame, (float)atol(token), 0.0f, 0.0f, 0.0f);
                }
            }
            continue;
        }
        if (strcmp(token, "#MapEventTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndMapEventTable") == 0)
                    break;
                queuedTriggers[queuedTriggerCount].map = atol(token);
                ReadWordInLine();
                queuedTriggers[queuedTriggerCount].value = atol(token);
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                queuedTriggers[queuedTriggerCount].flags[0] = atol(token);
                ReadWordInLine();
                queuedTriggers[queuedTriggerCount].flags[1] = atol(token);
                ReadWordInLine();
                queuedTriggers[queuedTriggerCount].flags[2] = atol(token);
                ReadWordInLine();
                queuedTriggers[queuedTriggerCount].flags[3] = atol(token);
                ReadWordInLine();
                strcpy(queuedTriggers[queuedTriggerCount].name, token);
                for (int k = 0; k < 6; k++)
                {
                    ReadWordInLine();
                    strcpy(queuedTriggers[queuedTriggerCount].labels[k], token);
                    queuedTriggers[queuedTriggerCount].values[k] = atol(token);
                }
                queuedTriggerCount++;
                if (queuedTriggerCount >= EVENT_LIMIT)
                    fread(0, 1, 1, 0);
            }
            continue;
        }
        if (strcmp(token, "#CharTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndCharTable") == 0)
                    break;
                charTable[charCount].id = atol(token);
                ReadWordInLine();
                strcpy(charTable[charCount].name, token);
                ReadWordInLine();
                strcpy(charTable[charCount].label, token);
                ReadWordInLine();
                charTable[charCount].kind = atol(token);
                ReadWordInLine();
                charTable[charCount].speedPercent = atol(token);
                ReadWordInLine();
                charTable[charCount].m_008c = atol(token);
                ReadWordInLine();
                charTable[charCount].faceSet = atol(token);
                for (int k = 0; k < 8; k++)
                {
                    ReadWordInLine();
                    strcpy(charTable[charCount].labels[k], token);
                    charTable[charCount].values[k] = atol(token);
                }
                charCount++;
            }
            continue;
        }
        if (strcmp(token, "#FaceTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndFaceTable") == 0)
                    break;
                int row = atol(token);
                if (row > FACE_ROW_LIMIT - 1)
                    fread(0, 1, 1, 0);
                ReadWordInLine();
                int column = atol(token);
                if (column > FACE_COLUMN_LIMIT - 1)
                    fread(0, 1, 1, 0);
                ReadWordInLine();
                strcpy(front.faces[row].names[column], token);
                ReadWordInLine();
                front.faces[row].frameA[column] = atol(token);
                ReadWordInLine();
                front.faces[row].frameB[column] = atol(token);
            }
            continue;
        }
        if (strcmp(token, "#SE_Table") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndSE_Table") == 0)
                    break;
                int effect = atol(token);
                ReadWordInLine();
                strcpy(se[effect].name, token);
                ReadWordInLine();
                se[effect].values[0] = atol(token);
                ReadWordInLine();
                se[effect].values[1] = atol(token);
                ReadWordInLine();
                se[effect].values[2] = atol(token);
                ReadWordInLine();
                se[effect].values[3] = atol(token);
            }
            continue;
        }
        if (strcmp(token, "#CharPos") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndCharPos") == 0)
                    break;
                charPos[charPosCount].id = atol(token);
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                charPos[charPosCount].m_0008 = atol(token);
                ReadWordInLine();
                charPos[charPosCount].m_000c = atol(token);
                ReadWordInLine();
                charPos[charPosCount].m_0010 = atol(token);
                ReadWordInLine();
                charPos[charPosCount].m_0014 = atol(token);
                ReadWordInLine();
                charPos[charPosCount].m_0004 = atol(token);
                for (int k = 0; k < 7; k++)
                {
                    ReadWordInLine();
                    strcpy(charPos[charPosCount].labels[k], token);
                    charPos[charPosCount].values[k] = atol(token);
                }
                charPosCount++;
                if (charPosCount >= CHAR_POS_LIMIT)
                    fread(0, 1, 1, 0);
            }
            continue;
        }
        if (strcmp(token, "#CharMonsPos") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndCharMonsPos") == 0)
                    break;
                charMonsPos[charMonsPosCount].id = atol(token);
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                charMonsPos[charMonsPosCount].m_0008 = atol(token);
                ReadWordInLine();
                charMonsPos[charMonsPosCount].m_000c = atol(token);
                ReadWordInLine();
                charMonsPos[charMonsPosCount].m_0010 = atol(token);
                ReadWordInLine();
                charMonsPos[charMonsPosCount].m_0014 = atol(token);
                ReadWordInLine();
                charMonsPos[charMonsPosCount].m_0004 = atol(token);
                for (int k = 0; k < 11; k++)
                {
                    ReadWordInLine();
                    strcpy(charMonsPos[charMonsPosCount].labels[k], token);
                    charMonsPos[charMonsPosCount].values[k] = atol(token);
                }
                charMonsPosCount++;
                if (charMonsPosCount >= CHAR_MONSTER_POS_LIMIT)
                    fread(0, 1, 1, 0);
            }
            continue;
        }
        if (strcmp(token, "#EventTrigger") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndEventTrigger") == 0)
                    break;
                mapEvents[mapEventCount].map = atol(token);
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                mapEvents[mapEventCount].flags[0] = atol(token);
                ReadWordInLine();
                mapEvents[mapEventCount].flags[1] = atol(token);
                ReadWordInLine();
                mapEvents[mapEventCount].flags[2] = atol(token);
                ReadWordInLine();
                mapEvents[mapEventCount].flags[3] = atol(token);
                ReadWordInLine();
                mapEvents[mapEventCount].value = atol(token);
                ReadWordInLine();
                strcpy(mapEvents[mapEventCount].name, token);
                for (int k = 0; k < 6; k++)
                {
                    ReadWordInLine();
                    strcpy(mapEvents[mapEventCount].labels[k], token);
                    mapEvents[mapEventCount].values[k] = atol(token);
                }
                mapEventCount++;
                if (mapEventCount >= EVENT_LIMIT)
                    fread(0, 1, 1, 0);
            }
            continue;
        }
        if (strcmp(token, "#BtMapTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndBtMapTable") == 0)
                    break;
                front.btMap[front.btMapCount].first = atol(token);
                ReadWordInLine();
                front.btMap[front.btMapCount].second = atol(token);
                for (int k = 0; k < 2; k++)
                {
                    ReadWordInLine();
                    strcpy(front.btMap[front.btMapCount].labels[k], token);
                    front.btMap[front.btMapCount].values[k] = atol(token);
                }
                front.btMapCount++;
            }
            continue;
        }
        if (strcmp(token, "#MapEventTrigger") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndMapEventTrigger") == 0)
                    break;
                triggers[triggerCount].map = atol(token);
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                ReadWordInLine();
                triggers[triggerCount].flags[0] = atol(token);
                ReadWordInLine();
                triggers[triggerCount].flags[1] = atol(token);
                ReadWordInLine();
                triggers[triggerCount].flags[2] = atol(token);
                ReadWordInLine();
                triggers[triggerCount].flags[3] = atol(token);
                ReadWordInLine();
                triggers[triggerCount].value = atol(token);
                ReadWordInLine();
                strcpy(triggers[triggerCount].name, token);
                for (int k = 0; k < 8; k++)
                {
                    ReadWordInLine();
                    strcpy(triggers[triggerCount].labels[k], token);
                    triggers[triggerCount].values[k] = atol(token);
                }
                triggerCount++;
                if (triggerCount >= TRIGGER_LIMIT)
                    fread(0, 1, 1, 0);
            }
            continue;
        }
        if (strcmp(token, "#ObjAnimeTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndObjAnimeTable") == 0)
                    break;
                objAnime[objAnimeCount].id = atol(token);
                ReadWordInLine();
                strcpy(objAnime[objAnimeCount].name, token);
                ReadWordInLine();
                objAnime[objAnimeCount].m_0024 = atol(token);
                ReadWordInLine();
                objAnime[objAnimeCount].m_0028 = atol(token);
                ReadWordInLine();
                objAnime[objAnimeCount].m_002c = atol(token);
                for (int k = 0; k < 2; k++)
                {
                    ReadWordInLine();
                    strcpy(objAnime[objAnimeCount].labels[k], token);
                    objAnime[objAnimeCount].values[k] = atol(token);
                }
                objAnimeCount++;
            }
            continue;
        }
        if (strcmp(token, "#ItemTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndItemTable") == 0)
                    break;
                MapItemEntry *item = &front.items[atol(token)];
                ReadWordInLine();
                strcpy(item->name, token);
                int *value = item->values;
                for (int k = 0; k < 22; k++)
                {
                    ReadWordInLine();
                    *value++ = atol(token);
                }
                ReadWordInLine();
                strcpy(item->text, token);
            }
            continue;
        }
        if (strcmp(token, "#AbTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndAbTable") == 0)
                    break;
                MapAbilityEntry *entry = &ability[atol(token)];
                ReadWordInLine();
                strcpy(entry->name, token);
                for (int k = 0; k < 15; k++)
                {
                    ReadWordInLine();
                    entry->values[k] = atol(token);
                    strcpy(entry->labels[k], token);
                }
                ReadWordInLine();
                strcpy(entry->text, token);
            }
            continue;
        }
        if (strcmp(token, "#StatusTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndStatusTable") == 0)
                    break;
                MapStatusEntry *entry = &status[atol(token)];
                ReadWordInLine();
                strcpy(entry->name, token);
                int *value = entry->values;
                for (int k = 0; k < 17; k++)
                {
                    ReadWordInLine();
                    *value++ = atol(token);
                }
                ReadWordInLine();
                strcpy(entry->text, token);
            }
            continue;
        }
        if (strcmp(token, "#BtMotTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndBtMotTable") == 0)
                    break;
                MapBtMotEntry *entry = &btMot[atol(token)];
                ReadWordInLine();
                strcpy(entry->name, token);
                for (int k = 0; k < 7; k++)
                {
                    ReadWordInLine();
                    strcpy(entry->labels[k], token);
                    entry->values[k] = atol(token);
                }
            }
            continue;
        }
        if (strcmp(token, "#CCTable") == 0)
        {
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndCCTable") == 0)
                    break;
                int index = atol(token);
                ReadWordInLine();
                strcpy(front.cc[index].name, token);
                if (index > front.ccCount - 1)
                    front.ccCount = index + 1;
            }
            continue;
        }
        if (strncmp(token, "#CharMotTable:", 14) == 0)
        {
            MapCharMotion *motion = &charMot[atol(token + 14)];
            int lines = 0;
            ReadWord();
            strcpy(motion->name, token);
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndCharMotTable") == 0)
                    break;
                strcpy(motion->labels[lines], token);
                for (int k = 0; k < 6; k++)
                {
                    ReadWordInLine();
                    motion->keys[lines].values[k] = atol(token);
                }
                lines++;
                if (lines > MAP_MOTION_KEY_COUNT)
                    fread(0, 1, 1, 0);
                motion->count = lines;
            }
            continue;
        }
        if (strncmp(token, "#BattleSc2:", 10) == 0)
        {
            int group = atol(token + 11);
            for (;;)
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
                if (strcmp(token, "#EndBattleSc2") == 0)
                    break;
                int *value = battle[group][atol(token)].values;
                for (int k = 0; k < 14; k++)
                {
                    ReadWordInLine();
                    *value++ = atol(token);
                }
            }
            continue;
        }
        if (strncmp(token, "#Script:", 8) == 0)
        {
            int script = atol(token + 8);
            int start = pos;
            do
            {
                ReadWord();
                if (pos >= end)
                    goto fail;
            } while (strcmp(token, "#EndScript") != 0);
            int length = pos - 9 - start;
            scripts[script] = (char *)operator new(length + 0x40);
            memcpy(scripts[script], text + start, length);
            scripts[script][length - 1] = 0;
        }
    }
        for (int i = 0; i < BGM_LIMIT; i++)
        {
            if (bgm[i].name[MAP_LABEL_SIZE - 1] != 0 || bgm[i].file[MAP_LABEL_SIZE - 1] != 0)
                fread(0, 1, 1, 0);
        }
        for (int i = 0; i < SKILL_LIMIT; i++)
        {
            for (int k = 0; k < SKILL_LABEL_COUNT; k++)
            {
                if (front.skills[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < 0x40; i++)
        {
            for (int k = 0; k < MAP_MOTION_KEY_COUNT; k++)
            {
                if (charMot[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < FACE_ROW_LIMIT; i++)
        {
            for (int k = 0; k < FACE_COLUMN_LIMIT; k++)
            {
                if (front.faces[i].names[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < ABILITY_LIMIT; i++)
        {
            for (int k = 0; k < ABILITY_LABEL_COUNT; k++)
            {
                if (ability[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < TRIGGER_LIMIT; i++)
        {
            for (int k = 0; k < TRIGGER_LABEL_COUNT; k++)
            {
                if (triggers[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < EVENT_LIMIT; i++)
        {
            for (int k = 0; k < TRIGGER_LABEL_COUNT; k++)
            {
                if (mapEvents[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < EVENT_LIMIT; i++)
        {
            for (int k = 0; k < TRIGGER_LABEL_COUNT; k++)
            {
                if (queuedTriggers[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < CHAR_POS_LIMIT; i++)
        {
            for (int k = 0; k < CHAR_POS_LABEL_COUNT; k++)
            {
                if (charPos[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < CHAR_MONSTER_POS_LIMIT; i++)
        {
            for (int k = 0; k < CHAR_POS_LABEL_COUNT; k++)
            {
                if (charMonsPos[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < CC_LIMIT; i++)
        {
            for (int k = 0; k < CC_LABEL_COUNT; k++)
            {
                if (front.cc[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < CHAR_TABLE_LIMIT; i++)
        {
            for (int k = 0; k < CHAR_POS_LABEL_COUNT; k++)
            {
                if (charTable[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
        for (int i = 0; i < OBJ_ANIME_LIMIT; i++)
        {
            for (int k = 0; k < OBJ_ANIME_LABEL_COUNT; k++)
            {
                if (objAnime[i].labels[k][MAP_LABEL_SIZE - 1] != 0)
                    fread(0, 1, 1, 0);
            }
        }
    return 1;

fail:
    fread(0, 1, 1, 0);
    return 0;
}
