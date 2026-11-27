// bmp_unpack.cpp : BMPUnpack and MMV3, Packs.dll's image unpackers (wavelet pyramid with an arithmetic coder).
#include "bmp_unpack.h"
#include "jpeg_api.h"
#include <windows.h>
#include <new.h>
#include <stdlib.h>

// Combines four stream bytes (least significant first) into a 32-bit value.
#define LE32(b0, b1, b2, b3) ((((((b3) << 8) + (b2)) << 8) + (b1)) << 8) + (b0)

// Byte offsets of the 24-byte stream header's fields (little-endian).
enum { HDR_QUANT_SCALE = 6, HDR_WIDTH = 8, HDR_HEIGHT = 12, HDR_LEVELS = 16, HDR_FLAGS = 20 };

// Reads the stream header, derives the plane size of every pyramid level and reads the band records.
// Returns the number of record bytes consumed. (BMPUnpack)
int ReadStreamHeader(PackStreamInfo *info, int *widths, int *heights, unsigned int *flags, LevelRecords *records,
                     const unsigned char *data, int *position)
{
    unsigned char header[PACK_HEADER_SIZE];
    int i;
    for (i = 0; i < PACK_HEADER_SIZE; i++)
        header[i] = data[(*position)++];

    unsigned int quantScale = (header[HDR_QUANT_SCALE + 1] << 8) + header[HDR_QUANT_SCALE];
    if (quantScale == 0)
        quantScale = ARITH_PROB_ONE;

    widths[0] = LE32(header[HDR_WIDTH], header[HDR_WIDTH + 1], header[HDR_WIDTH + 2], header[HDR_WIDTH + 3]);
    heights[0] = LE32(header[HDR_HEIGHT], header[HDR_HEIGHT + 1], header[HDR_HEIGHT + 2], header[HDR_HEIGHT + 3]);
    for (i = 1; i < PACK_LEVEL_SLOTS; i++) {
        widths[i] = (widths[i - 1] + 1) / 2;
        heights[i] = (heights[i - 1] + 1) / 2;
    }

    int levels = header[HDR_LEVELS] + (header[HDR_LEVELS + 1] << 8);
    info->quantScale = quantScale;
    int consumed = 0;
    info->levels = levels;
    unsigned int flagBits = LE32(header[HDR_FLAGS], header[HDR_FLAGS + 1], header[HDR_FLAGS + 2], header[HDR_FLAGS + 3]);
    *flags = flagBits;

    int recordCount = levels * 9;
    unsigned char *record = records[0][0][0];
    while (recordCount > 0) {
        recordCount--;
        unsigned char gain = data[*position];
        (*position)++;
        record[RECORD_GAIN] = gain;
        consumed++;
        if (gain != 0) {
            record[RECORD_ADAPT_WEIGHT] = data[*position];
            record[RECORD_DECAY_WEIGHT] = data[++*position];
            record[RECORD_BIAS] = data[++*position];
            (*position)++;
            consumed += 3;
        } else {
            record[RECORD_ADAPT_WEIGHT] = 0;
            record[RECORD_DECAY_WEIGHT] = 0;
            record[RECORD_BIAS] = 0;
        }
        record += BAND_RECORD_SIZE;
    }
    return consumed;
}

// Shifts the interval and reads the next bit of the stream until the interval is narrow again.
// (DecodeSignedValue, after each magnitude decision)
static __forceinline void RenormalizeDecoder(const unsigned char *data)
{
    for (;;) {
        if (g_arithHigh >= g_arithHalf && g_arithLow < g_arithHalf) {
            if (g_arithLow < g_arithQuarter || g_arithHigh >= g_arithThreeQuarter)
                break;
            g_arithValue -= g_arithQuarter;
            g_arithLow -= g_arithQuarter;
            g_arithHigh -= g_arithQuarter;
        }
        g_arithLow <<= 1;
        if (g_arithLow >= g_arithFull)
            g_arithLow -= g_arithFull;
        g_arithHigh <<= 1;
        if (g_arithHigh >= g_arithFull)
            g_arithHigh -= g_arithFull;
        g_arithHigh++;
        g_arithValue <<= 1;
        if (g_arithValue >= g_arithFull)
            g_arithValue -= g_arithFull;
        if (g_arithByte & g_arithBitMask)
            g_arithValue++;
        if (g_arithBitMask != 0x80) {
            g_arithBitMask <<= 1;
        } else {
            g_arithBytePos++;
            g_arithBitMask = 1;
            g_arithByte = data[g_arithBytePos];
        }
        g_arithBitsRead++;
    }
}

