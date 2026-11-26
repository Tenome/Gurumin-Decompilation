// Texture objects: binding to the Direct3D texture stages and releasing.
//
// A TextureObject (0x38 bytes, table of 1600 at 0x118d0ba8) owns a Direct3D texture. Animated or streamed
// textures keep their frames in the buffer pool (buffer_pool.cpp); for those `poolSlot` != 0 and the texture to
// bind is fetched from the pool when the object is bound.
#include <string.h>
#include "d3d9_min.h"

#include "texture_object.h"

#include "render_vars.h"

extern "C" void *FUN_10148590(int poolIndex, IUnknownLike *texture, int width, int height, int slot);
extern "C" int FUN_10148520(int poolIndex, int width, int height);
extern "C" int FUN_101484b0(int poolIndex, int width, int height);  // drops the pooled copy (buffer_pool.cpp)
#include "engine_memory.h"

// Binds this texture to stage 0 unless it is bound there already. Returns 1.
int TextureObject::BindStage0()
{
    if (DAT_10f6550c != this)
    {
        DAT_10f141d4++;
        if (poolSlot != 0)
        {
            void *pooled = FUN_10148590(poolIndex, texture, width, height, poolSlot);
            DAT_10f1fcf0->SetTexture(0, pooled);
            DAT_10f6550c = this;
            return 1;
        }
        DAT_10f1fcf0->SetTexture(0, texture);
        DAT_10f6550c = this;
    }
    return 1;
}

// Binds this texture to stage 1 (multi-texturing) unless it is bound there already. Returns 1.
int TextureObject::BindStage1()
{
    if (DAT_10f65510 != this)
    {
        DAT_10f141d4++;
        if (poolSlot != 0)
        {
            void *pooled = FUN_10148590(poolIndex, texture, width, height, poolSlot);
            DAT_10f1fcf0->SetTexture(1, pooled);
            DAT_10f65510 = this;
            return 1;
        }
        DAT_10f1fcf0->SetTexture(1, texture);
        DAT_10f65510 = this;
    }
    return 1;
}

// Releases the Direct3D texture(s) (returning pooled frames to the buffer pool first) and forgets any stage
// binding that still points at this object. Returns 0.
int TextureObject::Release()
{
    if (texture != 0)
    {
        if (poolSlot != 0)
            FUN_10148520(poolIndex, width, height);
        texture->Release();
    }
    if (surface != 0)
        surface->Release();
    if (DAT_10f6550c == this)
        DAT_10f6550c = (TextureObject *)1;
    if (DAT_10f65510 == this)
        DAT_10f65510 = (TextureObject *)1;
    width = 0;
    height = 0;
    poolSlot = 0;
    texture = 0;
    surface = 0;
    return 0;
}

// Copies 16-bit pixels into the texture: rows y..rows-1 of `rowPixels` pixels each from `bits` (`pitch` bytes per
// source row; `x` is not used). Drops the pooled copy and forgets stage bindings of this texture so the new
// pixels are used. Returns 0 when the texture cannot be locked, else 1.
int TextureObject::Write16(int x, int y, int rowPixels, int rows, int pitch, const void *bits)
{
    LockedRect locked;
    if (texture == 0)
        return 0;
    if (texture->LockRect(0, &locked, 0, 0x800) != 0)
        return 0;
    char *dst = (char *)locked.bits;
    const char *src = (const char *)bits;
    for (int row = y; row < rows; row++)
    {
        DAT_11b07530(dst, src, rowPixels * 2);
        dst += locked.pitch;
        src += pitch;
    }
    texture->UnlockRect(0);
    FUN_101484b0(poolIndex, width, height);
    if (DAT_10f6550c == this)
        DAT_10f6550c = (TextureObject *)1;
    if (DAT_10f65510 == this)
        DAT_10f65510 = (TextureObject *)1;
    return 1;
}

// 0x1014f120. Writes 8-bit palette indexes into the 16-bit texture: the rectangle left..right-1, top..bottom-1 of
// the texture gets palette[index] for each source byte (`bits` is the rectangle's first row, `pitch` bytes per
// row). Like Write16, it drops the pooled copy and forgets the stage bindings afterwards. Returns 0 when the
// texture cannot be locked, else 1.
int TextureObject::WritePaletted(int left, int top, int right, int bottom, int pitch, const unsigned char *bits,
                                 const unsigned short *palette)
{
    LockedRect locked;
    if (texture == 0)
        return 0;
    if (texture->LockRect(0, &locked, 0, 0x800) != 0)
        return 0;
    char *dst = (char *)locked.bits;
    const unsigned char *src = bits;
    for (int y = top; y < bottom; y++)
    {
        for (int x = left; x < right; x++)
            ((unsigned short *)dst)[x] = palette[src[x - left]];
        dst += locked.pitch;
        src += pitch;
    }
    texture->UnlockRect(0);
    FUN_101484b0(poolIndex, width, height);
    if (DAT_10f6550c == this)
        DAT_10f6550c = (TextureObject *)1;
    if (DAT_10f65510 == this)
        DAT_10f65510 = (TextureObject *)1;
    return 1;
}

// 0x1014ef20. Writes 32-bit pixels (B, G, R, unused bytes) into the 16-bit (R5G6B5) texture, rows top..bottom-1,
// two pixels per 32-bit store: the row of `pitch` source bytes becomes pitch / 8 pairs (`left` and `right` are not
// used). Like Write16, it drops the pooled copy and forgets the stage bindings afterwards. Returns 0 when the
// texture cannot be locked, else 1.
int TextureObject::WriteRgb32(int left, int top, int right, int bottom, int pitch, const unsigned char *bits)
{
    LockedRect locked;
    if (texture == 0)
        return 0;
    if (texture->LockRect(0, &locked, 0, 0x800) != 0)
        return 0;
    const unsigned char *src = bits;
    int pairs = pitch / 8;
    char *dst = (char *)locked.bits;
    for (int y = top; y < bottom; y++)
    {
        for (int i = 0; i < pairs; i++)
        {
            // second pixel in the high half, first pixel in the low half
            *(unsigned int *)dst = ((((((src[6] & 0xf8) << 5 | (src[5] & 0xfc)) << 6 | (src[4] & 0xf8)) << 5 |
                                     (src[2] & 0xf8)) << 5 | (src[1] & 0xfc)) << 3) | (src[0] >> 3);
            dst += 4;
            src += 8;
        }
        dst += locked.pitch - pairs * 4;
        src += pitch - pairs * 8;
    }
    texture->UnlockRect(0);
    FUN_101484b0(poolIndex, width, height);
    if (DAT_10f6550c == this)
        DAT_10f6550c = (TextureObject *)1;
    if (DAT_10f65510 == this)
        DAT_10f65510 = (TextureObject *)1;
    return 1;
}

// 0x10145060. Copies this 256x256 render target (R5G6B5) into `out` as 24-bit pixels in B, G, R byte order: the
// thumbnail Save_Write stores in the save file. The surface is read back through a system-memory copy. Returns 1,
// or 0 when the read-back fails.
int TextureObject::CaptureRgb24(unsigned char *out)
{
    IDirect3DSurface9Like *copy;
    DAT_10f1fcf0->CreateOffscreenPlainSurface(256, 256, 0x17 /* D3DFMT_R5G6B5 */, 2 /* D3DPOOL_SYSTEMMEM */,
                                              (void **)&copy, 0);
    if (DAT_10f1fcf0->GetRenderTargetData(surface, copy) != 0)
    {
        copy->Release();
        return 0;
    }
    LockedRect locked;
    copy->LockRect(&locked, 0, 0x10 /* D3DLOCK_READONLY */);
    const unsigned short *pixels = (const unsigned short *)locked.bits;
    for (int i = 0; i < 256 * 256; i++)
    {
        out[i * 3 + 2] = (pixels[i] >> 8) & 0xf8;               // red: top 5 bits
        out[i * 3 + 1] = (unsigned char)(pixels[i] >> 5) << 2;  // green: middle 6 bits
        out[i * 3 + 0] = pixels[i] << 3;                        // blue: low 5 bits
    }
    copy->UnlockRect();
    copy->Release();
    return 1;
}

