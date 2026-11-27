// test.dat: groups of words (each up to 31 characters) read at startup; FUN_1000c020 replaces them in message text.
// A group starts after one skipped line and ends at an empty line or a ';' line.

#include <mbctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"

// Word tables, per group and word.
extern "C" int FUN_1013f880(void);               // the game's random number
extern "C" const char *FUN_1013f8a0(const char *name, int folder);   // Path_Data

// 0x1000beb0 (TestDat_Load). Reads test.dat from the data folder into the word groups (more than 256 words in a group
// or 128 groups crash on purpose through fread(0, 1, 1, 0)). Returns 1, or 0 when the file is missing.
extern "C" int TestDat_Load(void)
{
    char line[128];
    FILE *file = fopen(FUN_1013f8a0("test.dat", 0), "rt");
    if (file == 0)
        return 0;
    DAT_104c02b0 = 0;
    int count = 0;
    do
    {
        while (1)
        {
            line[0] = 0;
            fgets(line, sizeof(line), file);
            if (count == 0)
                fgets(line, sizeof(line), file);
            int length = strlen(line);
            if (length > 0)
                line[length - 1] = 0;
            if (line[0] == 0 || line[0] == ';')
                break;
            DAT_1030ceb8[DAT_104c02b0][count] = 0xff;
            strcpy(DAT_1032d870[DAT_104c02b0][count], line);
            DAT_104487e8[DAT_104c02b0][count] = strlen(line);
            count++;
            if (count > 255)
                fread(0, 1, 1, 0);
        }
        DAT_103084a8[DAT_104c02b0] = count;
        count = 0;
        DAT_104c02b0++;
        if (DAT_104c02b0 > 127)
            fread(0, 1, 1, 0);
    } while (line[0] != 0);
    fclose(file);
    return 1;
}

// 0x1000c020 (Text_Substitute). Copies message `text` to `out`, replacing words of the test.dat groups (when
// DAT_10c66cc0 is set): at each double-byte character the longest group word that starts there is replaced by
// another word of its group, picked at random the first time (up to ten tries to differ from the word itself) and
// kept for that word afterwards. Text starting with '#' is copied without the '#' and without replacements.
// Returns 1 when the text was copied unchanged, 0 after the replacement pass.
extern "C" int FUN_1000c020(char *out, const char *text)
{
    if (text[0] == '#')
    {
        strcpy(out, text + 1);
        return 1;
    }
    if (DAT_10c66cc0 == 0)
    {
        strcpy(out, text);
        return 1;
    }
    *out = 0;
    while (*text != 0)
    {
        if (_ismbblead((unsigned char)*text))
        {
            int best = 0;
            int bestGroup;
            int bestWord;
            for (int group = 0; group < DAT_104c02b0; group++)
            {
                for (int word = 0; word < DAT_103084a8[group]; word++)
                {
                    if (DAT_104487e8[group][word] > best &&
                        strncmp(text, DAT_1032d870[group][word], DAT_104487e8[group][word]) == 0)
                    {
                        best = DAT_104487e8[group][word];
                        bestGroup = group;
                        bestWord = word;
                    }
                }
            }
            if (best != 0)
            {
                int pick;
                if (DAT_1030ceb8[bestGroup][bestWord] != 0xff)
                    pick = DAT_1030ceb8[bestGroup][bestWord];
                else
                {
                    // first use: another word of the group (at most ten tries)
                    pick = bestWord;
                    int tries = 0;
                    while (pick == bestWord && tries < 10)
                    {
                        tries++;
                        pick = FUN_1013f880() % DAT_103084a8[bestGroup];
                        DAT_1030ceb8[bestGroup][bestWord] = pick;
                    }
                }
                strcat(out, DAT_1032d870[bestGroup][pick]);
                out += DAT_104487e8[bestGroup][pick];
                text += best;
                *out = 0;
            }
            else
            {
                *out++ = *text++;
                *out++ = *text++;
                *out = 0;
            }
        }
        else
        {
            *out++ = *text++;
            *out = 0;
        }
    }
    *out = 0;
    return 0;
}