// Shifts the interval and reads the next bit of the stream until the interval is narrow again.
// (DecodeSignedValue, after the sign decision)
static __forceinline void RenormalizeAfterSign(const unsigned char *data)
{
    for (;;) {
        if (g_arithHigh >= g_arithHalf && g_arithLow < g_arithHalf) {
            if (g_arithLow < g_arithQuarter || g_arithHigh >= g_arithThreeQuarter)
                break;
            g_arithValue -= g_arithQuarter;
            g_arithLow -= g_arithQuarter;
            g_arithHigh -= g_arithQuarter;
        }
        g_arithLow <<= 1;
        if (g_arithLow >= g_arithFull)
            g_arithLow -= g_arithFull;
        g_arithHigh <<= 1;
        if (g_arithHigh >= g_arithFull)
            g_arithHigh -= g_arithFull;
        g_arithHigh++;
        g_arithValue <<= 1;
        if (g_arithValue >= g_arithFull)
            g_arithValue -= g_arithFull;
        if (g_arithByte & g_arithBitMask)
            g_arithValue++;
        if (g_arithBitMask != 0x80) {
            g_arithBitMask <<= 1;
        } else {
            g_arithBitMask = 1;
            g_arithBytePos++;
            g_arithByte = data[g_arithBytePos];
        }
        g_arithBitsRead++;
    }
}

// Decodes one signed coefficient: a unary magnitude (each step is one arithmetic decision with the given
// probability of "zero") followed by the sign. (DecodeBand)
short DecodeSignedValue(const unsigned char *data, unsigned short probability)
{
    short value = 0;
    int more;
    do {
        unsigned int split = (((g_arithHigh - g_arithLow + 1) * probability + ARITH_PROB_ROUND) >> 16) + g_arithLow;
        if (g_arithValue >= split) {
            g_arithLow = split;
            value++;
            more = 1;
        } else {
            g_arithHigh = split - 1;
            more = 0;
        }
        RenormalizeDecoder(data);
    } while (more);

    if (value != 0) {
        unsigned int split = ((g_arithHigh - g_arithLow + 2) >> 1) + g_arithLow;
        if (g_arithValue >= split) {
            g_arithLow = split;
            value = -value;
        } else {
            g_arithHigh = split - 1;
        }
        RenormalizeAfterSign(data);
    }
    return value;
}

// Turns the model scale into the 16-bit probability for the next arithmetic decision. (DecodeBand)
static inline unsigned int ProbabilityFromScale(unsigned int scale)
{
    unsigned int probability = (((scale >> 1) - scale) - ARITH_PROB_ROUND) / (scale + ARITH_PROB_ONE) + 1;
    if (probability > ARITH_PROB_MAX)
        return ARITH_PROB_MAX;
    return probability;
}