// 0x10145120. Like CaptureRgb24, but reads back only the first 0x400 pixels (four rows of 256) and writes them in
// R, G, B byte order. When an R5G6B5 copy cannot be created an X1R5G5B5 one is tried (its pixels are still
// converted as R5G6B5). Called by FUN_10017d30. Returns 1, or 0 when the copy or the read-back fails.
int TextureObject::CaptureRgbRows(unsigned char *out)
{
    IDirect3DSurface9Like *copy;
    if (DAT_10f1fcf0->CreateOffscreenPlainSurface(256, 256, 0x17 /* D3DFMT_R5G6B5 */, 2 /* D3DPOOL_SYSTEMMEM */,
                                                  (void **)&copy, 0) != 0 &&
        DAT_10f1fcf0->CreateOffscreenPlainSurface(256, 256, 0x18 /* D3DFMT_X1R5G5B5 */, 2, (void **)&copy, 0) != 0)
        return 0;
    if (DAT_10f1fcf0->GetRenderTargetData(surface, copy) != 0)
    {
        copy->Release();
        return 0;
    }
    LockedRect locked;
    copy->LockRect(&locked, 0, 0x10 /* D3DLOCK_READONLY */);
    const unsigned short *pixels = (const unsigned short *)locked.bits;
    for (int i = 0; i < 0x400; i++)
    {
        out[i * 3 + 0] = (pixels[i] >> 8) & 0xf8;               // red: top 5 bits
        out[i * 3 + 1] = (unsigned char)(pixels[i] >> 5) << 2;  // green: middle 6 bits
        out[i * 3 + 2] = pixels[i] << 3;                        // blue: low 5 bits
    }
    copy->UnlockRect();
    copy->Release();
    return 1;
}

extern "C" void FUN_1013ef30(int state, int value);                   // Render_SetRenderState (cached)
extern "C" void FUN_1013ef90(int stage, int type, int value);         // Render_SetTextureStageState (cached)
extern "C" void FUN_10143b80(void);                                   // Render_UnbindTexture1

// 0x1014bb70. Draws `triangles` triangles from `vertices` (FVF 0x1c4: position, diffuse colour, one texture
// coordinate; 0x20 bytes per vertex) with this texture on stage 0: texture times vertex colour for both colour and
// alpha, stages 1 to 3 off. Returns 0 when there is nothing to draw, else 1. Used by the triangle batches
// (tri_batch.cpp).
int TextureObject::DrawTriangles(const void *vertices, int triangles)
{
    if (triangles < 1)
        return 0;
    BindStage0();
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    FUN_10143b80();
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(4 /* D3DPT_TRIANGLELIST */, triangles, vertices, 0x20);
    return 1;
}


#include "screen_vertex.h"

// 0x1014ba20. Draws a triangle strip of `count` vertices given in 640x480 screen coordinates: the positions are
// scaled to the back-buffer size in place, then drawn like DrawTriangles. Returns 0 when there are fewer than 3
// vertices, else 1.
int TextureObject::DrawStrip(ScreenVertex *vertices, int count)
{
    if (count < 3)
        return 0;
    BindStage0();
    FUN_10143b80();
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    FUN_10143b80();
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    for (int i = 0; i < count; i++)
    {
        vertices[i].x = scaleX * vertices[i].x;
        vertices[i].y = scaleY * vertices[i].y;
    }
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, count - 2, vertices, 0x20);
    return 1;
}

#include "texture_bind.h"

// 0x10145210. Like CaptureRgbRows for only the first 32 pixels. When the render target cannot be read back, the
// 256x256 corner of the back buffer is stretch-copied into a scratch surface instead. Returns 1, or 0 on failure.
int TextureObject::CaptureRgbFirst(unsigned char *out)
{
    IDirect3DSurface9Like *copy;
    Rect rect;
    if (DAT_10f1fcf0->CreateOffscreenPlainSurface(256, 256, 0x17 /* D3DFMT_R5G6B5 */, 2 /* D3DPOOL_SYSTEMMEM */,
                                                  (void **)&copy, 0) != 0 &&
        DAT_10f1fcf0->CreateOffscreenPlainSurface(256, 256, 0x18 /* D3DFMT_X1R5G5B5 */, 2, (void **)&copy, 0) != 0)
        return 0;
    rect.left = 0;
    rect.top = 0;
    rect.right = 256;
    rect.bottom = 256;
    if (DAT_10f1fcf0->GetRenderTargetData(surface, copy) != 0)
    {
        copy->Release();
        if (DAT_10f1fcf0->CreateOffscreenPlainSurface(256, 256, 0x18, 3 /* D3DPOOL_SCRATCH */, (void **)&copy, 0) != 0)
            return 0;
        if (DAT_10f1fcf0->StretchRect(DAT_10f141dc, &rect, copy, 0, 0) != 0)
        {
            copy->Release();
            return 0;
        }
    }
    LockedRect locked;
    copy->LockRect(&locked, 0, 0x10 /* D3DLOCK_READONLY */);
    const unsigned short *pixels = (const unsigned short *)locked.bits;
    for (int i = 0; i < 0x20; i++)
    {
        out[i * 3 + 0] = (pixels[i] >> 8) & 0xf8;               // red: top 5 bits
        out[i * 3 + 1] = (unsigned char)(pixels[i] >> 5) << 2;  // green: middle 6 bits
        out[i * 3 + 2] = pixels[i] << 3;                        // blue: low 5 bits
    }
    copy->UnlockRect();
    copy->Release();
    return 1;
}

extern "C" int FUN_10148410(void);   // BufferPool_Alloc

