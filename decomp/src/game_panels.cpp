// Game (DAT_104b7200) drawing helpers for menus and the HUD. Coordinates are 640x480 screen pixels;
// UV coordinates index the menu texture sheets loaded by Game::LoadMenuTextures.

#include "font_renderer.h"
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "texture_object.h"

extern "C" void FUN_1013f070(int, int, int);

// Draws a framed panel at (x, y, w, h): the frame from texMenu14 and the half-transparent fill from texMenu15.
int Game::DrawPanel(int x, int y, int w, int h, float z, float alpha)
{
    FUN_1013f070(2, 0, 0);
    DAT_118d0ba8[texMenu14].DrawFrame(x, y, x + w, y + h, z, alpha, 1, 0);
    DAT_118d0ba8[texMenu15].DrawFill(x, y, x + w, y + h, z, alpha * 0.5f, 1, 0);
    return 0;
}

// Same as DrawPanel with the frame from texMenu4 (the alternative panel style).
int Game::DrawPanelAlt(int x, int y, int w, int h, float z, float alpha)
{
    FUN_1013f070(2, 0, 0);
    DAT_118d0ba8[texMenu4].DrawFrameAlt(x, y, x + w, y + h, z, alpha, 1, 0);
    DAT_118d0ba8[texMenu15].DrawFillAlt(x, y, x + w, y + h, z, alpha * 0.5f, 1, 0);
    return 0;
}

// Stores the cursor position (m_7630/m_7634) and draws the menu cursor (a 32x32 cell of texMenu1) at the
// previous position (m_7638/m_763c), fading with `frame`.
int Game::DrawCursor(int x, int y, float fade, int frame)
{
    m_7630 = x;
    m_7634 = y;
    FUN_1013f070(0, 1, 0);
    DAT_118d0ba8[texMenu1].DrawTinted(m_7638 - 4, m_763c + 2, m_7638 + 0x1c, m_763c + 0x22, 1.0f / 256, 97.0f / 256,
                                      31.0f / 256, 127.0f / 256, 0, fade, (frame - 5) / 20.0f, 0);
    FUN_1013f070(0, 0, 0);
    return 1;
}

// Draws a speech balloon from texWaku01: a top cap (lit or unlit), `rows` middle rows of 32 pixels and a
// bottom cap, all 160 pixels wide. Not byte-identical yet (see open.tsv).
int Game::DrawBalloon(int x, int y, int lit, int rows, int a, int b, float fade)
{
    FUN_1013f070(2, 0, 0);
    int right = x + 0xa0;
    if (lit != 0)
        DAT_118d0ba8[texWaku01].DrawTinted(x, y, right, y + 0x20, 0.0f, 0.875f, 0.625f, 1.0f, 0, fade, 0, 0);
    else
        DAT_118d0ba8[texWaku01].DrawTinted(x, y, right, y + 0x20, 0.0f, 0.5f, 0.625f, 0.625f, 0, fade, 0, 0);
    for (int i = 0; i < rows; i++)
        DAT_118d0ba8[texWaku01].DrawTinted(x, y + 0x20 + i * 0x20, right, y + 0x40 + i * 0x20, 0.0f, 0.625f, 0.625f, 0.75f, 0, fade, 0, 0);
    y += rows * 0x20;
    DAT_118d0ba8[texWaku01].DrawTinted(x, y + 0x20, right, y + 0x3f, 0.0f, 0.75390625f, 0.625f, 0.875f, 0, fade, 0, 0);
    return 0;
}

