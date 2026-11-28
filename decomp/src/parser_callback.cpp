// The game's callback for the script parser's messages (installed into the ListBank by Script_RegisterGameBuiltins).

#include <stdio.h>
#include <string.h>

#include "listbank.h"
#include "menu_main.h"
#include "script_vars.h"

// "Script error occurred (UPIPE)" in Shift-JIS, the title of the message window that shows a script error.
#define SCRIPT_ERROR_TITLE "\x83X\x83N\x83\x8a\x83v\x83g\x83G\x83\x89\x81[\x94\xad\x90\xb6\x81i\x82t\x82o\x82h\x82o\x82" "d\x81j"
// "<text> (<offset>)" with full-width parentheses: the error text and where in the script it happened.
#define SCRIPT_ERROR_FORMAT "%s\x81i%d\x81j"
// Number of frames the error message window stays open.
#define SCRIPT_ERROR_FRAMES 0x96


// 0x10017f20. The script parser calls this twice for each error: first with the error text (kept in DAT_102194f0),
// then with the offset of the error in the script, which opens a message window with the text and the offset. The
// parser never sends code 0, which would store its value in DAT_104c0208. Always returns 1.
extern "C" int FUN_10017f20(int code, int value)
{
    char message[0x100];
    if (code == PARSER_MESSAGE_ERROR_OFFSET)
    {
        sprintf(message, SCRIPT_ERROR_FORMAT, DAT_102194f0, value);
        FUN_1000ed70(SCRIPT_ERROR_TITLE, message, SCRIPT_ERROR_FRAMES);
    }
    else if (code == PARSER_MESSAGE_STORE)
        DAT_104c0208 = value;
    else
        strcpy(DAT_102194f0, (const char *)value);
    return 1;
}