// 0x1014f1f0 (TextureObject::Upload). (Re)creates this texture at width x height and forgets it as a bound texture.
// `alpha` chooses the kind: 0 = a system-memory R5G6B5 texture filled from the 24-bit B, G, R `pixels`; 1 or 2 = an
// empty R5G6B5 or A8R8G8B8 render target with its surface; any other value points to a second 24-bit image whose
// first byte per pixel is the alpha of a system-memory A4R4G4B4 texture filled from `pixels`. System-memory textures
// get a buffer pool index (they are copied into the pool when bound). Returns 0 when Direct3D fails, else 1 (also
// without a device).
int TextureObject::Upload(int width, int height, unsigned char *pixels, const unsigned char *alpha)
{
    if (DAT_10f6550c == this)
        DAT_10f6550c = (TextureObject *)1;
    if (DAT_10f65510 == this)
        DAT_10f65510 = (TextureObject *)1;
    if (DAT_10f1fcf0 == 0)
        return 1;
    poolSlot = 0;
    this->width = width;
    this->height = height;
    target = 0;
    if (alpha == 0)
    {
        if (DAT_10f1fcf0->CreateTexture(width, height, 1, 0, 0x17 /* D3DFMT_R5G6B5 */, 2 /* D3DPOOL_SYSTEMMEM */,
                                        (IUnknownLike **)&texture, 0) != 0)
            return 0;
        pad_0018 = 0;
        poolSlot = 1;
        surface = 0;
        int levels = 1;
        for (int level = 0; level < levels; level++)
        {
            LockedRect locked;
            if (texture->LockRect(level, &locked, 0, 0x800 /* D3DLOCK_NOSYSLOCK */) != 0)
                return 0;
            int offset = 0;
            for (int y = 0; y < height; y++)
            {
                unsigned short *row = (unsigned short *)((char *)locked.bits + locked.pitch * y);
                for (int x = 0; x < width; x++)
                {
                    unsigned short r = pixels[offset + x * 3 + 2];
                    unsigned short g = pixels[offset + x * 3 + 1];
                    row[x] = (unsigned short)((((r & 0xfff8) << 5 | g) & 0xfffc) << 3 | pixels[offset + x * 3] >> 3);
                }
                offset += width * 3;
            }
            texture->UnlockRect(level);
            if (level < levels - 1)
            {
                HalveRgb24(pixels, width, height);
                width /= 2;
                height /= 2;
            }
        }
    }
    else if (alpha == (const unsigned char *)2)
    {
        target = 2;
        if (DAT_10f1fcf0->CreateTexture(width, height, 1, 1 /* D3DUSAGE_RENDERTARGET */, 0x15 /* D3DFMT_A8R8G8B8 */,
                                        0, (IUnknownLike **)&texture, 0) != 0)
            return 0;
        return texture->GetSurfaceLevel(0, &surface) == 0;
    }
    else if (alpha == (const unsigned char *)1)
    {
        target = 1;
        if (DAT_10f1fcf0->CreateTexture(width, height, 1, 1 /* D3DUSAGE_RENDERTARGET */, 0x17 /* D3DFMT_R5G6B5 */,
                                        0, (IUnknownLike **)&texture, 0) != 0)
            return 0;
        return texture->GetSurfaceLevel(0, &surface) == 0;
    }
    else
    {
        if (DAT_10f1fcf0->CreateTexture(width, height, 1, 0, 0x1a /* D3DFMT_A4R4G4B4 */, 2 /* D3DPOOL_SYSTEMMEM */,
                                        (IUnknownLike **)&texture, 0) != 0)
            return 0;
        pad_0018 = 0;
        poolSlot = 2;
        LockedRect locked;
        texture->LockRect(0, &locked, 0, 0x800 /* D3DLOCK_NOSYSLOCK */);
        surface = 0;
        int offset = 0;
        for (int y = 0; y < height; y++)
        {
            unsigned short *row = (unsigned short *)((char *)locked.bits + locked.pitch * y);
            for (int x = 0; x < width; x++)
            {
                unsigned short r = pixels[offset + x * 3 + 2];
                unsigned short g = pixels[offset + x * 3 + 1];
                row[x] = (unsigned short)((((r & 0xfff0) << 4 | g) & 0xfff0) |
                                          (unsigned short)(alpha[offset + x * 3] >> 4) << 12 |
                                          pixels[offset + x * 3] >> 4);
            }
            offset += width * 3;
        }
        texture->UnlockRect(0);
    }
    poolIndex = FUN_10148410();
    return 1;
}


// 0x10143870. Makes this texture's surface the render target (with the off-screen depth buffer) for drawing into
// it: the textures of stages 0..3 are unbound and the viewport covers offset..width, offset..height (clamped to the
// back-buffer size). Unless an off-screen target is already set up, the state to return to is saved. Returns 1 when
// the target and viewport were set, else 0 (also when off-screen targets are not supported).
int TextureObject::BeginRenderTo(int width, int height, int offset)
{
    DAT_10f6548c = 0;
    DAT_10f1fcf0->SetTexture(0, 0);
    DAT_10f6550c = (TextureObject *)1;
    DAT_10f1fcf0->SetTexture(1, 0);
    DAT_10f65510 = (TextureObject *)1;
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    if (DAT_10f654cc == 0)
        return 0;
    else
    {
        long result = DAT_10f1fcf0->SetRenderTarget(0, surface);
        DAT_10f1fcf0->SetDepthStencilSurface(DAT_10f08d38);
        if (result != 0)
            return 0;
        if (DAT_10202b24 <= width)
            width = DAT_10202b24;
        if (DAT_10202b28 <= height)
            height = DAT_10202b28;
        memset(&DAT_10f1fd48, 0, sizeof(DAT_10f1fd48));
        DAT_10f1fd48.Width = width - offset;
        DAT_10f1fd48.X = offset;
        DAT_10f1fd48.Y = offset;
        DAT_10f1fd48.Height = height - offset;
        DAT_10f1fd48.MinZ = 0;
        DAT_10f1fd48.MaxZ = 1.0f;
        DAT_10202b34 = width;
        DAT_10202b38 = height;
        if (DAT_10f65478 == 0)
        {
            DAT_10f60d30 = surface;
            DAT_10f0d540 = DAT_10f08d38;
            DAT_10f0ece0 = DAT_10f1fd30;
            DAT_10f65470 = width;
            DAT_10f65474 = height;
        }
        return DAT_10f1fcf0->SetViewport(&DAT_10f1fd48) == 0;
    }
}

// 0x10144ee0. Halves a 24-bit image in place: every 2x2 block of `pixels` (width x height) becomes the average of
// its four pixels, per channel. The result (width/2 x height/2) is built in a temporary buffer and copied back.
// Returns 0 when there is no image, else 1. Does not use `this`.
int TextureObject::HalveRgb24(unsigned char *pixels, int width, int height)
{
    if (pixels == 0)
        return 0;
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    unsigned char *half = (unsigned char *)DAT_11b0b938(halfHeight * halfWidth * 3, "a");
    int pitch = width * 3;
    for (int y = 0; y < halfHeight; y++)
    {
        for (int x = 0; x < halfWidth * 3; x += 3)
        {
            const unsigned char *top = pixels + y * 2 * pitch + x * 2;
            const unsigned char *bottom = pixels + (y * 2 + 1) * pitch + x * 2;
            unsigned char *out = half + y * halfWidth * 3 + x;
            out[0] = (top[3] + bottom[3] + bottom[0] + top[0]) / 4;
            out[1] = (top[4] + top[1] + bottom[4] + bottom[1]) / 4;
            out[2] = (top[5] + top[2] + bottom[5] + bottom[2]) / 4;
        }
    }
    DAT_11b07530(pixels, half, halfHeight * halfWidth * 3);
    DAT_11b1ba40(half);
    return 1;
}

extern "C" void FUN_10143b50(void);                                   // Render_UnbindTexture0