// 0x10008a30. DrawNumber with a label in front: the text is two cells longer, the label (three digit cells wide, u
// 0.1875-0.4453 on row v 0.625-0.6875 of texMenu1) fills the first cells and the digits use row v 0.6875-0.75.
int Game::DrawLabelledNumber(int value, int x, int y, float z, float fade, float scale, float alpha)
{
    char text[256];
    sprintf(text, "%d", value);
    int length = strlen(text) + 2;
    int halfWidth = (int)((float)(length * 8) * scale * 0.7f);
    int digitWidth = (int)(scale * 11.2f);
    int halfHeight = (int)((float)digitWidth * 0.75f);
    DAT_118d0ba8[texMenu1].DrawTinted(x - halfWidth, y - halfHeight, x - halfWidth + digitWidth * 3, y + halfHeight,
                                      0.1875f, 0.625f, 0.4453125f, 0.6875f, z, fade, alpha, 0);
    for (int i = 2; i < length; i++)
    {
        int digit = text[i - 2] - '0';
        if (digit < 0)
            digit = 0;
        else if (digit > 9)
            digit = 9;
        int left = x - halfWidth + i * digitWidth;
        DAT_118d0ba8[texMenu1].DrawTinted(left, y - halfHeight, left + digitWidth, y + halfHeight,
                                          (float)((digit + 3) * 16) / 256.0f, 0.6875f,
                                          (float)((digit + 4) * 16) / 256.0f, 0.75f, z, fade, alpha, 0);
    }
    return 0;
}

// 0x10008c40. Draws `value` in decimal centred on (x, y) with the digits of texMenu1 (16-pixel cells, row
// v 0.8125-0.875, digit d at u (d + 3) / 16), `scale` setting the digit size (11.2 pixels wide at 1.0, 3/4 as high
// above and below y), faded by `alpha`.
int Game::DrawNumber(int value, int x, int y, float z, float fade, float scale, float alpha)
{
    char text[256];
    sprintf(text, "%d", value);
    int length = strlen(text);
    int halfWidth = (int)((float)(length * 8) * scale * 0.7f);
    int digitWidth = (int)(scale * 11.2f);
    int halfHeight = (int)((float)digitWidth * 0.75f);
    for (int i = 0; i < length; i++)
    {
        int digit = text[i] - '0';
        if (digit < 0)
            digit = 0;
        else if (digit > 9)
            digit = 9;
        int left = x - halfWidth + i * digitWidth;
        DAT_118d0ba8[texMenu1].DrawTinted(left, y - halfHeight, left + digitWidth, y + halfHeight,
                                          (float)((digit + 3) * 16) / 256.0f, 0.8125f,
                                          (float)((digit + 4) * 16) / 256.0f, 0.875f, z, fade, alpha, 0);
    }
    return 0;
}

// 0x10008df0. DrawNumber with the digit row above it (v 0.75-0.8125).
int Game::DrawNumberAlt(int value, int x, int y, float z, float fade, float scale, float alpha)
{
    char text[256];
    sprintf(text, "%d", value);
    int length = strlen(text);
    int halfWidth = (int)((float)(length * 8) * scale * 0.7f);
    int digitWidth = (int)(scale * 11.2f);
    int halfHeight = (int)((float)digitWidth * 0.75f);
    for (int i = 0; i < length; i++)
    {
        int digit = text[i] - '0';
        if (digit < 0)
            digit = 0;
        else if (digit > 9)
            digit = 9;
        int left = x - halfWidth + i * digitWidth;
        DAT_118d0ba8[texMenu1].DrawTinted(left, y - halfHeight, left + digitWidth, y + halfHeight,
                                          (float)((digit + 3) * 16) / 256.0f, 0.75f,
                                          (float)((digit + 4) * 16) / 256.0f, 0.8125f, z, fade, alpha, 0);
    }
    return 0;
}

// Draws ASCII `text` with the 8x16 debug font in texMenu1 (32 glyphs per row starting at the space).
int Game::DrawDebugText(const char *text, int x, int y)
{
    int length = strlen(text);
    for (int i = 0; i < length; i++)
    {
        int glyph = text[i] - 0x20;
        if (glyph > 0)
        {
            int column = (glyph & 0x1f) * 8;
            int row = (glyph >> 5) * 16;
            float v = row * (1.0f / 256);
            DAT_118d0ba8[texMenu1].DrawCell(x, y, x + 8, y + 0x10, column * (1.0f / 256), v, column * (1.0f / 256) + 0.03125f, v + 0.0625f, 0, 1.0f, 0);
        }
        x += 8;
    }
    return 1;
}

