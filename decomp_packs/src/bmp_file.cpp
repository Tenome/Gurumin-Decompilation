// bmp_file.cpp : Packs.dll's image files. A Packs image file holds a magic, the picture size and one or two JPEG
// streams (or, for raw loads, uncompressed RGB data after them); SaveJpeg writes a plain JPEG file.
#include <stdlib.h>
#include "bmp_file.h"
#include "pack_file.h"
#include "jpeg_api.h"

// Copies width pixels of 3 bytes from one row to another, keeping the byte order. (EncodeJpegRgb)
inline void CopyRowRgb(unsigned char *to, unsigned char *from, int width)
{
    for (int x = 0; x < width; x++) {
        *to++ = from[0];
        *to++ = from[1];
        *to++ = from[2];
        from += 3;
    }
}

// Copies width pixels of 3 bytes from one row to another, swapping the first and last byte. (SaveJpeg)
inline void CopyRowSwapRb(unsigned char *to, unsigned char *from, int width)
{
    for (int x = 0; x < width; x++) {
        *to++ = from[2];
        *to++ = from[1];
        *to++ = from[0];
        from += 3;
    }
}

// Decodes the JPEG stream at the file position into dest, one row per stride bytes (width * 3 when stride is 0).
// Reports the picture size through width and height. Returns 1, or 0 when fp or dest is missing. (BMPLoad)
int DecodeJpegRgb(FILE *fp, unsigned char *dest, int stride, int *width, int *height, int reserved)
{
    jpeg_error_mgr errorManager;
    jpeg_decompress_struct cinfo;
    JSAMPROW row;
    unsigned char *rowDest;

    if (fp == 0)
        return 0;
    if (dest == 0)
        return 0;
    cinfo.err = jpeg_std_error(&errorManager);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, 1);
    jpeg_start_decompress(&cinfo);
    *width = cinfo.output_width;
    *height = cinfo.output_height;
    if (stride == 0)
        stride = cinfo.output_width * 3;
    rowDest = dest;
    while (cinfo.output_scanline < cinfo.output_height) {
        row = rowDest;
        jpeg_read_scanlines(&cinfo, &row, 1);
        rowDest += stride;
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return 1;
}

// Encodes width x height RGB pixels (rows padded to 4 bytes) as a JPEG stream on fp. Returns 1, or 0 when fp or
// src is missing. (BMPSave)
int EncodeJpegRgb(FILE *fp, unsigned char *src, int width, int height, int quality, int progressive)
{
    jpeg_error_mgr errorManager;
    jpeg_compress_struct cinfo;
    int srcStride;
    unsigned char *rowBuffer;
    JSAMPROW row;
    unsigned char *srcRow;

    if (fp == 0)
        return 0;
    if (src == 0)
        return 0;
    cinfo.err = jpeg_std_error(&errorManager);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, 1);
    if (progressive != 0)
        jpeg_simple_progression(&cinfo);
    jpeg_start_compress(&cinfo, 1);
    rowBuffer = (unsigned char *)malloc(width * 3);
    srcStride = (width * 3 + 3) / 4 * 4;
    srcRow = src;
    while (cinfo.next_scanline < cinfo.image_height) {
        CopyRowRgb(rowBuffer, srcRow, width);
        srcRow += srcStride;
        row = rowBuffer;
        jpeg_write_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    free(rowBuffer);
    return 1;
}

// Writes a plain JPEG file from BGR pixels (rows padded to 4 bytes; the red and blue bytes are swapped on the
// way in). Returns 1, or 0 when the file cannot be created or bgr is missing. (exported)
extern "C" int SaveJpeg(const char *path, unsigned char *bgr, int width, int height, int quality, int progressive)
{
    jpeg_error_mgr errorManager;
    jpeg_compress_struct cinfo;
    int srcStride;
    unsigned char *rowBuffer;
    JSAMPROW row;
    unsigned char *srcRow;
    FILE *fp = fopen(path, "wb");

    if (fp == 0)
        return 0;
    if (bgr == 0)
        return 0;
    cinfo.err = jpeg_std_error(&errorManager);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, 1);
    if (progressive != 0)
        jpeg_simple_progression(&cinfo);
    jpeg_start_compress(&cinfo, 1);
    rowBuffer = (unsigned char *)malloc(width * 3);
    srcStride = (width * 3 + 3) / 4 * 4;
    srcRow = bgr;
    while (cinfo.next_scanline < cinfo.image_height) {
        CopyRowSwapRb(rowBuffer, srcRow, width);
        srcRow += srcStride;
        row = rowBuffer;
        jpeg_write_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    free(rowBuffer);
    fclose(fp);
    return 1;
}