// 0x1014b750 (TextureObject::DrawScreenCopy). Draws this whole texture into the rectangle left..right, top..bottom
// (640x480 screen coordinates, scaled to the back buffer) as a two-triangle strip, with counter-clockwise culling.
// When the texture is wider or taller than the back buffer, only the part the size of the back buffer is used (a
// copy of the screen in a larger texture). The texture coordinates start at 0.001. White at `alpha` (0..1), texture
// times vertex colour; stage 1 off. Returns 1.
int TextureObject::DrawScreenCopy(int left, int top, int right, int bottom, float z, float alpha)
{
    PanelVertex vertices[4];
    BindStage0();
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    float uMax = 1.0f;
    float vMax = 1.0f;
    if (DAT_10202b24 < width)
        uMax = (float)DAT_10202b24 / (float)width;
    if (DAT_10202b28 < height)
        vMax = (float)DAT_10202b28 / (float)height;
    vertices[0].x = (float)left * scaleX;
    vertices[0].y = (float)top * scaleY;
    vertices[1].x = (float)right * scaleX;
    vertices[1].y = (float)top * scaleY;
    vertices[2].x = (float)left * scaleX;
    vertices[2].y = (float)bottom * scaleY;
    vertices[3].x = (float)right * scaleX;
    vertices[3].y = (float)bottom * scaleY;
    vertices[0].u = 0.001f;
    vertices[0].v = 0.001f;
    vertices[1].u = uMax;
    vertices[1].v = 0.001f;
    vertices[2].u = 0.001f;
    vertices[2].v = vMax;
    vertices[3].u = uMax;
    vertices[3].v = vMax;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = a << 24 | 0xffffff;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular = 0xff000000;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = z;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x16, 3);                  // D3DRS_CULLMODE = D3DCULL_CCW
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    FUN_1013ef90(1, 1, 1);                  // stage 1 off
    FUN_1013ef90(1, 4, 1);
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x20);
    return 1;
}

#define MIN(a, b) ((a) > (b) ? (b) : (a))


// 0x10144320 (TextureObject::DrawThroughShader). Binds this texture to stage 0, `second` to stage 1 and `third` to
// stage 2 and draws the whole back buffer as one quad through the crossfade pixel shader. The four texture
// coordinate pairs all cover the part of the square targets the size of the screen (half a texel in from each edge).
// The shader and stages 1 to 3 are cleared afterwards. Returns 0.
int TextureObject::DrawThroughShader(TextureObject *second, TextureObject *third)
{
    ShaderVertex vertices[4];
    DAT_10f1fcf0->SetTexture(0, texture);
    DAT_10f1fcf0->SetTexture(1, second->texture);
    DAT_10f1fcf0->SetTexture(2, third->texture);
    float width = (float)DAT_10202b34;
    float height = (float)DAT_10202b38;
    vertices[0].x = 0.0f;
    vertices[0].y = 0.0f;
    vertices[1].x = width;
    vertices[1].y = 0.0f;
    vertices[2].x = 0.0f;
    vertices[2].y = height;
    vertices[3].x = width;
    vertices[3].y = height;
    vertices[0].u3 = vertices[0].u2 = vertices[0].u1 = vertices[0].u0 =
        0.5f / DAT_10202b18;
    vertices[0].v3 = vertices[0].v2 = vertices[0].v1 = vertices[0].v0 =
        0.5f / DAT_10202b18;
    vertices[1].u3 = vertices[1].u2 = vertices[1].u1 = vertices[1].u0 =
        (MIN(DAT_10202b18, width) - 0.5f) / DAT_10202b18;
    vertices[1].v3 = vertices[1].v2 = vertices[1].v1 = vertices[1].v0 =
        0.5f / DAT_10202b18;
    vertices[2].u3 = vertices[2].u2 = vertices[2].u1 = vertices[2].u0 =
        0.5f / DAT_10202b18;
    vertices[2].v3 = vertices[2].v2 = vertices[2].v1 = vertices[2].v0 =
        (MIN(DAT_10202b18, height) - 0.5f) / DAT_10202b18;
    vertices[3].u3 = vertices[3].u2 = vertices[3].u1 = vertices[3].u0 =
        (MIN(DAT_10202b18, width) - 0.5f) / DAT_10202b18;
    vertices[3].v3 = vertices[3].v2 = vertices[3].v1 = vertices[3].v0 =
        (MIN(DAT_10202b18, height) - 0.5f) / DAT_10202b18;
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = 0xffffffff;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular = 0xff000000;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = 1.0f;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x4c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f1fcf0->SetPixelShader(DAT_10f1fd0c);
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x38);
    DAT_10f1fcf0->SetPixelShader(0);
    FUN_10143b80();
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    return 0;
}

#define BLUR_TAP_OFFSET 0.0009765625f       // 1/1024: distance of each of the four taps from the pixel centre

// 0x101446f0 (TextureObject::DrawBlurred). Binds this texture to all four stages and draws the whole back buffer as
// one quad whose four texture coordinate pairs are each shifted by 1/1024 in a different diagonal direction (stage 0
// up-left, 1 down-right, 2 up-right, 3 down-left), so the fixed-function blend of the stages
// samples four neighbouring texels. Coordinates cover the part of the square targets the size of the screen (half a
// texel in from each edge). Returns 0.
int TextureObject::DrawBlurred()
{
    ShaderVertex vertices[4];
    DAT_10f1fcf0->SetTexture(0, texture);
    DAT_10f1fcf0->SetTexture(1, texture);
    DAT_10f1fcf0->SetTexture(2, texture);
    DAT_10f1fcf0->SetTexture(3, texture);
    float width = (float)DAT_10202b34;
    float height = (float)DAT_10202b38;
    vertices[0].x = 0.0f;
    vertices[0].y = 0.0f;
    vertices[1].x = width;
    vertices[1].y = 0.0f;
    vertices[2].x = 0.0f;
    vertices[2].y = height;
    vertices[3].x = width;
    vertices[3].y = height;
    vertices[0].u3 = vertices[0].u2 = vertices[0].u1 = vertices[0].u0 =
        0.5f / DAT_10202b18;
    vertices[0].v3 = vertices[0].v2 = vertices[0].v1 = vertices[0].v0 =
        0.5f / DAT_10202b18;
    vertices[1].u3 = vertices[1].u2 = vertices[1].u1 = vertices[1].u0 =
        (width - 0.5f) / DAT_10202b18;
    vertices[1].v3 = vertices[1].v2 = vertices[1].v1 = vertices[1].v0 =
        0.5f / DAT_10202b18;
    vertices[2].u3 = vertices[2].u2 = vertices[2].u1 = vertices[2].u0 =
        0.5f / DAT_10202b18;
    vertices[2].v3 = vertices[2].v2 = vertices[2].v1 = vertices[2].v0 =
        (height - 0.5f) / DAT_10202b18;
    vertices[3].u3 = vertices[3].u2 = vertices[3].u1 = vertices[3].u0 =
        (width - 0.5f) / DAT_10202b18;
    vertices[3].v3 = vertices[3].v2 = vertices[3].v1 = vertices[3].v0 =
        (height - 0.5f) / DAT_10202b18;
    vertices[0].u0 -= BLUR_TAP_OFFSET;
    vertices[0].v0 -= BLUR_TAP_OFFSET;
    vertices[1].u0 -= BLUR_TAP_OFFSET;
    vertices[1].v0 -= BLUR_TAP_OFFSET;
    vertices[2].u0 -= BLUR_TAP_OFFSET;
    vertices[2].v0 -= BLUR_TAP_OFFSET;
    vertices[3].u0 -= BLUR_TAP_OFFSET;
    vertices[3].v0 -= BLUR_TAP_OFFSET;
    vertices[0].u1 += BLUR_TAP_OFFSET;
    vertices[0].v1 += BLUR_TAP_OFFSET;
    vertices[1].u1 += BLUR_TAP_OFFSET;
    vertices[1].v1 += BLUR_TAP_OFFSET;
    vertices[2].u1 += BLUR_TAP_OFFSET;
    vertices[2].v1 += BLUR_TAP_OFFSET;
    vertices[3].u1 += BLUR_TAP_OFFSET;
    vertices[3].v1 += BLUR_TAP_OFFSET;
    vertices[0].u2 += BLUR_TAP_OFFSET;
    vertices[0].v2 -= BLUR_TAP_OFFSET;
    vertices[1].u2 += BLUR_TAP_OFFSET;
    vertices[1].v2 -= BLUR_TAP_OFFSET;
    vertices[2].u2 += BLUR_TAP_OFFSET;
    vertices[2].v2 -= BLUR_TAP_OFFSET;
    vertices[3].u2 += BLUR_TAP_OFFSET;
    vertices[3].v2 -= BLUR_TAP_OFFSET;
    vertices[0].u3 -= BLUR_TAP_OFFSET;
    vertices[0].v3 += BLUR_TAP_OFFSET;
    vertices[1].u3 -= BLUR_TAP_OFFSET;
    vertices[1].v3 += BLUR_TAP_OFFSET;
    vertices[2].u3 -= BLUR_TAP_OFFSET;
    vertices[2].v3 += BLUR_TAP_OFFSET;
    vertices[3].u3 -= BLUR_TAP_OFFSET;
    vertices[3].v3 += BLUR_TAP_OFFSET;
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = 0xffffffff;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular = 0xff000000;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = 0.0f;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x4c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f1fcf0->SetPixelShader(DAT_10f1fd08);
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x38);
    DAT_10f1fcf0->SetPixelShader(0);
    return 0;
}

