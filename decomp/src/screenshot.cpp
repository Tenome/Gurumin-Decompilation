// Reading the back buffer for a screenshot (Screenshot_Capture 0x10137b10 saves the result).

#include "d3d9_min.h"

#include "render_vars.h"
#include "ui_vars.h"

// 0x1013e9d0 (Screenshot_ReadBackBuffer). Copies the back buffer into a new 24-bit BGR picture (width * height * 3
// bytes, allocated with new) and stores its size in *width and *height. The back buffer goes through a system-memory
// surface of its own format (R8G8B8, X8R8G8B8 or R5G6B5, the last widened to 8 bits per channel). Returns the
// picture, or 0 when a Direct3D call fails.
extern "C" unsigned char *FUN_1013e9d0(int *width, int *height)
{
    IDirect3DSurface9Like *copy;
    IDirect3DSurface9Like *backBuffer;
    LockedRect locked;
    if (DAT_10f1fcf0->GetBackBuffer(0, 0, 0, (void **)&backBuffer) != 0)
        return 0;
    long result;
    if (DAT_10f654a4 != 0)
    {
        if (DAT_10f65540 != 0)
            result = DAT_10f1fcf0->CreateOffscreenPlainSurface(DAT_10202b24, DAT_10202b28, 0x14 /* D3DFMT_R8G8B8 */,
                                                               2 /* D3DPOOL_SYSTEMMEM */, (void **)&copy, 0);
        else
            result = DAT_10f1fcf0->CreateOffscreenPlainSurface(DAT_10202b24, DAT_10202b28, 0x16 /* D3DFMT_X8R8G8B8 */,
                                                               2, (void **)&copy, 0);
    }
    else
        result = DAT_10f1fcf0->CreateOffscreenPlainSurface(DAT_10202b24, DAT_10202b28, 0x17 /* D3DFMT_R5G6B5 */, 2,
                                                           (void **)&copy, 0);
    if (result != 0)
    {
        backBuffer->Release();
        return 0;
    }
    if (DAT_10f1fcf0->GetRenderTargetData(backBuffer, copy) != 0)
    {
        backBuffer->Release();
        copy->Release();
        return 0;
    }
    unsigned char *pixels = new unsigned char[DAT_10202b24 * DAT_10202b28 * 3];
    copy->LockRect(&locked, 0, 0x10 /* D3DLOCK_READONLY */);
    unsigned char *bits = (unsigned char *)locked.bits;
    if (DAT_10f654a4 != 0)
    {
        if (DAT_10f65540 != 0)
        {
            for (int y = 0; y < DAT_10202b28; y++)
            {
                unsigned char *row = bits + y * locked.pitch;
                for (int x = 0; x < DAT_10202b24; x++)
                {
                    pixels[(y * DAT_10202b24 + x) * 3 + 2] = row[x * 3 + 2];
                    pixels[(y * DAT_10202b24 + x) * 3 + 1] = row[x * 3 + 1];
                    pixels[(y * DAT_10202b24 + x) * 3 + 0] = row[x * 3 + 0];
                }
            }
        }
        else
        {
            for (int y = 0; y < DAT_10202b28; y++)
            {
                unsigned char *row = bits + y * locked.pitch;
                for (int x = 0; x < DAT_10202b24; x++)
                {
                    pixels[(y * DAT_10202b24 + x) * 3 + 2] = row[x * 4 + 2];
                    pixels[(y * DAT_10202b24 + x) * 3 + 1] = row[x * 4 + 1];
                    pixels[(y * DAT_10202b24 + x) * 3 + 0] = row[x * 4 + 0];
                }
            }
        }
    }
    else
    {
        for (int y = 0; y < DAT_10202b28; y++)
        {
            unsigned short *row = (unsigned short *)(bits + y * locked.pitch);
            for (int x = 0; x < DAT_10202b24; x++)
            {
                pixels[(y * DAT_10202b24 + x) * 3 + 2] = (row[x] >> 8) & 0xf8;               // red: top 5 bits
                pixels[(y * DAT_10202b24 + x) * 3 + 1] = (unsigned char)(row[x] >> 5) << 2;  // green: 6 bits
                pixels[(y * DAT_10202b24 + x) * 3 + 0] = row[x] << 3;                        // blue: low 5 bits
            }
        }
    }
    copy->UnlockRect();
    *width = DAT_10202b24;
    *height = DAT_10202b28;
    copy->Release();
    backBuffer->Release();
    return pixels;
}
