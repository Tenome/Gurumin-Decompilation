// Texture stage helpers and the material slot table of a mesh.

#include "d3d9_min.h"
#include "texture_object.h"

#include "engine_10143.h"

#include "render_vars.h"


// Unbinds texture stage 0 (only when something is bound).
extern "C" void FUN_10143b50(void)
{
    if (DAT_10f6550c != TEXTURE_BINDING_UNKNOWN)
    {
        DAT_10f1fcf0->SetTexture(0, 0);
        DAT_10f6550c = TEXTURE_BINDING_UNKNOWN;
    }
}

// Unbinds texture stage 1 (only when something is bound).
extern "C" void FUN_10143b80(void)
{
    if (DAT_10f65510 != TEXTURE_BINDING_UNKNOWN)
    {
        DAT_10f1fcf0->SetTexture(1, 0);
        DAT_10f65510 = TEXTURE_BINDING_UNKNOWN;
    }
}

// 0x10143bb0. Constructor: no texture and no surface yet (called first by TextureObject's constructor).
TextureHandle::TextureHandle()
{
    texture = 0;
    surface = 0;
}

// Unbinds texture stages 0..3. Returns 0.
extern "C" int FUN_101446b0(void)
{
    FUN_10143b50();
    FUN_10143b80();
    DAT_10f1fcf0->SetTexture(2, 0);
    DAT_10f1fcf0->SetTexture(3, 0);
    return 0;
}

// Resets the table: no owner (-1), no entries, all pointers cleared.
void SlotTable::M_10145a10()
{
    m_0000 = -1;
    m_0044 = 0;
    m_000c = 0;
    m_0014 = 0;
    m_0010 = 0;
    m_0018 = 0;
    m_001c = 0;
    m_0020 = 0;
    m_0024 = 0;
    m_0028 = 0;
    m_002c = 0;
    m_0030 = 0;
    m_0034 = 0;
    m_0038 = 0;
    m_0048 = 0;
}

// Limits later operations to entries first..last-1.
int SlotTable::M_10146d90(int first, int last)
{
    m_003c = first;
    m_0040 = last;
    return 1;
}

// Replaces the material of every entry in range whose current or original material is `key` (or of all
// entries when key is -1) with `value`. Node::M_1019e830 runs it over a whole model (texture swaps,
// e.g. costume colours).
int SlotTable::M_10147690(int key, int value)
{
    for (int i = m_003c; i < m_0044 && i < m_0040; i++)
    {
        if (key == -1 || m_0014[i] == key || m_0010[i] == key)
            m_0014[i] = value;
    }
    return 0;
}

// Sets the material of entry `index` (when in range).
int SlotTable::M_101476e0(int index, int value)
{
    if (index < m_0044 && index < m_0040)
        m_0014[index] = value;
    return 0;
}

// Ages every texture slot by one frame (DAT_10f181e0, used to evict unused textures).
extern "C" int FUN_10147a00(void)
{
    for (int i = 0; i < 0x5a0; i++)
        DAT_10f181e0[i]++;
    return 1;
}

extern "C" void FUN_1013ef30(int state, int value);              // Render_SetRenderState (cached)
extern "C" void FUN_1013ef90(int stage, int type, int value);    // Render_SetTextureStageState (cached)
extern "C" void FUN_1013f070(int mode, int a, int b);            // sets the 2D blend mode


// 0x10143bc0 (Render_DrawGradientQuad). Draws an untextured rectangle from (left, top) to (right, bottom) given on
// the 640x480 screen, with colour `top` along its top edge and `bottom` along its bottom edge, opacity `alpha`
// (0..1) and depth `z`. Returns 1.
extern "C" int __stdcall FUN_10143bc0(int left, int top, int right, int bottom, unsigned int topColour,
                                      unsigned int bottomColour, float z, float alpha)
{
    ColourVertex quad[4];
    FUN_10143b50();
    quad[3].v = quad[3].u = quad[2].v = quad[1].u = 0;
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    quad[2].x = quad[0].x = (float)left * scaleX;
    quad[1].y = quad[0].y = (float)top * scaleY;
    quad[3].x = quad[1].x = (float)right * scaleX;
    quad[3].y = quad[2].y = (float)bottom * scaleY;
    quad[2].u = quad[1].v = quad[0].v = quad[0].u = 0.001f;
    int opacity = (int)(alpha * 255.0f);
    if (opacity > 0xff)
        opacity = 0xff;
    else if (opacity < 0)
        opacity = 0;
    quad[0].diffuse = quad[1].diffuse = (opacity << 24) | topColour;
    quad[2].diffuse = quad[3].diffuse = (opacity << 24) | bottomColour;
    quad[0].specular = quad[1].specular = quad[2].specular = quad[3].specular = 0xff000000;
    quad[0].z = quad[1].z = quad[2].z = quad[3].z = z;
    quad[0].rhw = quad[1].rhw = quad[2].rhw = quad[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 4);                  // alpha: diffuse x texture
    FUN_1013ef90(0, 5, 2);
    FUN_1013ef90(0, 6, 0);
    FUN_1013ef90(0, 1, 4);                  // colour
    FUN_1013ef90(0, 2, 2);
    FUN_1013ef90(0, 3, 0);
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5, 2, quad, sizeof(ColourVertex));
    return 1;
}