// 0x10149100 (SwapTarget_DrawPreview). Draws the swap target in use as a 256x256 quad at the top left of the screen
// (the part of the square target the size of the back buffer), stages 1 to 3 off. Returns 1.
extern "C" int FUN_10149100(void)
{
    ShaderVertex vertices[4];
    DAT_10f1fcf0->SetTexture(0, DAT_10f1fd28[DAT_10f65490]);
    FUN_10143b80();
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    int width = (int)MIN(DAT_10202b18, (float)DAT_10202b24);
    int height = (int)MIN(DAT_10202b18, (float)DAT_10202b28);
    vertices[0].x = 0.0f;
    vertices[0].y = 0.0f;
    vertices[1].x = 256.0f;
    vertices[1].y = 0.0f;
    vertices[2].x = 0.0f;
    vertices[2].y = 256.0f;
    vertices[3].x = 256.0f;
    vertices[3].y = 256.0f;
    vertices[0].u3 = vertices[0].u2 = vertices[0].u1 = vertices[0].u0 =
        0.0f;
    vertices[0].v3 = vertices[0].v2 = vertices[0].v1 = vertices[0].v0 =
        0.0f;
    vertices[1].u3 = vertices[1].u2 = vertices[1].u1 = vertices[1].u0 =
        (float)width / DAT_10202b18;
    vertices[1].v3 = vertices[1].v2 = vertices[1].v1 = vertices[1].v0 =
        0.0f;
    vertices[2].u3 = vertices[2].u2 = vertices[2].u1 = vertices[2].u0 =
        0.0f;
    vertices[2].v3 = vertices[2].v2 = vertices[2].v1 = vertices[2].v0 =
        (float)height / DAT_10202b18;
    vertices[3].u3 = vertices[3].u2 = vertices[3].u1 = vertices[3].u0 =
        (float)width / DAT_10202b18;
    vertices[3].v3 = vertices[3].v2 = vertices[3].v1 = vertices[3].v0 =
        (float)height / DAT_10202b18;
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = 0xffffffff;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular = 0xff000000;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = 0.0f;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x4c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x38);
    return 1;
}

#include "vec4.h"

// 0x1014c080 (TextureObject::DrawQuad). Draws a two-triangle strip from four points: `positions` give x and y in
// 640x480 screen coordinates (scaled to the back buffer), depth and rhw; `uvs` the texture coordinates; `colours`
// and `speculars` the vertex colours as 0..1 RGBA (the alpha capped at 1 and multiplied by `alpha`; the specular
// colour without alpha). Texture times vertex colour, no culling, stages 1 to 3 off. Returns 0 (drawing nothing) as
// soon as a depth is outside 0..1, else 1; with `skip` it draws nothing and returns 1.
int TextureObject::DrawQuad(const Vec4 *positions, const Vec4 *uvs, const Vec4 *colours, const Vec4 *speculars,
                            float alpha, int skip)
{
    PanelVertex vertices[4];
    BindStage0();
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    if (skip == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            vertices[i].x = scaleX * positions[i].x;
            vertices[i].y = scaleY * positions[i].y;
            vertices[i].z = positions[i].z;
            vertices[i].rhw = positions[i].w;
            vertices[i].u = uvs[i].x;
            vertices[i].v = uvs[i].y;
            float opacity = 1.0f > colours[i].w ? colours[i].w : 1.0f;
            int a = (int)(opacity * alpha * 255.0f);
            if (a > 255)
                a = 255;
            else if (a < 0)
                a = 0;
            int r = (int)(colours[i].x * 255.0f);
            if (r > 255)
                r = 255;
            else if (r < 0)
                r = 0;
            int g = (int)(colours[i].y * 255.0f);
            if (g > 255)
                g = 255;
            else if (g < 0)
                g = 0;
            int b = (int)(colours[i].z * 255.0f);
            if (b > 255)
                b = 255;
            else if (b < 0)
                b = 0;
            vertices[i].colour = ((a << 8 | r) << 8 | g) << 8 | b;
            r = (int)(speculars[i].x * 255.0f);
            if (r > 255)
                r = 255;
            else if (r < 0)
                r = 0;
            g = (int)(speculars[i].y * 255.0f);
            if (g > 255)
                g = 255;
            else if (g < 0)
                g = 0;
            b = (int)(speculars[i].z * 255.0f);
            if (b > 255)
                b = 255;
            else if (b < 0)
                b = 0;
            vertices[i].specular = (r << 8 | g) << 8 | b;
            if (vertices[i].z < 0.0f || vertices[i].z > 1.0f)
                return 0;
        }
        FUN_1013ef30(0x16, 1);              // D3DRS_CULLMODE = D3DCULL_NONE
        FUN_1013ef30(0x8d, 1);              // D3DRS_COLORVERTEX
        DAT_10f1fcf0->SetFVF(0x1c4);
        DAT_10f1fcf0->SetVertexShader(0);
        FUN_1013ef30(0x91, 1);              // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 2);              //   texture
        FUN_1013ef90(0, 6, 0);              //   diffuse
        FUN_1013ef90(0, 1, 4);              // colour: modulate
        FUN_1013ef90(0, 2, 2);              //   texture
        FUN_1013ef90(0, 3, 0);              //   diffuse
        FUN_10143b80();
        DAT_10f1fcf0->SetTexture(2, 0);
        DAT_10f1fcf0->SetTexture(3, 0);
        DAT_10f0d5c8++;
        DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x20);
    }
    return 1;
}

