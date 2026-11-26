// Startup check of off-screen rendering: a 256x256 texture is made a render target, a blue square is drawn into
// it and read back. Cards that cannot render to (or read back) a texture turn the effects that need it off.

#include <stdlib.h>
#include <string.h>

#include "texture_object.h"

#include "char_vars.h"
#include "render_vars.h"

extern "C" int FUN_10175c90(const char *name, int width, int height, const unsigned char *pixels, int alpha,
                            int flags);          // Texture_CreateFromPixels
extern "C" int FUN_10174c70(int slot, int force);  // Texture_Release
extern "C" void FUN_1013f070(int mode, int a, int b);   // sets a 2D/3D render state
extern "C" int FUN_101441b0(void);               // draws the 257x257 black background square
extern "C" int FUN_10144040(void);               // draws the 34x34 blue square in the corner
extern "C" int FUN_101439e0(void);               // Render_ResetTarget
extern "C" void FUN_1014b480(void);              // Render_ResetSwapTarget

// 0x10017d30 (RenderTarget_Probe). Creates the texture "crtcc" (256x256, black) and renders into it twice: first
// only the clear and the background square, read back with CaptureRgbFirst (when that does not give bright blue
// pixels, DAT_10203434/DAT_10203438 are cleared and DAT_102022b8 set), then the blue square, read back with
// CaptureRgbRows. Rendering to textures counts as working (DAT_10f654d0 = 1, return 1) only when the texture
// could be made the target again and the first 32 read-back pixels are bright blue (blue byte over 240).
// The texture is released and the normal render target restored in every case.
// A failed UpdateImage result (0) is returned as it is.
extern "C" int FUN_10017d30(void)
{
    unsigned char pixels[0xc00];
    DAT_10f654cc = 1;
    unsigned char *black = (unsigned char *)operator new(0xc00);
    DAT_10f654d4 = 0;
    memset(black, 0, 0xc00);
    int slot = FUN_10175c90("crtcc", 0x100, 0x100, black, 1, 0);
    free(black);
    if (slot < 0)
    {
        DAT_10f654cc = 0;
        DAT_10f654d0 = 0;
        return 0;
    }
    DAT_118d0ba8[slot].UpdateImage(0);
    FUN_101441b0();
    DAT_118d0ba8[slot].CaptureRgbFirst(pixels);
    int bright;
    for (int i = 0; i < 0x60; i += 3)
    {
        if (pixels[i + 2] <= 0xf0)
        {
            bright = 0;
            break;
        }
        bright = 1;
    }
    DAT_10f654d4 = bright;
    if (bright == 0)
    {
        DAT_10203434 = 0;
        DAT_10203438 = 0;
        DAT_102022b8 = 1;
    }
    FUN_1013f070(6, 0, 0);
    DAT_10f654cc = 1;
    int updated = DAT_118d0ba8[slot].UpdateImage(0);
    if (updated == 0)
    {
        DAT_10f654cc = 0;
        DAT_10f654d0 = 0;
        if (DAT_10f65484 != 0)
        {
            FUN_101439e0();
            FUN_10174c70(slot, 1);
            return updated;
        }
        FUN_1014b480();
        FUN_10174c70(slot, 1);
        return updated;
    }
    FUN_10144040();
    DAT_118d0ba8[slot].CaptureRgbRows(pixels);
    for (int j = 0; j < 0x60; j += 3)
    {
        if (pixels[j + 2] <= 0xf0)
        {
            FUN_1013f070(6, 0, 0);
            if (DAT_10f65484 != 0)
                FUN_101439e0();
            else
                FUN_1014b480();
            DAT_10f654cc = 0;
            DAT_10f654d0 = 0;
            FUN_10174c70(slot, 1);
            return 0;
        }
    }
    FUN_1013f070(6, 0, 0);
    if (DAT_10f65484 != 0)
        FUN_101439e0();
    else
        FUN_1014b480();
    DAT_10f654cc = 1;
    DAT_10f654d0 = 1;
    FUN_10174c70(slot, 1);
    return 1;
}