// Decodes one wavelet band of width x height coefficients into a plane with the given stride. The band is coded in
// 16x16 blocks, each in scan order; an adaptive model turns the running magnitude into the probability for the next
// coefficient. (UnpackBands)
void DecodeBand(const unsigned char *data, short *band, int width, int height, int stride, const unsigned char *record)
{
    unsigned int gain = record[RECORD_GAIN];
    if (gain == 0) {
        for (int y = 0; y < height; y++) {
            short *row = band + y * stride;
            for (int x = 0; x < width; x++)
                row[x] = 0;
        }
        return;
    }

    unsigned int adaptWeight = record[RECORD_ADAPT_WEIGHT];
    unsigned int decayWeight = record[RECORD_DECAY_WEIGHT];
    unsigned int scale = gain * gain * gain * gain;
    int keep = 0x100 - adaptWeight;
    int weightInitial = (0x100 - decayWeight) * keep;
    unsigned int initialShare = (((scale & 0xffff) * weightInitial + ARITH_PROB_ROUND) >> 16) + (scale >> 16) * weightInitial;
    int weightCurrent = keep * decayWeight;
    int adaptStep = adaptWeight << 8;  // how much one unit of coefficient magnitude raises the scale

    int startPos = g_arithBytePos;
    g_arithBitsRead = 0;
    g_arithBitMask = 1;
    g_arithByte = data[g_arithBytePos];
    g_arithValue = 0;
    for (int bit = g_arithBits; bit > 0; bit--) {
        int bitSet = (g_arithByte & g_arithBitMask) != 0;
        if (g_arithBitMask != 0x80) {
            g_arithBitMask <<= 1;
        } else {
            g_arithBitMask = 1;
            g_arithBytePos++;
            g_arithByte = data[g_arithBytePos];
        }
        g_arithValue = bitSet + g_arithValue * 2;
    }
    g_arithHigh = g_arithTop;
    g_arithLow = 0;

    unsigned int probability = ProbabilityFromScale(scale);
    for (int blockY = 0; blockY < height; blockY += 16) {
        for (int blockX = 0; blockX < width; blockX += 16) {
            for (int i = 0; i < 256; i++) {
                int dx = g_blockScanX[i];
                if (blockX + dx < width) {
                    if (blockY + g_blockScanY[i] < height) {
                        short coefficient = DecodeSignedValue(data, (unsigned short)probability);
                        band[(blockY + g_blockScanY[i]) * stride + dx + blockX] = coefficient;
                        short magnitude = coefficient < 0 ? -coefficient : coefficient;
                        scale = (scale >> 16) * weightCurrent + initialShare +
                                (((scale & 0xffff) * weightCurrent + ARITH_PROB_ROUND) >> 16);
                        if (magnitude != 0)
                            scale += magnitude * adaptStep;
                        if (scale == 0)
                            scale = 1;
                        probability = ProbabilityFromScale(scale);
                    }
                }
            }
        }
    }
    g_arithBytePos = ((unsigned int)(g_arithBits + 8 + g_arithBitsRead) >> 3) + startPos;
}

// Window a reconstructed sample is clamped to before it becomes a byte (bits 7..14 of the clamped value).
enum { SAMPLE_CLAMP_LOW = 0x10000, SAMPLE_CLAMP_HIGH = 0x17fff, SAMPLE_BIAS = 0x10040 };

// Clamps a reconstructed sample and returns it as a byte. (ToRgbBytes)
static inline unsigned char SampleToByte(unsigned int sample)
{
    if (sample > SAMPLE_CLAMP_LOW) {
        if (sample >= SAMPLE_CLAMP_HIGH)
            sample = SAMPLE_CLAMP_HIGH;
    } else {
        sample = SAMPLE_CLAMP_LOW;
    }
    return (unsigned char)(sample >> 7);
}

// Converts the three reconstructed planes (a lifting-style colour transform) to bytes, three per pixel, written to
// out at *outPosition, which this function advances (and starts at 0). (BMPUnpack)
void ToRgbBytes(const short *plane0, const short *plane1, const short *plane2, const int *widths, const int *heights,
                unsigned char *out, int *outPosition)
{
    *outPosition = 0;
    for (int y = 0; y < heights[0]; y++) {
        int rowStart = widths[0] * y;
        for (int x = 0; x < widths[0]; x++) {
            short v0 = plane0[rowStart + x];
            short v2 = plane2[rowStart + x];
            short v1 = plane1[rowStart + x];
            unsigned int first = (v2 * -3 + v0) * 2 + SAMPLE_BIAS + v1 * -3;
            unsigned int second = (v1 + v0) * 2 + SAMPLE_BIAS;
            unsigned int third = (v0 + v2) * 2 + SAMPLE_BIAS + v1 * -3;
            out[(*outPosition)++] = SampleToByte(first);
            out[(*outPosition)++] = SampleToByte(second);
            out[(*outPosition)++] = SampleToByte(third);
        }
    }
}

// Largest magnitude a dequantised coefficient may take.
enum { COEFFICIENT_LIMIT = 0x7fff };

// Dequantises one coefficient: scales the magnitude by the level scale (as integer part and rounded remainder
// term) and adds the band bias away from zero. (DequantizeBand)
static inline int DequantizeSample(int value, int wholePart, int remainderPart, int half, int divisor, int bias)
{
    int result = value;
    if (value != 0) {
        if (value > 0)
            result = value * wholePart + (value * remainderPart + half + bias) / divisor;
        else
            result = value * wholePart - ((half - value * remainderPart) + bias) / divisor;
    }
    if (result <= -COEFFICIENT_LIMIT)
        result = -COEFFICIENT_LIMIT;
    else if (result >= COEFFICIENT_LIMIT)
        result = COEFFICIENT_LIMIT;
    return result;
}