// 0x1014bc60 (TextureObject::DrawCell). Draws the cell u0..u1, v0..v1 of this texture into the screen rectangle
// left..right, top..bottom (back-buffer pixels, no 640x480 scaling) as a two-triangle strip, turned by `angle`
// radians about the rectangle's centre unless it is 0. White at `alpha` (0..1), texture times vertex colour.
// Returns 1.
int TextureObject::DrawCell(int left, int top, int right, int bottom, float u0, float v0, float u1, float v1, float z,
                            float alpha, float angle)
{
    PanelVertex vertices[4];
    BindStage0();
    if (angle != 0.0f)
    {
        Vec4 topLeft;
        Vec4 bottomRight;
        Vec4 topRight;
        Vec4 bottomLeft;
        Vec4 centre;
        centre.x = (float)(left + right) * 0.5f;
        centre.y = (float)(top + bottom) * 0.5f;
        topLeft.x = (float)left - centre.x;
        topLeft.y = (float)top - centre.y;
        bottomRight.x = (float)right - centre.x;
        bottomRight.y = (float)bottom - centre.y;
        topRight.x = (float)right - centre.x;
        topRight.y = (float)top - centre.y;
        bottomLeft.x = (float)left - centre.x;
        bottomLeft.y = (float)bottom - centre.y;
        topLeft.RotateZ(angle);
        bottomRight.RotateZ(angle);
        topRight.RotateZ(angle);
        bottomLeft.RotateZ(angle);
        topLeft.x += centre.x;
        topLeft.y += centre.y;
        bottomRight.x += centre.x;
        bottomRight.y += centre.y;
        topRight.x += centre.x;
        topRight.y += centre.y;
        bottomLeft.x += centre.x;
        bottomLeft.y += centre.y;
        vertices[0].x = topLeft.x;
        vertices[0].y = topLeft.y;
        vertices[1].x = topRight.x;
        vertices[1].y = topRight.y;
        vertices[2].x = bottomLeft.x;
        vertices[2].y = bottomLeft.y;
        vertices[3].x = bottomRight.x;
        vertices[3].y = bottomRight.y;
    }
    else
    {
        vertices[0].x = (float)left;
        vertices[0].y = (float)top;
        vertices[1].x = (float)right;
        vertices[1].y = (float)top;
        vertices[2].x = (float)left;
        vertices[2].y = (float)bottom;
        vertices[3].x = (float)right;
        vertices[3].y = (float)bottom;
    }
    vertices[0].u = u0;
    vertices[0].v = v0;
    vertices[1].u = u1;
    vertices[1].v = v0;
    vertices[2].u = u0;
    vertices[2].v = v1;
    vertices[3].u = u1;
    vertices[3].v = v1;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = a << 24 | 0xffffff;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular = 0xff000000;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = z;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x20);
    return 1;
}
// 0x1014c430 (TextureObject::DrawTinted). Like DrawCell, but the rectangle is given in 640x480 screen coordinates
// (scaled to the back buffer) and the specular colour is an opaque grey of level `grey` (0..1); stages 1 to 3 off.
int TextureObject::DrawTinted(int left, int top, int right, int bottom, float u0, float v0, float u1, float v1,
                              float z, float alpha, float angle, float grey)
{
    PanelVertex vertices[4];
    BindStage0();
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    if (angle != 0.0f)
    {
        Vec4 topLeft;
        Vec4 bottomRight;
        Vec4 topRight;
        Vec4 bottomLeft;
        Vec4 centre;
        centre.x = (float)(left + right) * 0.5f;
        centre.y = (float)(top + bottom) * 0.5f;
        topLeft.x = (float)left - centre.x;
        topLeft.y = (float)top - centre.y;
        bottomRight.x = (float)right - centre.x;
        bottomRight.y = (float)bottom - centre.y;
        topRight.x = (float)right - centre.x;
        topRight.y = (float)top - centre.y;
        bottomLeft.x = (float)left - centre.x;
        bottomLeft.y = (float)bottom - centre.y;
        topLeft.RotateZ(angle);
        bottomRight.RotateZ(angle);
        topRight.RotateZ(angle);
        bottomLeft.RotateZ(angle);
        topLeft.x += centre.x;
        topLeft.y += centre.y;
        bottomRight.x += centre.x;
        bottomRight.y += centre.y;
        topRight.x += centre.x;
        topRight.y += centre.y;
        bottomLeft.x += centre.x;
        bottomLeft.y += centre.y;
        vertices[0].x = topLeft.x * scaleX;
        vertices[0].y = topLeft.y * scaleY;
        vertices[1].x = topRight.x * scaleX;
        vertices[1].y = topRight.y * scaleY;
        vertices[2].x = bottomLeft.x * scaleX;
        vertices[2].y = bottomLeft.y * scaleY;
        vertices[3].x = bottomRight.x * scaleX;
        vertices[3].y = bottomRight.y * scaleY;
    }
    else
    {
        vertices[0].x = (float)left * scaleX;
        vertices[0].y = (float)top * scaleY;
        vertices[1].x = (float)right * scaleX;
        vertices[1].y = (float)top * scaleY;
        vertices[2].x = (float)left * scaleX;
        vertices[2].y = (float)bottom * scaleY;
        vertices[3].x = (float)right * scaleX;
        vertices[3].y = (float)bottom * scaleY;
    }
    vertices[0].u = u0;
    vertices[0].v = v0;
    vertices[1].u = u1;
    vertices[1].v = v0;
    vertices[2].u = u0;
    vertices[2].v = v1;
    vertices[3].u = u1;
    vertices[3].v = v1;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = a << 24 | 0xffffff;
    int level = (int)(grey * 255.0f);
    if (level > 255)
        level = 255;
    else if (level < 0)
        level = 0;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular =
        0xff000000 | level << 16 | level << 8 | level;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = z;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 4);                  // colour: modulate
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    FUN_1013ef90(1, 1, 1);                  // stage 1 off
    FUN_1013ef90(1, 4, 1);
    FUN_10143b80();
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x20);
    return 1;
}

// 0x1014c960 (TextureObject::DrawColoured). Like DrawCell, but the rectangle is given in 640x480 screen coordinates
// (scaled to the back buffer) and the diffuse colour is `alpha`, `red`, `green`, `blue` (each 0..1); no specular.
// Returns 1.
int TextureObject::DrawColoured(int left, int top, int right, int bottom, float u0, float v0, float u1, float v1,
                                float z, float alpha, float angle, float red, float green, float blue)
{
    PanelVertex vertices[4];
    BindStage0();
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    if (angle != 0.0f)
    {
        Vec4 topLeft;
        Vec4 bottomRight;
        Vec4 topRight;
        Vec4 bottomLeft;
        Vec4 centre;
        centre.x = (float)(left + right) * 0.5f;
        centre.y = (float)(top + bottom) * 0.5f;
        topLeft.x = (float)left - centre.x;
        topLeft.y = (float)top - centre.y;
        bottomRight.x = (float)right - centre.x;
        bottomRight.y = (float)bottom - centre.y;
        topRight.x = (float)right - centre.x;
        topRight.y = (float)top - centre.y;
        bottomLeft.x = (float)left - centre.x;
        bottomLeft.y = (float)bottom - centre.y;
        topLeft.RotateZ(angle);
        bottomRight.RotateZ(angle);
        topRight.RotateZ(angle);
        bottomLeft.RotateZ(angle);
        topLeft.x += centre.x;
        topLeft.y += centre.y;
        bottomRight.x += centre.x;
        bottomRight.y += centre.y;
        topRight.x += centre.x;
        topRight.y += centre.y;
        bottomLeft.x += centre.x;
        bottomLeft.y += centre.y;
        vertices[0].x = topLeft.x * scaleX;
        vertices[0].y = topLeft.y * scaleY;
        vertices[1].x = topRight.x * scaleX;
        vertices[1].y = topRight.y * scaleY;
        vertices[2].x = bottomLeft.x * scaleX;
        vertices[2].y = bottomLeft.y * scaleY;
        vertices[3].x = bottomRight.x * scaleX;
        vertices[3].y = bottomRight.y * scaleY;
    }
    else
    {
        vertices[0].x = (float)left * scaleX;
        vertices[0].y = (float)top * scaleY;
        vertices[1].x = (float)right * scaleX;
        vertices[1].y = (float)top * scaleY;
        vertices[2].x = (float)left * scaleX;
        vertices[2].y = (float)bottom * scaleY;
        vertices[3].x = (float)right * scaleX;
        vertices[3].y = (float)bottom * scaleY;
    }
    vertices[0].u = u0;
    vertices[0].v = v0;
    vertices[1].u = u1;
    vertices[1].v = v0;
    vertices[2].u = u0;
    vertices[2].v = v1;
    vertices[3].u = u1;
    vertices[3].v = v1;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    unsigned int colour = (((unsigned int)(int)(red * 255.0f) | a << 8) << 8 | (int)(green * 255.0f)) << 8 |
                          (int)(blue * 255.0f);
    vertices[0].colour = vertices[1].colour = vertices[2].colour = vertices[3].colour = colour;
    vertices[0].specular = vertices[1].specular = vertices[2].specular = vertices[3].specular = 0xff000000;
    vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = z;
    vertices[0].rhw = vertices[1].rhw = vertices[2].rhw = vertices[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: modulate
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, vertices, 0x20);
    return 1;
}

