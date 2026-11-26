// Opening a message box: up to four text lines with the speaker's name, each expanded (names, then the text
// substitutions of 0x1000c020) into the box's buffers.

#include <string.h>

#include "message_text.h"

#include "char_slot.h"
#include "game_vars.h"

#include "char_vars.h"
#include "name_paths.h"

extern "C" int FUN_1003ad20(int sound, const void *position, float volume);
extern "C" int FUN_1000c020(char *out, const char *text);     // Text_Substitute

// 0x10056260 (Message_Open). Opens message box `slot` with the lines `line1` to `line4` (0 = empty) spoken by
// character `slot`, playing sound 0x38e when the first line has text. Returns 1.
extern "C" int FUN_10056260(int slot, const char *line1, const char *line2, const char *line3, const char *line4)
{
    char text[0x100];
    if (line1 != 0 && line1[0] != 0)
        FUN_1003ad20(0x38e, 0, 1.0f);
    DAT_104bfd5c = 0;
    DAT_104bfb08[slot] = 1;
    DAT_104481d8[slot] = 0;
    DAT_10467c20[slot] = 0;
    if (line1 != 0)
        FUN_1013e790(DAT_10453208[slot].lines[0], line1);
    else
        DAT_10453208[slot].lines[0][0] = 0;
    char *text2 = DAT_10453208[slot].lines[1];
    if (line2 != 0)
        FUN_1013e790(text2, line2);
    else
        text2[0] = 0;
    char *text3 = DAT_10453208[slot].lines[2];
    if (line3 != 0)
        FUN_1013e790(text3, line3);
    else
        text3[0] = 0;
    char *text4 = DAT_10453208[slot].lines[3];
    if (line4 != 0)
        FUN_1013e790(text4, line4);
    else
        text4[0] = 0;
    if (slot == 0)
    {
        if (DAT_10c655f0 == 6)
            strcpy(text, "\x83|\x83|\x83\x93");
        else
            strcpy(text, DAT_10daaae8[0].name);
    }
    else
        strcpy(text, DAT_10daaae8[slot].name);
    FUN_1000c020(DAT_10453208[slot].speaker, text);
    if (line1 != 0)
    {
        char *text1 = DAT_10453208[slot].lines[0];
        FUN_1000c020(text, text1);
        strcpy(text1, text);
    }
    if (line2 != 0)
    {
        FUN_1000c020(text, text2);
        strcpy(text2, text);
    }
    if (line3 != 0)
    {
        FUN_1000c020(text, text3);
        strcpy(text3, text);
    }
    if (line4 != 0)
    {
        FUN_1000c020(text, text4);
        strcpy(text4, text);
    }
    return 1;
}