// Dequantises a width x height region of the three planes in place; the same sample index is used in every plane.
// quantScale and levelScale come from the stream header and the pyramid level, each plane has its own bias.
// (DequantizeLevels)
void DequantizeBand(short *plane0, short *plane1, short *plane2, int width, int height, int stride, int quantScale,
                    int levelScale, int bias0, int bias1, int bias2)
{
    int scaledLevel = levelScale << 8;  // level scale in 8.8 fixed point
    int divisor = quantScale * 16;
    int half = (divisor - 1) >> 1;
    int wholePart = scaledLevel / divisor - 1;
    int remainderPart = scaledLevel % divisor + divisor;
    int rowStart = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = rowStart + x;
            plane0[index] = (short)DequantizeSample(plane0[index], wholePart, remainderPart, half, divisor, bias0);
            plane1[index] = (short)DequantizeSample(plane1[index], wholePart, remainderPart, half, divisor, bias1);
            plane2[index] = (short)DequantizeSample(plane2[index], wholePart, remainderPart, half, divisor, bias2);
        }
        rowStart += stride;
    }
}

// Bias stored in a band record is offset by 128.
enum { BAND_BIAS_ZERO = 0x80 };

// Dequantises every band of every pyramid level, coarsest level last. The level scale halves per level but never
// drops below the stream's quantisation scale. Each level has three bands: right of, below and diagonal to the
// next coarser level. (BMPUnpack)
void DequantizeLevels(short *plane0, short *plane1, short *plane2, const int *widths, const int *heights,
                      int quantScale, int levels, const LevelRecords *records)
{
    int levelScale[PACK_LEVEL_SLOTS];
    levelScale[0] = ARITH_PROB_ONE;
    for (int i = 0; i < PACK_MAX_LEVELS; i++) {
        levelScale[i + 1] = levelScale[i] >> 1;
        if (levelScale[i] >> 1 < quantScale)
            levelScale[i + 1] = quantScale;
    }

    int stride = widths[0];
    const LevelRecords *rec = records;
    for (int level = levels; level > 0; level--, rec++) {
        int scale = levelScale[level];

        int offset = widths[level];
        DequantizeBand(plane0 + offset, plane1 + offset, plane2 + offset, widths[level - 1] - widths[level],
                       heights[level], stride, quantScale, scale, ((*rec)[0][0][RECORD_BIAS] - BAND_BIAS_ZERO) * scale,
                       ((*rec)[0][1][RECORD_BIAS] - BAND_BIAS_ZERO) * scale, ((*rec)[0][2][RECORD_BIAS] - BAND_BIAS_ZERO) * scale);

        offset = heights[level] * stride;
        DequantizeBand(plane0 + offset, plane1 + offset, plane2 + offset, widths[level],
                       heights[level - 1] - heights[level], stride, quantScale, scale,
                       ((*rec)[1][0][RECORD_BIAS] - BAND_BIAS_ZERO) * scale, ((*rec)[1][1][RECORD_BIAS] - BAND_BIAS_ZERO) * scale,
                       ((*rec)[1][2][RECORD_BIAS] - BAND_BIAS_ZERO) * scale);

        offset = heights[level] * stride + widths[level];
        DequantizeBand(plane0 + offset, plane1 + offset, plane2 + offset, widths[level - 1] - widths[level],
                       heights[level - 1] - heights[level], stride, quantScale, scale,
                       ((*rec)[2][0][RECORD_BIAS] - BAND_BIAS_ZERO) * scale, ((*rec)[2][1][RECORD_BIAS] - BAND_BIAS_ZERO) * scale,
                       ((*rec)[2][2][RECORD_BIAS] - BAND_BIAS_ZERO) * scale);
    }
}

