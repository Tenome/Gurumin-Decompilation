// bmp_unpack.h : types and shared state of Packs.dll's BMP unpacker (BMPUnpack and its wavelet/arithmetic helpers).
#ifndef BMP_UNPACK_H
#define BMP_UNPACK_H

#include <stdio.h>

// The packed image is a wavelet pyramid: each level halves the plane (rounding up), at most this many levels.
enum { PACK_MAX_LEVELS = 8, PACK_LEVEL_SLOTS = PACK_MAX_LEVELS + 1 };

// The stream starts with a 24-byte header (little-endian fields, read byte by byte).
enum { PACK_HEADER_SIZE = 24 };

// Arithmetic coder: probabilities are 16-bit fixed point, the code value is 16 bits wide.
enum { ARITH_DEFAULT_BITS = 16, ARITH_PROB_ONE = 0x10000, ARITH_PROB_ROUND = 0x8000, ARITH_PROB_MAX = 0xfffe };

// Parameters of one wavelet band of one colour plane (4 bytes in the stream), indexed by these fields. A gain of 0
// means the band is all zero and the other three bytes are absent from the stream.
enum {
    RECORD_GAIN,          // adaptive model strength, raised to the 4th power to get the initial scale
    RECORD_ADAPT_WEIGHT,  // weight of the coded magnitude in the model update
    RECORD_DECAY_WEIGHT,  // weight of the old model value in the model update
    RECORD_BIAS,          // dequantisation bias, stored with +128
    BAND_RECORD_SIZE
};

// The three bands (right, below, diagonal) of one pyramid level, each with a record per colour plane:
// [band][plane][field].
typedef unsigned char LevelRecords[3][3][BAND_RECORD_SIZE];

// The part of the header BMPUnpack uses besides the sizes.
struct PackStreamInfo {
    unsigned int quantScale;  // dequantisation scale (65536 when the stream stores 0)
    int levels;               // number of pyramid levels, 0..PACK_MAX_LEVELS
};

// Arithmetic decoder state (shared with the packer's encoder).
extern int g_arithBits;                // width of the code value in bits (set to 16 by BMPUnpack)
extern unsigned int g_arithHigh;       // upper bound of the current interval
extern unsigned int g_arithLow;        // lower bound of the current interval
extern unsigned int g_arithValue;      // the code value read from the stream
extern unsigned int g_arithQuarter;    // 1 << (bits - 2)
extern unsigned int g_arithHalf;       // 2 * quarter
extern unsigned int g_arithThreeQuarter;
extern unsigned int g_arithFull;       // 4 * quarter
extern unsigned int g_arithTop;        // full - 1
extern int g_arithBytePos;             // position of the current input byte
extern unsigned char g_arithByte;      // the current input byte
extern unsigned char g_arithBitMask;   // bit of the current byte being read next
extern int g_arithBitsRead;            // bits consumed after the initial code value

// The order in which the 256 coefficients of a 16x16 block are coded (x and y offset of each).
extern int g_blockScanX[256];
extern int g_blockScanY[256];

// MMV3 commands (the first argument is otherwise a pointer to the file name to open).
enum { MMV3_CONTINUE = 0, MMV3_CLOSE = 1, MMV3_NEXT_FRAME = 2 };

// The movie container MMV3 has open (shared state of its calls).
extern FILE *g_mmvFile;

int ReadStreamHeader(PackStreamInfo *info, int *widths, int *heights, unsigned int *flags, LevelRecords *records,
                     const unsigned char *data, int *position);
short DecodeSignedValue(const unsigned char *data, unsigned short probability);
void DecodeBand(const unsigned char *data, short *band, int width, int height, int stride, const unsigned char *record);
void ToRgbBytes(const short *plane0, const short *plane1, const short *plane2, const int *widths, const int *heights,
                unsigned char *out, int *outPosition);

void DequantizeBand(short *plane0, short *plane1, short *plane2, int width, int height, int stride, int quantScale,
                    int levelScale, int bias0, int bias1, int bias2);
void DequantizeLevels(short *plane0, short *plane1, short *plane2, const int *widths, const int *heights,
                      int quantScale, int levels, const LevelRecords *records);
void DecodeLevels(const unsigned char *data, short *plane0, short *plane1, short *plane2, const int *widths,
                  const int *heights, int levels, const LevelRecords *records);
void InverseWaveletLevel(short *plane, short *scratch, int width, int height, int stride);

extern "C" int BMPUnpack(const unsigned char *data, int unused, unsigned char *out, int *outWidth, int *outHeight);
extern "C" int MMV3(int command, unsigned int *pixels, int unused, int *width, unsigned int *height);

#endif
