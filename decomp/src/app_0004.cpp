// Game menu state (see app_0006.cpp for the window slots) and helpers.

#include "game.h"

// 1 while any menu window is open or still animating (more than 16 frames into its open animation).
int Game::IsAnyWindowBusy()
{
    for (int i = 0; i < 3; i++)
    {
        if (m_0110[i] != 0)
            return 1;
        if (m_008c[i] != 0 && m_0098[i] > 16)
            return 1;
    }
    return 0;
}

// Closes all three menu windows and clears their item tables.
void Game::CloseAllWindows()
{
    for (int i = 0; i < 3; i++)
    {
        m_008c[i] = 0;
        m_0110[i] = 0;
        m_00a4[i] = 0;
        for (int j = 0; j < 0x82; j++)
            m_5db4[i][j] = 0;
    }
}

// Sets menu `id`'s callback value m_7654. 0 when not open.
int Game::SetWindowCallbackValue(int id, int (*callback)(int cursor, char *caption))
{
    int slot = FindSlot(id);
    if (slot == 3)
        return 0;
    m_7654[slot] = callback;
    return 1;
}

// Marks menu `id` as confirmed (m_00ec). 0 when not open.
int Game::ConfirmWindow(int id)
{
    int slot = FindSlot(id);
    if (slot == 3)
        return 0;
    m_00ec[slot] = 1;
    return 1;
}

// Sets menu `id`'s m_0128 value (the first visible line offset).
int Game::SetWindowScroll(int id, int value)
{
    int slot = FindSlot(id);
    if (slot != 3)
        m_0128[slot] = value;
    return 0;
}

// Sets menu `id`'s last item index (count - 1).
int Game::SetWindowLastItem(int id, int count)
{
    int slot = FindSlot(id);
    if (slot != 3)
        m_011c[slot] = count - 1;
    return 0;
}

#include "app_0004.h"

typedef void (Callee::*CalleeMethod)();

// The compiler's "vector constructor iterator": calls `method` on each of `count` objects spaced `stride`
// bytes apart (used for arrays of objects with constructors).
extern "C" void __stdcall FUN_10004650(char *object, int stride, int count, CalleeMethod method)
{
    while (--count >= 0)
    {
        (((Callee *)object)->*method)();
        object += stride;
    }
}

// Toggles a 30-frame latch when in state 10: 0 -> 30, 30 -> 0.
int Latch::M_1000b9c0()
{
    if (type == 10)
    {
        if (frame == 0)
            targetFrame = 30;
        else if (frame == 30)
            targetFrame = 0;
    }
    return 1;
}


// Sets the pointer that receives menu `id`'s chosen result and clears it. 0 when the menu is not open.
int Game::SetWindowResult(int id, int *result)
{
    int slot = FindSlot(id);
    if (slot == 3)
        return 0;
    m_579c[slot].entries[MENU_RESULT] = result;
    if (result != 0)
        *result = 0;
    return 1;
}
