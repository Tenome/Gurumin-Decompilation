// Drawing the world's pictures: the 2D images the scripts place on the screen (a texture slot, or the copy of the
// screen for the blur, mirror and flash effects), each with its own blend mode.

#include "vec4.h"
#include "matrix4.h"
#include "texture_object.h"
#include "render_state.h"
#include "world_renderer.h"

Vec4 Project(const Vec4 &point, const Matrix4 &matrix);   // 0x101ae060: to -1..1 screen space

// Size of the screen copy texture (the copied screen covers screenWidth x screenHeight texels of it).
#define SCREEN_TEXTURE_WIDTH 1024.0f
#define SCREEN_TEXTURE_HEIGHT 512.0f
// The same sizes as doubles, for the half-texel rectangle of PICTURE_SCREEN_EXACT (computed in double).
#define SCREEN_TEXTURE_WIDTH_D 1024.0
#define SCREEN_TEXTURE_HEIGHT_D 512.0
// Texels cut off each edge of the screen copy by PICTURE_SCREEN and PICTURE_SCREEN_FLIPPED.
#define SCREEN_BORDER 2
// Half a texel, the inset of PICTURE_SCREEN_EXACT.
#define HALF_TEXEL 0.5
// Offset in pixels of the darkened copy a PICTURE_BLEND_SHADOWED picture draws first.
#define SHADOW_OFFSET 2
// The 640x480 screen the pictures are placed on.
#define SCREEN_WIDTH 640.0f
#define SCREEN_HEIGHT 480.0f
#define SCREEN_CENTRE_X 320.0f
#define SCREEN_CENTRE_Y 240.0f

