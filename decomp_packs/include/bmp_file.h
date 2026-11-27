// bmp_file.h : Packs.dll's image file functions (.bmp pairs stored as JPEG data, and plain JPEG files).
#ifndef BMP_FILE_H
#define BMP_FILE_H

#include <stdio.h>

// File magic of a Packs image file: one picture, or a picture plus a second (alpha) picture.
const int PACK_IMAGE_MAGIC_SINGLE = 0x378;
const int PACK_IMAGE_MAGIC_PAIR = 0x379;

// JPEG quality BMPSave asks for (libjpeg clamps it to 100).
const int PACK_IMAGE_JPEG_QUALITY = 0x80;

// Decodes a JPEG stream into an RGB buffer. stride 0 means width * 3.
int DecodeJpegRgb(FILE *fp, unsigned char *dest, int stride, int *width, int *height, int reserved);

// Encodes an RGB buffer (rows padded to 4 bytes) as a JPEG stream.
int EncodeJpegRgb(FILE *fp, unsigned char *src, int width, int height, int quality, int progressive);

// Exports.
int BMPLoad(const char *path, unsigned char *image, unsigned char *second, int rawPixels);
int BMPSave(const char *path, unsigned char *image, unsigned char *second, int width, int height);
extern "C" int SaveJpeg(const char *path, unsigned char *bgr, int width, int height, int quality, int progressive);

#endif
