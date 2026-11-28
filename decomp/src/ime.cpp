// Japanese IME support for text input (the name entry screen). The game reads the composition and candidates
// itself and draws them with its own font; ImeContext wraps the Imm32 API.

#include <windows.h>
#include <imm.h>
#include <stdlib.h>
#include <string.h>

#include "ime.h"
#include "render_window.h"

// Stores `value` in slot `index` of the first state array.
int StateArrays::M_1015caa0(int value, int index)
{
    m_0020[index] = value;
    return 1;
}

// Stores `value` in slot `index` of the second state array (floats: the value is copied without the FPU).
int StateArrays::M_1015cac0(float value, int index)
{
    m_0040[index] = value;
    return 1;
}

// Constructor: no window or context yet.
ImeContext::ImeContext()
{
    window = 0;
    context = 0;
    buffer = 0;
}

// Releases the input context and frees the candidate buffer.
int ImeContext::Release()
{
    if (buffer != 0)
    {
        free(buffer);
        buffer = 0;
    }
    if (context != 0)
    {
        ImmReleaseContext(window, context);
        context = 0;
        window = 0;
    }
    return 1;
}

// Takes the input context of window `target`.
int ImeContext::Attach(HWND target)
{
    Release();
    if (target != 0)
    {
        window = target;
        context = ImmGetContext(target);
    }
    return 1;
}

// 1 when the IME is switched on.
int ImeContext::IsOpen()
{
    return ImmGetOpenStatus(context) != 0;
}

// Switches the IME on or off.
int ImeContext::SetOpen(BOOL open)
{
    return ImmSetOpenStatus(context, open) != 0;
}

// Reads the conversion and sentence modes.
int ImeContext::GetConversion(DWORD *conversion, DWORD *sentence)
{
    return ImmGetConversionStatus(context, conversion, sentence) != 0;
}

// Sets the conversion and sentence modes.
int ImeContext::SetConversion(const DWORD *conversion, const DWORD *sentence)
{
    return ImmSetConversionStatus(context, *conversion, *sentence) != 0;
}

// Returns the composition cursor position.
LONG ImeContext::GetCompositionValue()
{
    LONG value[2];
    if (ImmGetCompositionStringA(context, 0x20, value, sizeof(value)) == 0)
        return 0;
    return value[1];
}

// Returns the size of the composition attribute data (GCS_COMPATTR) after querying the clause data.
LONG ImeContext::M_1015d400()
{
    ImmGetCompositionStringA(context, 0x100, 0, 0);
    return ImmGetCompositionStringA(context, 0x80, 0, 0);
}

// Returns the selected candidate.
DWORD ImeContext::GetSelection()
{
    if (buffer == 0)
        return 0;
    return buffer->dwSelection;
}

// Returns candidate string `index` from the loaded candidate list.
const char *ImeContext::GetCandidate(int index)
{
    CANDIDATELIST *list = buffer;
    if (list == 0)
        return 0;
    return (const char *)list + list->dwOffset[index];
}

// Returns the first candidate on the current page.
DWORD ImeContext::GetPageStart()
{
    if (buffer == 0)
        return 0;
    return buffer->dwPageStart;
}

// Returns the number of candidates.
DWORD ImeContext::GetCount()
{
    if (buffer == 0)
        return 0;
    return buffer->dwCount;
}

// Returns the number of candidates per page.
DWORD ImeContext::GetPageSize()
{
    if (buffer == 0)
        return 0;
    return buffer->dwPageSize;
}

// 0x1015d4f0. Destructor: releases the input context and the candidate buffer (a tail call of Release). Nothing in the
// game destroys the global IME context; the linker kept the copy.
ImeContext::~ImeContext()
{
    Release();
}

#include "ui_vars.h"
#include "menu_text.h"

// Stores the IME option value DAT_10f66bbc. Returns 1.
extern "C" int FUN_1015d500(int value)
{
    DAT_10f66bbc = value;
    return 1;
}

// 1 when the game's IME is switched on.
extern "C" int FUN_1015d510(void)
{
    return DAT_10f68124.IsOpen();
}

// Copies the finished (converted) text into `out`.
int ImeContext::GetResultString(char *out)
{
    *out = 0;
    LONG length = ImmGetCompositionStringA(context, GCS_RESULTSTR, 0, 0);
    if (length == 0)
        return 0;
    char *text = (char *)operator new(length + 1);
    ImmGetCompositionStringA(context, GCS_RESULTSTR, text, length);
    text[length] = 0;
    strcpy(out, text);
    free(text);
    return 1;
}

// Copies the text being composed into `out`.
int ImeContext::GetCompositionString(char *out)
{
    *out = 0;
    LONG length = ImmGetCompositionStringA(context, GCS_COMPSTR, 0, 0);
    if (length == 0)
        return 0;
    char *text = (char *)operator new(length + 1);
    ImmGetCompositionStringA(context, GCS_COMPSTR, text, length);
    text[length] = 0;
    strcpy(out, text);
    free(text);
    return 1;
}

// Loads the candidate list into the context's buffer and returns its size.
DWORD ImeContext::LoadCandidates()
{
    if (buffer != 0)
    {
        free(buffer);
        buffer = 0;
    }
    DWORD size = ImmGetCandidateListA(context, 0, 0, 0);
    if (size == 0)
        return 0;
    buffer = (CANDIDATELIST *)operator new(size);
    ImmGetCandidateListA(context, 0, buffer, size);
    return buffer->dwCount;
}
#include "win_vars.h"