// Decodes every band of every level from the arithmetic-coded stream, coarsest level last, and sets the coder up
// first (interval constants from the code width). (BMPUnpack)
void DecodeLevels(const unsigned char *data, short *plane0, short *plane1, short *plane2, const int *widths,
                  const int *heights, int levels, const LevelRecords *records)
{
    g_arithQuarter = 1 << (g_arithBits - 2);
    g_arithThreeQuarter = g_arithQuarter * 3;
    g_arithHalf = g_arithQuarter * 2;
    g_arithFull = g_arithQuarter * 4;
    g_arithTop = g_arithFull - 1;
    int stride = widths[0];
    g_arithBytePos = 0;
    const LevelRecords *rec = records;
    for (int level = levels; level > 0; level--, rec++) {
        int offset = widths[level];
        int bandWidth = widths[level - 1] - widths[level];
        int bandHeight = heights[level];
        DecodeBand(data, plane0 + offset, bandWidth, bandHeight, stride, (*rec)[0][0]);
        DecodeBand(data, plane1 + offset, bandWidth, bandHeight, stride, (*rec)[0][1]);
        DecodeBand(data, plane2 + offset, bandWidth, bandHeight, stride, (*rec)[0][2]);

        offset = heights[level] * stride;
        bandWidth = widths[level];
        bandHeight = heights[level - 1] - heights[level];
        DecodeBand(data, plane0 + offset, bandWidth, bandHeight, stride, (*rec)[1][0]);
        DecodeBand(data, plane1 + offset, bandWidth, bandHeight, stride, (*rec)[1][1]);
        DecodeBand(data, plane2 + offset, bandWidth, bandHeight, stride, (*rec)[1][2]);

        offset = heights[level] * stride + widths[level];
        bandWidth = widths[level - 1] - widths[level];
        bandHeight = heights[level - 1] - heights[level];
        DecodeBand(data, plane0 + offset, bandWidth, bandHeight, stride, (*rec)[2][0]);
        DecodeBand(data, plane1 + offset, bandWidth, bandHeight, stride, (*rec)[2][1]);
        DecodeBand(data, plane2 + offset, bandWidth, bandHeight, stride, (*rec)[2][2]);
    }
}

// Packs the RGB pixel at rgb (three bytes, red first) into the 32-bit word MMV3 stores per pixel pair: 5 bits red,
// 6 bits green, 5 bits blue, repeated across the two halves of the word. The masks ~7 and ~3 drop the low 3 bits
// of red and blue and the low 2 of green; the green/blue term is repeated for the upper half. (MMV3)
static inline unsigned int PackPixelPair(const unsigned char *rgb)
{
    return ((((rgb[1] & ~3) | ((rgb[2] & ~7) << 5)) << 6 | (rgb[0] & ~7)) << 10 | ((((rgb[2] & ~7) << 5) | rgb[1]) & ~3)) << 3 | (rgb[0] >> 3);
}

// Reads Gurumin's movie-frame container (MMV3 is the format's tag). command is MMV3_CLOSE, MMV3_NEXT_FRAME,
// MMV3_CONTINUE or a pointer to the file name to open: the container is a chain of blocks, each starting with
// the offset of the next one, and a JPEG frame is decoded from the block at the file position into pixels
// (one packed 32-bit word per pixel pair, rows overlapping the JPEG scanline in place). Returns 1 when a frame was
// decoded or the position moved, else 0. (exported, called by the game)
extern "C" int MMV3(int command, unsigned int *pixels, int unused, int *width, unsigned int *height)
{
    int nextBlock;
    if (command == MMV3_CLOSE) {
        if (g_mmvFile != 0)
            fclose(g_mmvFile);
        return 0;
    }
    if (command == MMV3_NEXT_FRAME) {
        int blockOffset;  // offset of the next block, read from the file position
        fread(&blockOffset, 1, 4, g_mmvFile);
        if (blockOffset == 0)
            return 0;
        fseek(g_mmvFile, blockOffset, SEEK_SET);
        return 1;
    }
    if (command != MMV3_CONTINUE) {
        int blockOffset;  // the two header words skipped after opening
        if (g_mmvFile != 0)
            fclose(g_mmvFile);
        g_mmvFile = fopen((const char *)command, "rb");
        fread(&blockOffset, 1, 4, g_mmvFile);
        fread(&blockOffset, 1, 4, g_mmvFile);
    }
    if (g_mmvFile == 0)
        return 0;
    fread(&nextBlock, 1, 4, g_mmvFile);
    if (nextBlock == 0)
        return 0;
    if (pixels == 0)
        return 0;

    jpeg_error_mgr errorManager;
    jpeg_decompress_struct cinfo;
    cinfo.err = jpeg_std_error(&errorManager);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, g_mmvFile);
    jpeg_read_header(&cinfo, 1);
    jpeg_start_decompress(&cinfo);
    unsigned int *rowStart = pixels;  // start of the row being converted; the JPEG scanline lands here first
    int outputWidth = cinfo.output_width;
    *width = outputWidth;
    *height = cinfo.output_height;
    int rowBytes = outputWidth * 2;
    int pairsPerRow = outputWidth / 2;
    JSAMPROW scanline;  // row buffer handed to the JPEG library, one scanline at a time
    while (cinfo.output_scanline < cinfo.output_height) {
        scanline = (JSAMPROW)rowStart;
        jpeg_read_scanlines(&cinfo, &scanline, 1);
        if (pairsPerRow > 0) {
            unsigned int *out = rowStart;
            const unsigned char *rgb = (const unsigned char *)rowStart;
            for (int pairs = pairsPerRow; pairs > 0; pairs--) {
                *out++ = PackPixelPair(rgb);
                rgb += 6;
            }
        }
        rowStart = (unsigned int *)((char *)rowStart + rowBytes);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fseek(g_mmvFile, nextBlock, SEEK_SET);
    return 1;
}

