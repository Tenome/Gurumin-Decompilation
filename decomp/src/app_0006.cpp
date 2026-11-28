// Game menu helpers. Up to three menu windows can be open at once; FindSlot(id) returns the window slot
// holding menu `id` (3 = not open). Per slot: m_0110 open flag, m_0140 cursor position, m_0188..m_01d0 layout.
// m_01dc[id] marks menus that remember their cursor, m_0434[id] holds the remembered position.

#include <stdlib.h>
#include <string.h>
#include "game.h"

// 0x10002030. Frees every "#Script" section loaded by LoadMapData and clears its slot.
void Game::FreeScripts()
{
    for (int i = 0; i < SCRIPT_LIMIT; i++)
    {
        if (scripts[i] != 0)
            free(scripts[i]);
        scripts[i] = 0;
    }
}

// Looks up a named script constant (case-insensitive prefix match over the m_95e14 table); -1 when unknown.
int Game::FindConstant(const char *name)
{
    for (int i = 0; i < m_a4614; i++)
    {
        if (_strnicmp(name, m_95e14[i].name, strlen(m_95e14[i].name)) == 0)
            return m_95e14[i].map;
    }
    return -1;
}

// Closes menu `id`: plays the close sound through the m_7660 callback and clears the slot. 0 when not open.
int Game::CloseWindow(int id)
{
    int slot = FindSlot(id);
    if (slot == 3)
        return 0;
    m_7660(2);
    m_0110[slot] = 0;
    return 1;
}

// 0x100084b0. Opens menu window `id` at (x, y) with size (width, height): in its current slot when it is already
// open, else in the first unused slot (0 when all three are in use). The window's state is reset, the cursor goes
// to the remembered position of menus that keep theirs, the item labels and the result pointer are cleared and the
// open sound plays through m_7660. Every window's fade then advances by 0.05 and the entry tables of all three
// are cleared (with their result pointers). Returns `id`.
int Game::OpenWindow(int id, int x, int y, int width, int height)
{
    int slot = FindSlot(id);
    if (slot == 3)
    {
        for (slot = 0; slot < 3; slot++)
        {
            if (m_008c[slot] == 0)
                break;
        }
        if (slot == 3)
            return 0;
        m_0140[slot] = 0;
        m_0098[slot] = 1;
    }
    m_068c[slot] = 0;
    m_00bc[slot] = y;
    m_00b0[slot] = x;
    m_00d4[slot] = height;
    m_0110[slot] = 1;
    m_01b8[slot] = 0;
    m_0134[slot] = 0;
    m_7654[slot] = 0;
    m_00a4[slot] = id;
    m_00c8[slot] = width;
    m_00ec[slot] = 0;
    m_0164[slot] = 0;
    m_0158[slot] = 0;
    m_0088 = slot;
    m_0194[slot] = 0;
    m_00f8[slot] = 0;
    m_011c[slot] = 0x80;
    m_0128[slot] = 0x18;
    m_008c[slot] = 1;
    m_0140[slot] = 0;
    if (m_01dc[id] != 0)
        m_0140[slot] = (unsigned char)m_0434[id];
    m_579c[slot].entries[MENU_RESULT] = 0;
    m_0104 = 10;
    for (int i = 0; i < 0x80; i++)
        m_069c[slot][i][0] = 0;
    m_7660(0);
    m_7668[slot] = 0.03f;
    for (int w = 0; w < 3; w++)
    {
        m_7668[w] += 0.05f;
        for (int j = 0; j < 0x82; j++)
        {
            if (m_579c[w].entries[j] != 0)
                m_579c[w].entries[j] = 0;
        }
    }
    return id;
}

// Sets menu `id`'s cursor position (and remembers it when that menu keeps its cursor).
int Game::SetWindowCursor(int id, int value)
{
    int slot = FindSlot(id);
    if (slot != 3)
    {
        m_0140[slot] = value;
        if (m_01dc[id] != 0)
            m_0434[id] = (char)m_0140[slot];
    }
    return 0;
}

// Sets four layout values of menu `id` (window rectangle).
int Game::SetWindowLayout(int id, int a, int b, int *c, int *d)
{
    int slot = FindSlot(id);
    if (slot != 3)
    {
        m_01ac[slot] = a;
        m_01a0[slot] = b;
        m_0194[slot] = c;
        m_0188[slot] = d;
    }
    return 0;
}

// Sets three more layout values of menu `id`.
int Game::SetWindowLayout2(int id, int a, int b, int *c)
{
    int slot = FindSlot(id);
    if (slot != 3)
    {
        m_01b8[slot] = c;
        m_01c4[slot] = a;
        m_01d0[slot] = b;
    }
    return 0;
}