// 0x1014d4e0 (TextureObject::DrawFrame). Draws the speech-balloon panel: one triangle fan (21 points) from the
// centre of the rectangle left..right, top..bottom pulled 8 - `border` pixels inwards, with the corners cut off in
// steps of 2, 4 and 8 pixels and a tail 20 + `border` pixels long below the middle of the bottom edge. The points are
// given in 640x480 screen coordinates and scaled to the back buffer; the texture repeats every 64 pixels. White at
// `alpha` (0..1), with this texture times the vertex colour when `textured`, else untextured; depth writes on.
// Returns 1.
int TextureObject::DrawFrame(int left, int top, int right, int bottom, float z, float alpha, int textured, int border)
{
    PanelVertex vertices[34];
    if (textured != 0)
    {
        BindStage0();
        FUN_1013ef90(0, 1, 4);              // colour: modulate
        FUN_1013ef90(0, 2, 2);              //   texture
        FUN_1013ef90(0, 3, 0);              //   diffuse
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 2);              //   texture
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    else
    {
        FUN_10143b50();
        FUN_1013ef90(0, 1, 4);              // colour: modulate
        FUN_1013ef90(0, 2, 2);              //   texture
        FUN_1013ef90(0, 3, 0);              //   diffuse
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 0);              //   diffuse
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    float scaleX = (float)DAT_10202b34 / 640.0f;
    left += 8 - border;
    top += 8 - border;
    right += border - 8;
    bottom += border - 8;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    int xs[21] = {(left + right) / 2, left, left + 2, left + 4, left + 8, right - 8, right - 4, right - 2, right,
                  right, right - 2, right - 4, right - 8, (left + right) / 2 + 6 + border, (left + right) / 2,
                  (left + right) / 2 - border - 6, left + 8, left + 4, left + 2, left, left};
    int ys[21] = {(top + bottom) / 2, top + 8, top + 4, top + 2, top, top, top + 2, top + 4, top + 8,
                  bottom - 8, bottom - 4, bottom - 2, bottom, bottom, bottom + 20 + border, bottom, bottom,
                  bottom - 2, bottom - 4, bottom - 8, top + 8};
    for (int i = 0; i < 21; i++)
    {
        float x = (float)xs[i];
        float y = (float)ys[i];
        vertices[i].x = x * scaleX;
        vertices[i].y = y * scaleY;
        vertices[i].colour = a << 24 | 0xffffff;
        vertices[i].specular = 0xff000000;
        vertices[i].z = z;
        vertices[i].rhw = 1.0f;
        vertices[i].u = x / 64.0f;
        vertices[i].v = y / 64.0f;
    }
    FUN_1013ef30(0xe, 1);                   // D3DRS_ZWRITEENABLE
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(6 /* D3DPT_TRIANGLEFAN */, 19, vertices, 0x20);
    return 1;
}

// 0x1014d850 (TextureObject::DrawFillAlt). Fills the alternative panel inside DrawFrameAlt's outline with a soft
// edge: a triangle strip zigzagging between an outer ring (the frame outline, alpha 0) and an inner ring 3 pixels
// further in (alpha `alpha`), each of 17 points. The rectangle is pulled 8 - `border` pixels inwards; the points are
// given in 640x480 screen coordinates and scaled to the back buffer, and the texture is mapped pixel for pixel. The
// colour of each point comes from a table of four (all white). Returns 1.
int TextureObject::DrawFillAlt(int left, int top, int right, int bottom, float z, float alpha, int textured,
                               int border)
{
    PanelVertex vertices[68];
    if (textured != 0)
    {
        BindStage0();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 2);              //   texture
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    else
    {
        FUN_10143b50();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 0);              //   diffuse
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    float scaleX = (float)DAT_10202b34 / 640.0f;
    left += 8 - border;
    top += 8 - border;
    right += border - 8;
    bottom += border - 8;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    unsigned int colours[4] = {0xffffff, 0xffffff, 0xffffff, 0xffffff};
    int shades[34] = {1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1,
                      1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1};
    // Points 0..16: the inner ring; 17..33: the outer ring.
    int ys[34] = {top + 11, top + 7, top + 5, top + 3, top + 3, top + 5, top + 7, top + 11, bottom - 12, bottom - 8,
                  bottom - 6, bottom - 4, bottom - 4, bottom - 6, bottom - 8, bottom - 12, top + 11,
                  top + 8, top + 4, top + 2, top, top, top + 2, top + 4, top + 8, bottom - 8, bottom - 4,
                  bottom - 2, bottom, bottom, bottom - 2, bottom - 4, bottom - 8, top + 8};
    int xs[34] = {left + 3, left + 5, left + 7, left + 11, right - 11, right - 7, right - 5, right - 3, right - 3,
                  right - 5, right - 7, right - 11, left + 11, left + 7, left + 5, left + 3, left + 3,
                  left, left + 2, left + 4, left + 8, right - 8, right - 4, right - 2, right, right, right - 2,
                  right - 4, right - 8, left + 8, left + 4, left + 2, left, left};
    float texWidth = (float)width;
    float texHeight = (float)height;
    for (int i = 0; i < 34; i++)
    {
        int point = (i & 1) * 17 + i / 2;
        float x = (float)xs[point];
        float y = (float)ys[point];
        vertices[i].x = x * scaleX;
        vertices[i].y = y * scaleY;
        vertices[i].u = x / texWidth;
        vertices[i].v = y / texHeight;
        if ((i & 1) != 0)
            vertices[i].colour = colours[shades[point]] | a << 24;
        else
            vertices[i].colour = colours[shades[point]];
        vertices[i].specular = 0xff000000;
        vertices[i].z = z;
        vertices[i].rhw = 1.0f;
    }
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 32, vertices, 0x20);
    return 1;
}