// Stores base - sum / 2 (rounded toward zero) at dst[index]. (InverseWaveletLevel)
static inline void StoreSubtractHalf(short *dst, int index, short base, int sum)
{
    if (sum == 0)
        dst[index] = base;
    else if (sum > 0)
        dst[index] = base - ((unsigned short)sum >> 1);
    else if (sum < 0)
        dst[index] = base + ((unsigned short)-(unsigned short)sum >> 1);
}

// Stores base + sum / 2 (rounded toward zero) at dst[index]. (InverseWaveletLevel)
static inline void StoreAddHalf(short *dst, int index, short base, int sum)
{
    if (sum == 0)
        dst[index] = base;
    else if (sum > 0)
        dst[index] = ((unsigned short)sum >> 1) + base;
    else if (sum < 0)
        dst[index] = base - ((unsigned short)-(unsigned short)sum >> 1);
}

// Inverse of one wavelet level in both directions: undoes the horizontal then the vertical lifting steps on the
// width x height region of plane, using scratch as the intermediate image (same stride). In each line the first
// (length + 1) / 2 samples are the low band and the rest the high band; the result is interleaved (even = low minus
// half the neighbouring highs, odd = high plus half the neighbouring lows). The last two pairs of a line clamp their
// neighbours to the line. (BMPUnpack)
void InverseWaveletLevel(short *plane, short *scratch, int width, int height, int stride)
{
    int halfWidth = (width + 1) / 2;
    int halfHeight = (height + 1) / 2;

    // Horizontal pass: plane rows into scratch rows.
    int rowStart = 0;
    int highStart = halfWidth;
    for (int y = 0; y < height; y++) {
        int lowNext = rowStart + 1;
        int highNext = highStart;
        int out = rowStart;
        short low = plane[rowStart];
        short high = plane[highStart];
        short prevLow = low;
        short prevHigh = high;
        int i;
        for (i = 0; i < halfWidth - 2; i++) {
            low = plane[lowNext++];
            high = plane[highNext++];
            StoreSubtractHalf(scratch, out++, prevLow, prevHigh + high);
            StoreAddHalf(scratch, out++, high, prevLow + low);
            prevLow = low;
            prevHigh = high;
        }
        for (i = halfWidth - 2; i < halfWidth; i++) {
            short nextLow = plane[lowNext];
            if (i + 1 < halfWidth - 1)
                lowNext++;
            short nextHigh = plane[highNext];
            if (halfWidth + i < width - 1)
                highNext++;
            StoreSubtractHalf(scratch, out++, low, prevHigh + nextHigh);
            if (out < (width - halfWidth) + highStart)
                StoreAddHalf(scratch, out++, nextHigh, low + nextLow);
            low = nextLow;
            prevHigh = nextHigh;
        }
        rowStart += stride;
        highStart += stride;
    }

    // Vertical pass: scratch columns back into plane columns.
    for (int x = 0; x < width; x++) {
        int lowNext = x + stride;
        int highNext = halfHeight * stride + x;
        int out = x;
        short low = scratch[x];
        short high = scratch[halfHeight * stride + x];
        short prevLow = low;
        short prevHigh = high;
        int i;
        for (i = 0; i < halfHeight - 2; i++) {
            low = scratch[lowNext];
            lowNext += stride;
            high = scratch[highNext];
            highNext += stride;
            StoreSubtractHalf(plane, out, prevLow, prevHigh + high);
            out += stride;
            StoreAddHalf(plane, out, high, prevLow + low);
            out += stride;
            prevLow = low;
            prevHigh = high;
        }
        for (i = halfHeight - 2; i < halfHeight; i++) {
            short nextLow = scratch[lowNext];
            if (i + 1 < halfHeight - 1)
                lowNext += stride;
            short nextHigh = scratch[highNext];
            if (halfHeight + i < height - 1)
                highNext += stride;
            StoreSubtractHalf(plane, out, low, prevHigh + nextHigh);
            out += stride;
            if (out < height * stride + x) {
                StoreAddHalf(plane, out, nextHigh, low + nextLow);
                out += stride;
            }
            low = nextLow;
            prevHigh = nextHigh;
        }
    }
}