// Prepares text input with `text` as the starting text: maximum length 0x80 (DAT_10f66bbc), the text is copied into
// the input line and `text` becomes the buffer that receives the result. Clears the composition, result and
// candidate strings and the input counters, and marks input as active. Returns 0.
extern "C" int FUN_1015f310(char *text)
{
    DAT_10f66bbc = 0x80;
    strcpy(DAT_10f66df8, text);
    DAT_10f66ef8 = text;
    DAT_10f66bd0[0] = 0;
    DAT_10f66cd0[0] = 0;
    DAT_10f68008[0] = 0;
    DAT_10f66ab8[0] = 0;
    DAT_10f68108 = 0;
    DAT_10f669a0 = 0;
    DAT_10f66de0 = 0;
    DAT_10f66df0 = 0;
    DAT_10f66aa8 = 0;
    DAT_10f6811c = 0;
    DAT_10f66bc8 = 0;
    DAT_10f6810c = 0;
    DAT_10f66bc0 = 0;
    DAT_10f67000 = 1;
    DAT_10f66ddc = 0;
    DAT_10f66de4 = 0;
    DAT_10f66bc4 = 0;
    DAT_10f66dd4 = 0;
    for (int i = 0; i < 32; i++)
        DAT_10f67008[i].text[0] = 0;
    return 0;
}

// Ends text input (name entry): marks input as finished, restores the IME conversion mode saved when input
// started, closes the IME and copies the entered text into the caller's buffer.
extern "C" int FUN_1015fd30(void)
{
    DAT_10f67000 = 0;
    DAT_10f66df0 = 1;
    if (DAT_10f66bc8 != 0)
    {
        DAT_10f68124.SetConversion(&DAT_10f66dd8, &DAT_10f66bb8);
        DAT_10f66bc8 = 0;
    }
    DAT_10f68124.SetOpen(0);
    if (DAT_10f66ef8 != 0)
        strcpy(DAT_10f66ef8, DAT_10f66df8);
    return 0;
}
// 0x1015f3e0. IME callback: the composition string changed, copy it into DAT_10f66bd0. Returns 0.
extern "C" int FUN_1015f3e0()
{
    DAT_10f68124.GetCompositionString(DAT_10f66bd0);
    return 0;
}

// 0x1015f400. IME callback: a result string arrived. Appends it to the text entered so far (converted to full width
// when text input is allowed), cuts the text at the maximum length and clears the result. Returns 0.
extern "C" int FUN_1015f400()
{
    DAT_10f68124.GetResultString(DAT_10f669a8);
    if (DAT_10f669a8[0] != 0)
    {
        strcat(DAT_10f66df8, DAT_10f669a8);
        if (DAT_10203428 == 1)
            FUN_1015d520(DAT_10f66df8);
        DAT_10f66df8[DAT_10f66bbc] = 0;
        DAT_10f669a8[0] = 0;
        DAT_10f65698.compositionResult = 0;
    }
    return 0;
}

// 0x1015f490. IME callback: the cursor position inside the composition changed. Returns 0.
extern "C" int FUN_1015f490()
{
    DAT_10f66ddc = DAT_10f68124.M_1015d400();
    return 0;
}

// 0x1015f4b0. IME callback: the clause attributes changed. Reads the composition value twice. Returns 0.
extern "C" int FUN_1015f4b0()
{
    DAT_10f66de4 = DAT_10f68124.GetCompositionValue();
    DAT_10f66dec = DAT_10f68124.GetCompositionValue();
    return 0;
}

// Starts text input into `buffer` (name entry): resets the input state, attaches the IME to the game window and
// installs the four IME callbacks. FUN_1015fd30 ends it. Returns 1.
extern "C" int FUN_1015fda0(char *buffer)
{
    DAT_10f67000 = 0;
    DAT_10203428 = 1;
    DAT_10f68110 = 0;
    DAT_10f66998 = 0;
    DAT_10f68118 = buffer;
    DAT_10f68124.Attach(DAT_10f65698.window);
    DAT_10f65698.compositionEnded = 0;
    DAT_10f65698.compositionResult = 0;
    DAT_10f65698.onCompositionString = FUN_1015f3e0;
    DAT_10f65698.onCompositionResult = FUN_1015f400;
    DAT_10f65698.onCursorPos = FUN_1015f490;
    DAT_10f65698.onAttributes = FUN_1015f4b0;
    return 1;
}

// 0x1015d180. Writes the IME input mode as a short Japanese label into `out`, built from the conversion and
// sentence modes: "R" (romaji input), full or half width, kana or katakana, then the sentence-mode marks
// (renbunsetsu, fukugo, tanbunsetsu, jidou). Returns 1.
int ImeContext::GetModeText(char *out)
{
    DWORD conversion;
    DWORD sentence;
    out[0] = 0;
    GetConversion(&conversion, &sentence);
    if (conversion & IME_CMODE_ROMAN)
        strcat(out, "\x82\x71");                  // "R" (full width)
    if (conversion & IME_CMODE_FULLSHAPE)
        strcat(out, "\x91\x53");                  // "zen" (full width)
    else
        strcat(out, "\x94\xbc");                  // "han" (half width)
    if (conversion & IME_CMODE_KATAKANA)
        strcat(out, "\x83\x4a\x83\x69");          // "kana" in katakana
    else
        strcat(out, "\x82\xa9\x82\xc8");          // "kana" in hiragana
    if (sentence & IME_SMODE_PHRASEPREDICT)
        strcat(out, "\x98\x41");                  // "ren"
    if (sentence & IME_SMODE_PLAURALCLAUSE)
        strcat(out, "\x95\xa1");                  // "fuku"
    if (sentence & IME_SMODE_SINGLECONVERT)
        strcat(out, "\x92\x50");                  // "tan"
    if (sentence & IME_SMODE_AUTOMATIC)
        strcat(out, "\x8e\xa9");                  // "ji"
    return 1;
}