#include "vec4.h"

Vec4 Normalize3(const Vec4 &v);      // 0x101b1180: v scaled to unit length (x/y/z)

// 0x10009050. Moves the menu cursor towards its target: 160 pixels a frame when more than 256 away, 80 when more
// than 128, 20 when more than 20, and onto the target otherwise. Each speed is a literal scale of the step, so the
// float arguments are compiler temporaries that share the slot of the dead distance. Returns 1.
// Not byte-identical yet: the shared `step.Copy(step * speed)` tail loads its `this` before pushing the product.
int Game::MoveCursor()
{
    Vec4 step;
    step.x = (float)(m_7630 - m_7638);
    step.y = (float)(m_7634 - m_763c);
    step.z = 0;
    float distance = step.Length();
    if (distance > 256.0f)
    {
        step.Copy(Normalize3(step));
        step.Copy(step * 160.0f);
    }
    else if (distance > 128.0f)
    {
        step.Copy(Normalize3(step));
        step.Copy(step * 80.0f);
    }
    else if (distance > 20.0f)
    {
        step.Copy(Normalize3(step));
        step.Copy(step * 20.0f);
    }
    else
    {
        m_7638 = m_7630;
        m_763c = m_7634;
        return 1;
    }
    m_7638 += (int)step.x;
    m_763c += (int)step.y;
    return 1;
}


// 0x100092c0. Draws the marks of the three menu windows, last window first: for list windows an up mark when the
// list is scrolled, a down mark when an item follows the visible ones, and the cursor on the chosen item of the
// active window; for grid windows (m_0158) only the cursor at its column and row. Returns 0.
int Game::DrawWindowMarks(int frame)
{
    for (int w = 2; w >= 0; w--)
    {
        if (m_00ec[w] == 0)
            continue;
        if (m_0158[w] != 0)
        {
            if (m_008c[w] != 0 && m_0088 == w && m_0110[w] != 0)
            {
                FUN_1013f070(0, 1, 0);
                DrawCursor(m_0170[w] * m_0164[w] - 0x10 + m_00b0[w], m_0128[w] * m_017c[w] + m_00bc[w],
                           (float)m_0098[w] / 32.0f, frame);
                FUN_1013f070(0, 0, 0);
            }
        }
        else if (m_008c[w] != 0)
        {
            if (m_0134[w] > 0)
            {
                if (frame > 5)
                    textRenderer->DrawText(m_00b0[w] + 0x20, m_00bc[w], MARK_UP_OUTLINE, 0xffefcf, 0, 1.0f, 0.9f);
                else
                    textRenderer->DrawText(m_00b0[w] + 0x20, m_00bc[w], MARK_UP, 0xffefcf, 0, 1.0f, 0.9f);
            }
            if (m_011c[w] < 0x80 && m_069c[w][m_0134[w] + m_011c[w] + 1][0] != 0)
            {
                if (frame > 5)
                    textRenderer->DrawText(m_00b0[w] + 0x20, m_0128[w] * m_011c[w] + 0x2c + m_00bc[w], MARK_DOWN_OUTLINE,
                                     0xffefcf, 0, 1.0f, 0.9f);
                else
                    textRenderer->DrawText(m_00b0[w] + 0x20, m_0128[w] * m_011c[w] + 0x2c + m_00bc[w], MARK_DOWN, 0xffefcf,
                                     0, 1.0f, 0.9f);
            }
            if (m_0088 == w && m_0110[w] != 0)
            {
                FUN_1013f070(0, 1, 0);
                DrawCursor(m_00b0[w], (m_0140[w] - m_0134[w]) * m_0128[w] + 0xc + m_00bc[w], (float)m_0098[w] / 32.0f,
                           frame);
                FUN_1013f070(0, 0, 0);
            }
        }
    }
    return 0;
}