// Unpacks a packed image: header, the raw coarsest level, the arithmetic-coded detail bands, dequantisation, the
// inverse wavelet levels and the colour conversion to bytes (three per pixel) in out. data is the packed stream,
// the width and height come back in *outWidth and *outHeight. Returns 1 on success, 0 when memory ran out.
// (exported, called by the game; the GetTickCount calls are the leftovers of the original's timing code)
extern "C" int BMPUnpack(const unsigned char *data, int unused, unsigned char *out, int *outWidth, int *outHeight)
{
    PackStreamInfo info;
    int widths[PACK_LEVEL_SLOTS];
    int heights[PACK_LEVEL_SLOTS];
    unsigned int flags;
    LevelRecords records[PACK_MAX_LEVELS];
    int position;
    int outPosition;

    g_arithBits = ARITH_DEFAULT_BITS;
    position = 0;
    for (int pass = 1; pass < 2; pass++) {
        ReadStreamHeader(&info, widths, heights, &flags, records, data, &position);
        int pixels = widths[0] * heights[0];
        short *plane0 = (short *)operator new(pixels * 8);
        if (plane0 == 0)
            return 0;
        short *plane1 = plane0 + pixels;
        short *plane2 = plane0 + pixels * 2;
        short *scratch = plane0 + pixels * 3;

        if (heights[info.levels] > 0) {
            int coarseWidth = widths[info.levels];
            int coarseHeight = heights[info.levels];
            short *row0 = plane0;
            short *row1 = plane1;
            short *row2 = plane2;
            for (int y = 0; y < coarseHeight; y++) {
                for (int x = 0; x < coarseWidth; x++) {
                    unsigned char b0 = data[position++];
                    unsigned char b1 = data[position++];
                    unsigned char b2 = data[position++];
                    unsigned char b3 = data[position++];
                    row0[x] = (((b1 & 7) - 2) * 0x100 + b0) * 16;
                    row1[x] = (((b2 & 0x3f) - 0x20) * 0x20 + (b1 >> 3)) * 16;
                    row2[x] = (((b2 >> 6) - 0x200) + b3 * 4) * 16;
                }
                row0 += widths[0];
                row1 += widths[0];
                row2 += widths[0];
            }
        }
        GetTickCount();
        DecodeLevels(data + position, plane0, plane1, plane2, widths, heights, info.levels, records);
        GetTickCount();
        GetTickCount();
        DequantizeLevels(plane0, plane1, plane2, widths, heights, info.quantScale, info.levels, records);
        GetTickCount();
        GetTickCount();
        for (int level = info.levels - 1; level >= 0; level--)
            InverseWaveletLevel(plane0, scratch, widths[level], heights[level], widths[0]);
        for (int level = info.levels - 1; level >= 0; level--)
            InverseWaveletLevel(plane1, scratch, widths[level], heights[level], widths[0]);
        for (int level = info.levels - 1; level >= 0; level--)
            InverseWaveletLevel(plane2, scratch, widths[level], heights[level], widths[0]);
        GetTickCount();
        GetTickCount();
        outPosition = 0;
        ToRgbBytes(plane0, plane1, plane2, widths, heights, out, &outPosition);
        GetTickCount();
        *outWidth = widths[0];
        *outHeight = heights[0];
        free(plane0);
    }
    return 1;
}