// 0x1014e5d0 (TextureObject::DrawFrameAltStretched). DrawFrameAlt with the texture stretched over the frame instead
// of mapped pixel for pixel: the texture coordinates run from 5 / size across the frame's size + 15.
int TextureObject::DrawFrameAltStretched(int left, int top, int right, int bottom, float z, float alpha,
                                         int textured, int border)
{
    PanelVertex vertices[34];
    if (textured != 0)
    {
        BindStage0();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 2);              //   texture
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    else
    {
        FUN_10143b50();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 0);              //   diffuse
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    float scaleX = (float)DAT_10202b34 / 640.0f;
    left += 8 - border;
    top += 8 - border;
    right += border - 8;
    bottom += border - 8;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    int xs[18] = {left + 10, left, left + 2, left + 4, left + 8, right - 8, right - 4, right - 2, right,
                  right, right - 2, right - 4, right - 8, left + 8, left + 4, left + 2, left, left};
    int ys[18] = {top + 10, top + 8, top + 4, top + 2, top, top, top + 2, top + 4, top + 8,
                  bottom - 8, bottom - 4, bottom - 2, bottom, bottom, bottom - 2, bottom - 4, bottom - 8, top + 8};
    float uLeft = (float)left;
    float uSpan = (float)(right - left + 15);
    float uOffset = 5.0f / (float)(right - left);
    float vTop = (float)top;
    float vSpan = (float)(bottom - top + 15);
    float vOffset = 5.0f / (float)(bottom - top);
    for (int i = 0; i < 18; i++)
    {
        float x = (float)xs[i];
        float y = (float)ys[i];
        vertices[i].x = x * scaleX;
        vertices[i].y = y * scaleY;
        vertices[i].colour = a << 24 | 0xffffff;
        vertices[i].specular = 0xff000000;
        vertices[i].z = z;
        vertices[i].rhw = 1.0f;
        vertices[i].u = (x - uLeft) / uSpan + uOffset;
        vertices[i].v = (y - vTop) / vSpan + vOffset;
    }
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(6 /* D3DPT_TRIANGLEFAN */, 16, vertices, 0x20);
    return 1;
}

// 0x1014e940 (TextureObject::DrawFrameAlt). Draws the alternative panel frame: one triangle fan (18 points) around
// the rectangle left..right, top..bottom pulled 8 - `border` pixels inwards, with the corners cut off in steps of
// 2, 4 and 8 pixels. The points are given in 640x480 screen coordinates and scaled to the back buffer; the texture
// coordinates map the points onto this texture pixel for pixel. White at `alpha` (0..1), with this texture times the
// vertex alpha when `textured`, else untextured. Returns 1.
int TextureObject::DrawFrameAlt(int left, int top, int right, int bottom, float z, float alpha, int textured,
                                int border)
{
    PanelVertex vertices[34];
    if (textured != 0)
    {
        BindStage0();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 2);              //   texture
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    else
    {
        FUN_10143b50();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 0);              //   diffuse
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    float scaleX = (float)DAT_10202b34 / 640.0f;
    left += 8 - border;
    top += 8 - border;
    right += border - 8;
    bottom += border - 8;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    int xs[18] = {left + 10, left, left + 2, left + 4, left + 8, right - 8, right - 4, right - 2, right,
                  right, right - 2, right - 4, right - 8, left + 8, left + 4, left + 2, left, left};
    int ys[18] = {top + 10, top + 8, top + 4, top + 2, top, top, top + 2, top + 4, top + 8,
                  bottom - 8, bottom - 4, bottom - 2, bottom, bottom, bottom - 2, bottom - 4, bottom - 8, top + 8};
    float texWidth = (float)width;
    float texHeight = (float)height;
    for (int i = 0; i < 18; i++)
    {
        float x = (float)xs[i];
        float y = (float)ys[i];
        vertices[i].x = x * scaleX;
        vertices[i].y = y * scaleY;
        vertices[i].colour = a << 24 | 0xffffff;
        vertices[i].specular = 0xff000000;
        vertices[i].z = z;
        vertices[i].rhw = 1.0f;
        vertices[i].u = x / texWidth;
        vertices[i].v = y / texHeight;
    }
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(6 /* D3DPT_TRIANGLEFAN */, 16, vertices, 0x20);
    return 1;
}

// 0x1014ec30 (TextureObject::DrawFrameAltTinted). DrawFrameAlt in the colour `rgb` (0xRRGGBB) instead of white.
int TextureObject::DrawFrameAltTinted(int left, int top, int right, int bottom, float z, float alpha, int textured,
                                      int border, unsigned int rgb)
{
    PanelVertex vertices[34];
    if (textured != 0)
    {
        BindStage0();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 2);              //   texture
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    else
    {
        FUN_10143b50();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 0);              //   diffuse
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    float scaleX = (float)DAT_10202b34 / 640.0f;
    left += 8 - border;
    top += 8 - border;
    right += border - 8;
    bottom += border - 8;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    int xs[18] = {left + 10, left, left + 2, left + 4, left + 8, right - 8, right - 4, right - 2, right,
                  right, right - 2, right - 4, right - 8, left + 8, left + 4, left + 2, left, left};
    int ys[18] = {top + 10, top + 8, top + 4, top + 2, top, top, top + 2, top + 4, top + 8,
                  bottom - 8, bottom - 4, bottom - 2, bottom, bottom, bottom - 2, bottom - 4, bottom - 8, top + 8};
    float texWidth = (float)width;
    float texHeight = (float)height;
    for (int i = 0; i < 18; i++)
    {
        float x = (float)xs[i];
        float y = (float)ys[i];
        vertices[i].x = x * scaleX;
        vertices[i].y = y * scaleY;
        vertices[i].colour = a << 24 | rgb;
        vertices[i].specular = 0xff000000;
        vertices[i].z = z;
        vertices[i].rhw = 1.0f;
        vertices[i].u = x / texWidth;
        vertices[i].v = y / texHeight;
    }
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(6 /* D3DPT_TRIANGLEFAN */, 16, vertices, 0x20);
    return 1;
}

// 0x10144bf0 (TextureObject::DrawFrameAltPlain). The DrawFrameAlt shape in the colour `rgb` with no texture bound
// in either case; `vertexAlpha` only switches the stage 0 alpha operation from modulate to "select argument 2".
int TextureObject::DrawFrameAltPlain(int left, int top, int right, int bottom, float z, float alpha, int vertexAlpha,
                                     int border, unsigned int rgb)
{
    PanelVertex vertices[34];
    if (vertexAlpha != 0)
    {
        FUN_10143b50();
        FUN_1013ef90(0, 4, 3);              // alpha: the vertex alpha (select argument 2)
        FUN_1013ef90(0, 5, 2);
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    else
    {
        FUN_10143b50();
        FUN_1013ef90(0, 4, 4);              // alpha: modulate
        FUN_1013ef90(0, 5, 0);              //   diffuse
        FUN_1013ef90(0, 6, 0);              //   diffuse
    }
    float scaleX = (float)DAT_10202b34 / 640.0f;
    left += 8 - border;
    top += 8 - border;
    right += border - 8;
    bottom += border - 8;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    int a = (int)(alpha * 255.0f);
    if (a > 255)
        a = 255;
    else if (a < 0)
        a = 0;
    int xs[18] = {left + 10, left, left + 2, left + 4, left + 8, right - 8, right - 4, right - 2, right,
                  right, right - 2, right - 4, right - 8, left + 8, left + 4, left + 2, left, left};
    int ys[18] = {top + 10, top + 8, top + 4, top + 2, top, top, top + 2, top + 4, top + 8,
                  bottom - 8, bottom - 4, bottom - 2, bottom, bottom, bottom - 2, bottom - 4, bottom - 8, top + 8};
    float texWidth = (float)width;
    float texHeight = (float)height;
    for (int i = 0; i < 18; i++)
    {
        float x = (float)xs[i];
        float y = (float)ys[i];
        vertices[i].x = x * scaleX;
        vertices[i].y = y * scaleY;
        vertices[i].colour = a << 24 | rgb;
        vertices[i].specular = 0xff000000;
        vertices[i].z = z;
        vertices[i].rhw = 1.0f;
        vertices[i].u = x / texWidth;
        vertices[i].v = y / texHeight;
    }
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(6 /* D3DPT_TRIANGLEFAN */, 16, vertices, 0x20);
    return 1;
}