// Loads a Packs image file. image receives the first picture and second the optional second one. With rawPixels 0
// the pictures are decoded from their JPEG streams; otherwise the JPEG streams are skipped and uncompressed RGB
// pixels (width * height * 3 bytes each) are read from after them. Returns 1 on success, 0 when the file is
// missing or has the wrong magic. (exported)
int BMPLoad(const char *path, unsigned char *image, unsigned char *second, int rawPixels)
{
    PackFile file;
    int magic;
    int width;
    int height;
    int jpegWidth;
    int jpegHeight;
    int streamStart;
    int hasSecond;

    if (!file.Open(path, "rb"))
        return 0;
    file.Read(&magic, 1, 4);
    if (magic == PACK_IMAGE_MAGIC_SINGLE)
        hasSecond = 0;
    else if (magic == PACK_IMAGE_MAGIC_PAIR)
        hasSecond = 1;
    else {
        file.Close();
        return 0;
    }
    file.Read(&width, 1, 4);
    file.Read(&height, 1, 4);
    if (rawPixels != 0) {
        file.Read(&magic, 1, 4);
        streamStart = file.Tell();
        file.Seek(streamStart + magic, SEEK_SET);
        if (hasSecond) {
            file.Read(&magic, 1, 4);
            streamStart = file.Tell();
            file.Seek(streamStart + magic, SEEK_SET);
        }
        file.Read(image, 1, width * height * 3);
        if (hasSecond)
            file.Read(second, 1, width * height * 3);
    } else {
        file.Read(&magic, 1, 4);
        streamStart = file.Tell();
        DecodeJpegRgb(file.m_fp, image, 0, &jpegWidth, &jpegHeight, 0);
        file.Seek(streamStart + magic, SEEK_SET);
        if (hasSecond) {
            file.Read(&magic, 1, 4);
            DecodeJpegRgb(file.m_fp, second, 0, &jpegWidth, &jpegHeight, 0);
        }
    }
    file.Close();
    return 1;
}

// Saves a Packs image file: magic, width, height, then each picture as a size field plus JPEG stream (the size field is
// written as a placeholder first and patched once the stream length is known; streamSize first holds the stream
// start), then the raw RGB pixels of both
// pictures. second may be 0. Returns 1 on success, 0 when the file cannot be created. (exported)
int BMPSave(const char *path, unsigned char *image, unsigned char *second, int width, int height)
{
    PackFile file;
    int magic;
    int streamSize;
    int sizeFieldPos;
    int streamEnd;

    if (!file.Open(path, "wb"))
        return 0;
    magic = (second != 0) + PACK_IMAGE_MAGIC_SINGLE;
    file.Write(&magic, 1, 4);
    file.Write(&width, 1, 4);
    file.Write(&height, 1, 4);
    sizeFieldPos = file.Tell();
    file.Write(&magic, 1, 4);
    streamSize = file.Tell();
    EncodeJpegRgb(file.m_fp, image, width, height, PACK_IMAGE_JPEG_QUALITY, 0);
    streamEnd = file.Tell();
    streamSize = streamEnd - streamSize;
    file.Seek(sizeFieldPos, SEEK_SET);
    file.Write(&streamSize, 1, 4);
    file.Seek(streamEnd, SEEK_SET);
    if (second != 0) {
        sizeFieldPos = file.Tell();
        file.Write(&magic, 1, 4);
        streamSize = file.Tell();
        EncodeJpegRgb(file.m_fp, second, width, height, PACK_IMAGE_JPEG_QUALITY, 0);
        streamEnd = file.Tell();
        streamSize = streamEnd - streamSize;
        file.Seek(sizeFieldPos, SEEK_SET);
        file.Write(&streamSize, 1, 4);
        file.Seek(streamEnd, SEEK_SET);
    }
    file.Write(image, 1, width * height * 3);
    if (second != 0)
        file.Write(second, 1, width * height * 3);
    file.Close();
    return 1;
}