// 0x10167440 (WorldRenderer::DrawPictures). Called by the frame drawing (0x1008c680) with the view matrix. Draws
// every picture slot that is not placed in the world and has a kind or a texture, as a rectangle of its size around
// its position, with depth testing and writing off. Kind 0 shows its texture slot with its own texture rectangle,
// the other kinds show the screen copy. A PICTURE_BLEND_SHADOWED picture is drawn twice: first darkened, 2 pixels
// down and to the right, then added. (The branches that project an in-world picture with `view` are never reached:
// in-world pictures are skipped first.)
void WorldRenderer::DrawPictures(const Matrix4 &view)
{
    FUN_1013f070(RENDER_DEPTH_WRITE, 0, 0);
    FUN_1013f070(RENDER_DEPTH_TEST, 0, 0);
    for (int i = 0; i < PICTURE_COUNT; i++)
    {
        if (pictureInWorld[i] != 0)
            continue;
        for (int pass = 0; pass < 2; pass++)
        {
            if (pictureKind[i] == PICTURE_TEXTURE && textures[i] == -1)
                continue;
            int offsetX = 0;
            int offsetY = 0;
            if (pictureBlend[i] == PICTURE_BLEND_ALPHA)
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_ALPHA, 0);
            if (pictureBlend[i] == PICTURE_BLEND_ADD)
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
            if (pictureBlend[i] == PICTURE_BLEND_MULTIPLY)
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_MULTIPLY, 0);
            if (pictureBlend[i] == PICTURE_BLEND_DARKEN)
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_DARKEN, 0);
            if (pictureBlend[i] == PICTURE_BLEND_SHADOWED && pass == 0)
            {
                // The shadow: darkened and moved down and right.
                offsetX = SHADOW_OFFSET;
                offsetY = SHADOW_OFFSET;
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_DARKEN, 0);
            }
            if (pictureBlend[i] == PICTURE_BLEND_SHADOWED && pass == 1)
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
            // Only shadowed pictures have a second pass.
            if (pictureBlend[i] != PICTURE_BLEND_SHADOWED && pass == 1)
                break;

            char kind = pictureKind[i];
            if (kind == PICTURE_SCREEN)
            {
                if (pictureInWorld[i] == 0)
                {
                    DAT_118d0ba8[screenTexture].DrawTinted(
                        (int)(picturePositions[i].x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(picturePositions[i].y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(offsetX + picturePositions[i].x + pictureSizes[i].x * 0.5f),
                        (int)(picturePositions[i].y + pictureSizes[i].y * 0.5f + offsetY),
                        SCREEN_BORDER / SCREEN_TEXTURE_WIDTH, SCREEN_BORDER / SCREEN_TEXTURE_HEIGHT,
                        (float)(screenWidth - SCREEN_BORDER) / SCREEN_TEXTURE_WIDTH,
                        (float)(screenHeight - SCREEN_BORDER) / SCREEN_TEXTURE_HEIGHT, picturePositions[i].z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
                else
                {
                    Vec4 screen;
                    screen.Copy(Project(picturePositions[i], view));
                    screen.x = screen.x * SCREEN_WIDTH / 2.0f + SCREEN_CENTRE_X;
                    screen.y = SCREEN_CENTRE_Y - screen.y * SCREEN_HEIGHT / 2.0f;
                    DAT_118d0ba8[screenTexture].DrawTinted(
                        (int)(screen.x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(screen.x + pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y + pictureSizes[i].y * 0.5f + offsetY),
                        SCREEN_BORDER / SCREEN_TEXTURE_WIDTH, SCREEN_BORDER / SCREEN_TEXTURE_HEIGHT,
                        (float)(screenWidth - SCREEN_BORDER) / SCREEN_TEXTURE_WIDTH,
                        (float)(screenHeight - SCREEN_BORDER) / SCREEN_TEXTURE_HEIGHT, screen.z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
            }
            else if (kind == PICTURE_SCREEN_FLIPPED)
            {
                if (pictureInWorld[i] == 0)
                {
                    DAT_118d0ba8[screenTexture].DrawTinted(
                        (int)(picturePositions[i].x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(picturePositions[i].y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(offsetX + picturePositions[i].x + pictureSizes[i].x * 0.5f),
                        (int)(picturePositions[i].y + pictureSizes[i].y * 0.5f + offsetY),
                        SCREEN_BORDER / SCREEN_TEXTURE_WIDTH,
                        (float)(screenHeight - SCREEN_BORDER) / SCREEN_TEXTURE_HEIGHT,
                        (float)(screenWidth - SCREEN_BORDER) / SCREEN_TEXTURE_WIDTH,
                        SCREEN_BORDER / SCREEN_TEXTURE_HEIGHT, picturePositions[i].z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
                else
                {
                    Vec4 screen;
                    screen.Copy(Project(picturePositions[i], view));
                    screen.x = screen.x * SCREEN_WIDTH / 2.0f + SCREEN_CENTRE_X;
                    screen.y = SCREEN_CENTRE_Y - screen.y * SCREEN_HEIGHT / 2.0f;
                    DAT_118d0ba8[screenTexture].DrawTinted(
                        (int)(screen.x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(screen.x + pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y + pictureSizes[i].y * 0.5f + offsetY),
                        SCREEN_BORDER / SCREEN_TEXTURE_WIDTH,
                        (float)(screenHeight - SCREEN_BORDER) / SCREEN_TEXTURE_HEIGHT,
                        (float)(screenWidth - SCREEN_BORDER) / SCREEN_TEXTURE_WIDTH,
                        SCREEN_BORDER / SCREEN_TEXTURE_HEIGHT, screen.z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
            }
            else if (kind == PICTURE_SCREEN_EXACT)
            {
                if (pictureInWorld[i] == 0)
                {
                    DAT_118d0ba8[screenTexture].DrawTinted(
                        (int)(picturePositions[i].x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(picturePositions[i].y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(offsetX + picturePositions[i].x + pictureSizes[i].x * 0.5f),
                        (int)(picturePositions[i].y + pictureSizes[i].y * 0.5f + offsetY),
                        (float)(HALF_TEXEL / SCREEN_TEXTURE_WIDTH_D), (float)(HALF_TEXEL / SCREEN_TEXTURE_HEIGHT_D),
                        (float)((screenWidth - HALF_TEXEL) / SCREEN_TEXTURE_WIDTH_D),
                        (float)((screenHeight - HALF_TEXEL) / SCREEN_TEXTURE_HEIGHT_D), picturePositions[i].z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
                else
                {
                    // The bottom edge keeps the 2-texel border of the other kinds here.
                    Vec4 screen;
                    screen.Copy(Project(picturePositions[i], view));
                    screen.x = screen.x * SCREEN_WIDTH / 2.0f + SCREEN_CENTRE_X;
                    screen.y = SCREEN_CENTRE_Y - screen.y * SCREEN_HEIGHT / 2.0f;
                    DAT_118d0ba8[screenTexture].DrawTinted(
                        (int)(screen.x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(screen.x + pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y + pictureSizes[i].y * 0.5f + offsetY),
                        (float)(HALF_TEXEL / SCREEN_TEXTURE_WIDTH_D), (float)(HALF_TEXEL / SCREEN_TEXTURE_HEIGHT_D),
                        (screenWidth - HALF_TEXEL) / SCREEN_TEXTURE_WIDTH_D,
                        (float)(screenHeight - SCREEN_BORDER) / SCREEN_TEXTURE_HEIGHT, screen.z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
            }
            else
            {
                if (pictureInWorld[i] == 0)
                {
                    DAT_118d0ba8[textures[i]].DrawTinted(
                        (int)(picturePositions[i].x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(picturePositions[i].y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(offsetX + picturePositions[i].x + pictureSizes[i].x * 0.5f),
                        (int)(picturePositions[i].y + pictureSizes[i].y * 0.5f + offsetY),
                        pictureUVs[i].x, pictureUVs[i].y, pictureUVs[i].z, pictureUVs[i].w, picturePositions[i].z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
                else
                {
                    Vec4 screen;
                    screen.Copy(Project(picturePositions[i], view));
                    screen.x = screen.x * SCREEN_WIDTH / 2.0f + SCREEN_CENTRE_X;
                    screen.y = SCREEN_CENTRE_Y - screen.y * SCREEN_HEIGHT / 2.0f;
                    DAT_118d0ba8[textures[i]].DrawTinted(
                        (int)(screen.x - pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y - pictureSizes[i].y * 0.5f + offsetY),
                        (int)(screen.x + pictureSizes[i].x * 0.5f + offsetX),
                        (int)(screen.y + pictureSizes[i].y * 0.5f + offsetY),
                        pictureUVs[i].x, pictureUVs[i].y, pictureUVs[i].z, pictureUVs[i].w, screen.z,
                        pictureAlpha[i], pictureAngle[i], pictureGrey[i]);
                }
            }
        }
    }
    FUN_1013f070(RENDER_BLEND_MODE, BLEND_ALPHA, 0);
    FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
    FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
}