// 0x10143e00 (Render_FillRect). Draws an untextured rectangle from (left, top) to (right, bottom) given on the
// 640x480 screen in `colour` with opacity `alpha` (0..1) and depth `z`, after unbinding both texture stages (stage 0
// passes the vertex colour through). Used for the screen fades and the cinema bars. Returns 1.
extern "C" int FUN_10143e00(int left, int top, int right, int bottom, unsigned int colour, float z, float alpha)
{
    ColourVertex quad[4];
    FUN_10143b50();
    FUN_10143b80();
    quad[3].v = quad[3].u = quad[2].v = quad[1].u = 0;
    float scaleX = (float)DAT_10202b34 / 640.0f;
    float scaleY = (float)DAT_10202b38 / 480.0f;
    float x0 = (float)left * scaleX;
    quad[0].x = x0;
    quad[2].x = x0;
    float y0 = (float)top * scaleY;
    quad[0].y = y0;
    quad[1].y = y0;
    float x1 = (float)right * scaleX;
    quad[1].x = x1;
    quad[3].x = x1;
    float y1 = (float)bottom * scaleY;
    quad[2].y = y1;
    quad[3].y = y1;
    quad[2].u = quad[1].v = quad[0].v = quad[0].u = 0.001f;
    int opacity = (int)(alpha * 255.0f);
    if (opacity > 0xff)
        opacity = 0xff;
    else if (opacity < 0)
        opacity = 0;
    quad[0].diffuse = quad[1].diffuse = quad[2].diffuse = quad[3].diffuse = (opacity << 24) | colour;
    quad[0].specular = quad[1].specular = quad[2].specular = quad[3].specular = 0xff000000;
    quad[0].z = quad[1].z = quad[2].z = quad[3].z = z;
    quad[0].rhw = quad[1].rhw = quad[2].rhw = quad[3].rhw = 1.0f;
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 3);                  // alpha: select argument 2
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 3);                  // colour: select argument 2
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f0d5c8++;
    DAT_10f1fcf0->DrawPrimitiveUP(5, 2, quad, sizeof(ColourVertex));
    return 1;
}

// 0x10144040. Draws a 34x34 blue square (0xff0000ff) in the top-left corner of the screen (from -1 to 33 in x and y), untextured
// (stage 0 passes the vertex colour through), then sets 2D blend mode 6. Returns 1.
extern "C" int FUN_10144040(void)
{
    FUN_10143b50();
    // The corners (-1, -1), (33, -1), (-1, 33), (33, 33) with texture coordinates 0.001..1; the shared values are
    // set with chained assignments.
    ColourVertex quad[4];
    quad[3].y = quad[3].x = quad[2].y = quad[1].x = 33.0f;
    quad[3].v = quad[3].u = quad[2].v = quad[1].u = 1.0f;
    quad[2].x = quad[1].y = quad[0].y = quad[0].x = -1.0f;
    quad[2].u = quad[1].v = quad[0].v = quad[0].u = 0.001f;
    quad[0].diffuse = quad[1].diffuse = quad[2].diffuse = quad[3].diffuse = 0xff0000ff;
    quad[0].specular = quad[1].specular = quad[2].specular = quad[3].specular = 0xff000000;
    quad[0].z = quad[1].z = quad[2].z = quad[3].z = 0;
    quad[0].rhw = quad[1].rhw = quad[2].rhw = quad[3].rhw = 1.0f;
    FUN_1013ef30(0x16, 3);                  // D3DRS_CULLMODE = D3DCULL_CCW
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 3);                  // alpha: select argument 2
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 3);                  // colour: select argument 2
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, quad, 0x20);
    FUN_1013f070(6, 0, 0);
    return 1;
}

// 0x101441b0. Same as FUN_10144040 with a 257x257 black square (from -1 to 256): the background of the 256x256
// save thumbnail (both are called by FUN_10017d30). Returns 1.
extern "C" int FUN_101441b0(void)
{
    FUN_10143b50();
    // The corners (-1, -1), (256, -1), (-1, 256), (256, 256) with texture coordinates 0.001..1; the shared values are
    // set with chained assignments.
    ColourVertex quad[4];
    quad[3].y = quad[3].x = quad[2].y = quad[1].x = 256.0f;
    quad[3].v = quad[3].u = quad[2].v = quad[1].u = 1.0f;
    quad[2].x = quad[1].y = quad[0].y = quad[0].x = -1.0f;
    quad[2].u = quad[1].v = quad[0].v = quad[0].u = 0.001f;
    quad[0].diffuse = quad[1].diffuse = quad[2].diffuse = quad[3].diffuse = 0xff000000;
    quad[0].specular = quad[1].specular = quad[2].specular = quad[3].specular = 0xff0000ff;
    quad[0].z = quad[1].z = quad[2].z = quad[3].z = 0;
    quad[0].rhw = quad[1].rhw = quad[2].rhw = quad[3].rhw = 1.0f;
    FUN_1013ef30(0x16, 3);                  // D3DRS_CULLMODE = D3DCULL_CCW
    FUN_1013ef30(0x8d, 1);                  // D3DRS_COLORVERTEX
    DAT_10f1fcf0->SetFVF(0x1c4);
    DAT_10f1fcf0->SetVertexShader(0);
    FUN_1013ef30(0x91, 1);                  // D3DRS_DIFFUSEMATERIALSOURCE = D3DMCS_COLOR1
    FUN_1013ef90(0, 4, 3);                  // alpha: select argument 2
    FUN_1013ef90(0, 5, 2);                  //   texture
    FUN_1013ef90(0, 6, 0);                  //   diffuse
    FUN_1013ef90(0, 1, 3);                  // colour: select argument 2
    FUN_1013ef90(0, 2, 2);                  //   texture
    FUN_1013ef90(0, 3, 0);                  //   diffuse
    DAT_10f1fcf0->DrawPrimitiveUP(5 /* D3DPT_TRIANGLESTRIP */, 2, quad, 0x20);
    FUN_1013f070(6, 0, 0);
    return 1;
}
