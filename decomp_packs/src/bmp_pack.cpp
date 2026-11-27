// bmp_pack.cpp : BMPPack, the BMP compressor of Packs.dll, and its helpers.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_pack.h"

// ---- run-length coding of quantised coefficients ----

// Stores one run/level pair: the zero run in the low nibble of the word and the level in the high twelve bits
// (positive levels as is, negative levels as 0x800 - level, saturated to +-0x7ff). (PackBlocks)
void StoreCoefficient(short *out, short zeroRun, short value)
{
    out[g_rleLength] = zeroRun;
    if (value > COEFFICIENT_LIMIT) {
        value = COEFFICIENT_LIMIT;
    } else if (value < -COEFFICIENT_LIMIT) {
        value = -COEFFICIENT_LIMIT;
    }
    if (value >= 0)
        out[g_rleLength] += value * LEVEL_UNIT;
    else
        out[g_rleLength] += (NEGATIVE_LEVEL_BASE - value) * LEVEL_UNIT;
    g_rleLength++;
}

// Scans one sub-band in 16x16 blocks (cells in g_scanColumn/g_scanRow order) and stores a run/level pair for every
// non-zero cell or full run of fifteen zeros; a trailing run is stored as its length minus one. (PackLevels)
void PackBlocks(const short *plane, short *out, int width, int height, int stride)
{
    int run = 0;
    for (int blockRow = 0; blockRow < height; blockRow += SCAN_BLOCK_SIZE) {
        for (int blockColumn = 0; blockColumn < width; blockColumn += SCAN_BLOCK_SIZE) {
            for (int cell = 0; cell < SCAN_BLOCK_CELLS; cell++) {
                if (g_scanColumn[cell] + blockColumn < width && g_scanRow[cell] + blockRow < height) {
                    short value = plane[(g_scanRow[cell] + blockRow) * stride + g_scanColumn[cell] + blockColumn];
                    if (value == 0 && run != 15) {
                        run++;
                    } else {
                        StoreCoefficient(out, run, value);
                        run = 0;
                    }
                }
            }
        }
    }
    if (run != 0) {
        out[g_rleLength] = run - 1;
        g_rleLength++;
    }
}

// Run-length codes the three detail sub-bands (right, bottom and diagonal of the previous level's size) of every
// level, finest level first. widths/heights hold the plane size at each level, level 0 being the full plane.
// (BMPPack)
void PackLevels(const short *plane, short *out, const int *widths, const int *heights, int levels)
{
    g_rleLength = 0;
    int stride = widths[0];
    for (int level = levels; level >= 1; level--) {
        PackBlocks(plane + widths[level], out, widths[level - 1] - widths[level], heights[level], stride);
        PackBlocks(plane + heights[level] * stride, out, widths[level], heights[level - 1] - heights[level], stride);
        PackBlocks(plane + heights[level] * stride + widths[level], out, widths[level - 1] - widths[level],
                   heights[level - 1] - heights[level], stride);
    }
}

// ---- arithmetic coder ----

// Appends one bit to the output byte stream, least significant bit first, flushing a byte when the mask reaches
// 0x80; then writes the pending underflow bits as the opposite of bit. (EncodeBit, EncodeBand)
void EmitBit(int bit, unsigned char *out)
{
    if (bit != 0)
        g_outputByte |= g_outputBitMask;
    if (g_outputBitMask != 0x80) {
        g_outputBitMask <<= 1;
    } else {
        out[g_outputLength] = g_outputByte;
        g_outputByte = 0;
        g_outputBitMask = 1;
        g_outputLength++;
    }
    while (g_coderPending != 0) {
        if (1 - bit != 0)
            g_outputByte |= g_outputBitMask;
        if (g_outputBitMask != 0x80) {
            g_outputBitMask <<= 1;
        } else {
            out[g_outputLength] = g_outputByte;
            g_outputByte = 0;
            g_outputBitMask = 1;
            g_outputLength++;
        }
        g_coderPending--;
    }
}

// Narrows the coding interval to the sub-interval of a bit that has the given probability of being 0 (16-bit
// fixed point), then renormalises: shifts out settled bits and counts underflow bits. (EncodeBand)
void EncodeBit(int bit, unsigned short probability, unsigned char *out)
{
    if (bit != 0)
        g_coderLow = g_coderLow + (((g_coderHigh - g_coderLow + 1) * probability + 0x8000) >> 16);
    else
        g_coderHigh = (((g_coderHigh - g_coderLow + 1) * probability + 0x8000) >> 16) - 1 + g_coderLow;
    for (;;) {
        if (g_coderHigh < g_coderHalf) {
            EmitBit(0, out);
        } else if (g_coderLow >= g_coderHalf) {
            EmitBit(1, out);
        } else if (g_coderLow >= g_coderQuarter && g_coderHigh < g_coderThreeQuarters) {
            g_coderPending++;
            g_coderLow -= g_coderQuarter;
            g_coderHigh -= g_coderQuarter;
        } else {
            return;
        }
        g_coderLow <<= 1;
        if (g_coderLow >= g_coderFull)
            g_coderLow -= g_coderFull;
        g_coderHigh <<= 1;
        if (g_coderHigh >= g_coderFull)
            g_coderHigh -= g_coderFull;
        g_coderHigh++;
    }
}

// ---- band coding ----

// Codes the same sub-band of the three planes in turn (each plane's coded words continue where its previous band
// stopped, tracked in g_planeOffset0..2) with its model. length is passed through and unused. (EncodeStream)
int EncodeBandPlanes(short *plane0, short *plane1, short *plane2, unsigned char *out, int count, int *length,
                     BandModel *model)
{
    g_planeOffset0 += EncodeBand(plane0 + g_planeOffset0, out, count, length, &model->plane[0]);
    g_planeOffset1 += EncodeBand(plane1 + g_planeOffset1, out, count, length, &model->plane[1]);
    g_planeOffset2 += EncodeBand(plane2 + g_planeOffset2, out, count, length, &model->plane[2]);
    return 0;
}

// Sets up the arithmetic coder, then codes every level's right, bottom and diagonal sub-bands (coarsest level
// first, as PackLevels wrote them) into out; *length receives the number of bytes produced. (BMPPack)
int EncodeStream(short *plane0, short *plane1, short *plane2, unsigned char *out, const int *widths,
                 const int *heights, int levels, int *length, LevelModelSet *models)
{
    g_coderQuarter = 1 << (g_coderPrecision - 2);
    g_coderHalf = g_coderQuarter * 2;
    g_coderThreeQuarters = g_coderQuarter * 3;
    g_coderFull = g_coderQuarter * 4;
    g_coderMax = g_coderFull - 1;
    g_outputLength = 0;
    g_planeOffset0 = 0;
    g_planeOffset1 = 0;
    g_planeOffset2 = 0;
    if (levels > 0) {
        BandModel *rightBand = &models->level[0].band[0];
        BandModel *bottomBand = &models->level[0].band[1];
        BandModel *diagonalBand = &models->level[0].band[2];
        for (int level = levels; level > 0; level--) {
            EncodeBandPlanes(plane0, plane1, plane2, out, (widths[level - 1] - widths[level]) * heights[level],
                             length, rightBand);
            EncodeBandPlanes(plane0, plane1, plane2, out, (heights[level - 1] - heights[level]) * widths[level],
                             length, bottomBand);
            EncodeBandPlanes(plane0, plane1, plane2, out,
                             (heights[level - 1] - heights[level]) * (widths[level - 1] - widths[level]), length,
                             diagonalBand);
            rightBand += 3;
            bottomBand += 3;
            diagonalBand += 3;
        }
        *length = g_outputLength;
        return 0;
    }
    *length = 0;
    return 0;
}

// ---- colour planes ----

// Splits 24-bit BGR pixels into three 16-bit planes: A = 8 * (b + 3 * (r + 2g)), B = 8 * (4g - 3r - b) and
// C = 20 * (r - b). scratch is unused. Always succeeds. (BMPPack)
int ConvertBgrToPlanes(const unsigned char *bgr, void *scratch, short *planeA, short *planeB, short *planeC,
                       int width, int height)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned short b = *bgr++;
            unsigned short g = *bgr++;
            unsigned short r = *bgr++;
            planeA[y * width + x] = (b + (r + g * 2) * 3) * 8;
            planeB[y * width + x] = (g * 4 - r * 3 - b) * 8;
            planeC[y * width + x] = (r - b) * 20;
        }
    }
    return 0;
}

// Packs three planes into 32-bit words (11 + 11 + 10 bits) written as four bytes per cell: A + 0x200 clamped to
// 0..0x7ff, B + 0x400 clamped to 0..0x7ff, C + 0x200 clamped to 0..0x3ff. The source cells are cleared. stride is
// the distance between rows of the planes; out receives width * height * 4 bytes. (BMPPack)
void PackPlanesToWords(short *planeA, short *planeB, short *planeC, unsigned char *out, int width, int height,
                       int stride)
{
    int outIndex = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned short a = planeA[y * stride + x] + 0x200;
            if ((short)a < 0)
                a = 0;
            else if ((short)a > 0x7ff)
                a = 0x7ff;
            out[outIndex++] = a % 256;
            unsigned short b = planeB[y * stride + x] + 0x400;
            if ((short)b < 0)
                b = 0;
            else if ((short)b > 0x7ff)
                b = 0x7ff;
            out[outIndex++] = (b % 32) * 8 + (a >> 8);
            unsigned short c = planeC[y * stride + x] + 0x200;
            if ((short)c < 0)
                c = 0;
            else if ((short)c > 0x3ff)
                c = 0x3ff;
            out[outIndex++] = (c % 4) * 64 + (b >> 5);
            out[outIndex++] = c >> 2;
            planeA[y * stride + x] = 0;
            planeB[y * stride + x] = 0;
            planeC[y * stride + x] = 0;
        }
    }
}

// ---- quantisation ----

// Measures the rounding error of one coefficient without changing it: with q = round(value * quality / divisor)
// (sign symmetric, given rounding offset), a non-zero q adds value - dequantised(q) to *errorSum and counts in
// *count. (QuantiseBand pass one)
__forceinline void MeasureCoefficient(int value, int roundOffset, int divisor, int quality, int reconstructionOffset,
                                      int *errorSum, int *count)
{
    if (value == 0)
        return;
    if (value > 0) {
        int level = (value * quality + roundOffset) / divisor;
        if (level != 0) {
            *errorSum += value - (level * divisor + reconstructionOffset) / quality;
            (*count)++;
        }
    } else {
        int level = -((roundOffset - value * quality) / divisor);
        if (level != 0) {
            *errorSum -= (reconstructionOffset - level * divisor) / quality + value;
            (*count)++;
        }
    }
}

// Quantises one coefficient like MeasureCoefficient and returns q (0 when the sign would flip), while
// accumulating the same error statistics. (QuantiseBand pass two)
__forceinline int QuantiseCoefficient(int value, int roundOffset, int divisor, int quality,
                                      int reconstructionOffset, int *errorSum, int *count)
{
    int level;
    if (value == 0) {
        level = 0;
    } else if (value > 0) {
        level = (value * quality + roundOffset) / divisor;
        if (level < 0) {
            level = 0;
        } else if (level != 0) {
            *errorSum += value - (level * divisor + reconstructionOffset) / quality;
            (*count)++;
        }
    } else {
        level = -((roundOffset - value * quality) / divisor);
        if (level > 0) {
            level = 0;
        } else if (level != 0) {
            *errorSum -= (reconstructionOffset - level * divisor) / quality + value;
            (*count)++;
        }
    }
    return level;
}

// Turns an average rounding error into a rounding offset for the second pass:
// ((average - 0.5) * (1 - log2(scale) / 16) + 0.5) * divisor + 0.5, where average is the mean error as a fraction
// of the step clamped to 0..1 (0.5 when no coefficient was non-zero). (QuantiseBand)
__forceinline int RoundingOffset(int errorSum, int count, int quality, double divisor, double sharpness)
{
    double average;
    if (count != 0) {
        average = (double)errorSum * (double)quality / divisor / (double)count + 0.5;
        if (average < 0.0)
            average = 0.0;
        else if (average > 1.0)
            average = 1.0;
    } else {
        average = 0.5;
    }
    return (int)(((average - 0.5) * sharpness + 0.5) * divisor + 0.5);
}

// Turns a pass-two error sum into the bias byte stored in the band model: mean error in 1/256 steps + 128.5,
// clamped to 0..255. (QuantiseBand)
__forceinline int BiasByte(int errorSum, int count, int quality, double divisor)
{
    int bias = errorSum;
    if (count != 0)
        bias = (int)((double)errorSum * (double)quality * 256.0 / divisor / (double)count + 128.5);
    if (bias < 0)
        bias = 0;
    else if (bias > 0xff)
        bias = 0xff;
    return bias;
}

// Two-pass dead-zone quantiser of one sub-band: pass one measures each plane's mean rounding error, pass two
// quantises in place with offsets derived from it and records the final biases in model. scale is the quantiser
// strength (the step is 20 * scale / quality), quality the fixed-point denominator. (BMPPack quantiser driver)
void QuantiseBand(short *planeA, short *planeB, short *planeC, int width, int height, int stride, int quality,
                  int scale, BandModel *model)
{
    int divisor = scale * 20;
    int roundOffset = (divisor - 1) / 2;
    int reconstructionOffset = (quality - 1) / 2;
    int errorSumA = 0, errorSumB = 0, errorSumC = 0;
    int countA = 0, countB = 0, countC = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            MeasureCoefficient(planeA[y * stride + x], roundOffset, divisor, quality, reconstructionOffset,
                               &errorSumA, &countA);
            MeasureCoefficient(planeB[y * stride + x], roundOffset, divisor, quality, reconstructionOffset,
                               &errorSumB, &countB);
            MeasureCoefficient(planeC[y * stride + x], roundOffset, divisor, quality, reconstructionOffset,
                               &errorSumC, &countC);
        }
    }
    double divisorValue = (double)divisor;
    double scaleValue = divisorValue / 20.0;
    double sharpness = (16.0 - log(scaleValue) / log(2.0)) / 16.0;
    int offsetA = RoundingOffset(errorSumA, countA, quality, divisorValue, sharpness);
    int offsetB = RoundingOffset(errorSumB, countB, quality, divisorValue, sharpness);
    int offsetC = RoundingOffset(errorSumC, countC, quality, divisorValue, sharpness);

    errorSumA = errorSumB = errorSumC = 0;
    countA = countB = countC = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            planeA[y * stride + x] = QuantiseCoefficient(planeA[y * stride + x], offsetA, divisor, quality,
                                                         reconstructionOffset, &errorSumA, &countA);
            planeB[y * stride + x] = QuantiseCoefficient(planeB[y * stride + x], offsetB, divisor, quality,
                                                         reconstructionOffset, &errorSumB, &countB);
            planeC[y * stride + x] = QuantiseCoefficient(planeC[y * stride + x], offsetC, divisor, quality,
                                                         reconstructionOffset, &errorSumC, &countC);
        }
    }
    errorSumA = BiasByte(errorSumA, countA, quality, divisorValue);
    errorSumB = BiasByte(errorSumB, countB, quality, divisorValue);
    errorSumC = BiasByte(errorSumC, countC, quality, divisorValue);
    model->plane[0].bias = errorSumA;
    model->plane[1].bias = errorSumB;
    model->plane[2].bias = errorSumC;
}

// Quantises the coarsest approximation with the finest setting, then every level's right, bottom and diagonal
// sub-bands (coarsest level first) with a strength that halves per level down to the quality floor (the diagonal
// band uses the next finer strength). quality is clamped to 1..QUALITY_MAX. (BMPPack)
void QuantiseBands(short *planeA, short *planeB, short *planeC, const int *widths, const int *heights, int quality,
                   int levels, LevelModelSet *models)
{
    int scales[PACK_MAX_LEVELS + 1];

    if (quality < 1)
        quality = 1;
    else if (quality > QUALITY_MAX)
        quality = QUALITY_MAX;
    scales[0] = QUALITY_MAX;
    for (int i = 0; i < PACK_MAX_LEVELS; i++) {
        scales[i + 1] = scales[i] / 2;
        if (scales[i] / 2 < quality)
            scales[i + 1] = quality;
    }
    int stride = widths[0];
    QuantiseBand(planeA, planeB, planeC, widths[levels], heights[levels], stride, QUALITY_MAX, QUALITY_MAX,
                 &models->level[0].band[0]);
    if (levels > 0) {
        BandModel *rightBand = &models->level[0].band[0];
        BandModel *diagonalBand = &models->level[0].band[2];
        BandModel *bottomBand = &models->level[0].band[1];
        for (int level = levels; level > 0; level--) {
            int right = heights[level];
            QuantiseBand(planeA + widths[level], planeB + widths[level], planeC + widths[level],
                         widths[level - 1] - widths[level], right, stride, quality, scales[level], rightBand);
            int bottom = heights[level] * stride;
            QuantiseBand(planeA + bottom, planeB + bottom, planeC + bottom, widths[level],
                         heights[level - 1] - heights[level], stride, quality, scales[level], bottomBand);
            int diagonal = heights[level] * stride + widths[level];
            QuantiseBand(planeA + diagonal, planeB + diagonal, planeC + diagonal,
                         widths[level - 1] - widths[level], heights[level - 1] - heights[level], stride, quality,
                         scales[level - 1], diagonalBand);
            rightBand += 3;
            bottomBand += 3;
            diagonalBand += 3;
        }
    }
}

// ---- band coder ----

// Codes one plane's run/level words of a sub-band with the adaptive arithmetic coder: zero runs as 0 bits, levels as
// unary magnitude then a terminating 0 and a sign bit, the probability following the model measured by
// BandStatistics. Returns the number of words consumed. out receives the coded bytes. (EncodeBandPlanes)
int EncodeBand(const short *words, unsigned char *out, int count, int *length, PlaneModel *model)
{
    BandStatistics(model, words, count);
    unsigned int estimate = model->scale;
    if (estimate == 0)
        return 0;
    unsigned char magnitudeStep = model->zeroRatio;
    estimate = estimate * estimate * estimate * estimate;
    int remainingStep = 256 - magnitudeStep;
    int offset = (256 - model->escapeRatio) * remainingStep;
    remainingStep = remainingStep * model->escapeRatio;
    g_coderHigh = g_coderMax;
    offset = ((estimate & 0xffff) * offset + 0x8000 >> 16) + (estimate >> 16) * offset;
    unsigned int probability = (((estimate >> 1) - estimate) - 0x8000) / (estimate + 0x10000) + 1;
    g_outputByte = 0;
    g_outputBitMask = 1;
    g_coderLow = 0;
    g_coderPending = 0;
    int consumed = 0;
    int position = 0;
    int zeroRun = 0;
    if (probability > 0xfffe)
        probability = 0xfffe;
    while (position < count) {
        unsigned short word = words[consumed];
        unsigned short level = word >> 4;
        int run = (word & 0xf) + 1;
        consumed++;
        position += run;
        zeroRun += run;
        if (level != 0 || position == count) {
            int zeros = zeroRun - 1;
            zeroRun = zeros;
            if (zeros != 0) {
                zeroRun = 0;
                do {
                    EncodeBit(0, probability, out);
                    estimate = (estimate >> 16) * remainingStep + offset +
                               ((estimate & 0xffff) * remainingStep + 0x8000 >> 16);
                    if (estimate == 0)
                        estimate = 1;
                    probability = (((estimate >> 1) - estimate) - 0x8000) / (estimate + 0x10000) + 1;
                    if (probability > 0xfffe)
                        probability = 0xfffe;
                    zeros--;
                } while (zeros != 0);
            }
            if (level == 0) {
                EncodeBit(0, probability, out);
            } else {
                for (unsigned int ones = level & 0x7ff; ones != 0; ones--)
                    EncodeBit(1, probability, out);
                EncodeBit(0, probability, out);
                EncodeBit(word >> 15, 0x8000, out);
                estimate = (level & 0x7ff) * magnitudeStep * 256 + (estimate >> 16) * remainingStep + offset +
                           ((estimate & 0xffff) * remainingStep + 0x8000 >> 16);
                if (estimate == 0)
                    estimate = 1;
                probability = (((estimate >> 1) - estimate) - 0x8000) / (estimate + 0x10000) + 1;
                if (probability > 0xfffe)
                    probability = 0xfffe;
            }
        }
    }
    g_coderPending += g_coderPrecision;
    EmitBit(g_coderHalf <= g_coderLow, out);
    if (g_outputBitMask > 1) {
        out[g_outputLength] = g_outputByte;
        g_outputLength++;
    }
    return consumed;
}

// ---- file writer ----

// Appends one byte; *length is the index of the last byte written (so it starts at 0 after the first byte).
// (WriteStream)
__forceinline void PutByte(unsigned char *out, int *length, int value)
{
    out[++*length] = value % 256;
}

// Appends a 16-bit little-endian value. (WriteStream)
__forceinline void PutWord(unsigned char *out, int *length, int value)
{
    PutByte(out, length, value);
    PutByte(out, length, value / 256);
}

// Appends a 32-bit little-endian value. (WriteStream)
__forceinline void PutLong(unsigned char *out, int *length, int value)
{
    PutByte(out, length, value);
    PutByte(out, length, value / 256);
    PutByte(out, length, value / 65536);
    PutByte(out, length, value / 65536 / 256);
}

// Writes the compressed file into out: signature "HPC@" 3,0, quality, width, height, level count, two zero bytes,
// stream length, the band models (scale byte, then zeroRatio, escapeRatio and bias when the scale is non-zero),
// the packed pixel words of the coarsest level and the coded stream. Returns the number of models with a scale.
// (BMPPack)
int WriteStream(const unsigned char *pixelWords, const unsigned char *stream, const int *widths, const int *heights,
                int quality, int levels, const int *streamLength, const LevelModelSet *models, unsigned char *out,
                int *outLength)
{
    *outLength = 0;
    out[0] = 'H';
    PutByte(out, outLength, 'P');
    PutByte(out, outLength, 'C');
    PutByte(out, outLength, '@');
    PutByte(out, outLength, 3);
    PutByte(out, outLength, 0);
    if (quality == QUALITY_MAX)
        quality = 0;
    PutWord(out, outLength, quality);
    PutLong(out, outLength, widths[0]);
    PutLong(out, outLength, heights[0]);
    PutWord(out, outLength, levels);
    PutByte(out, outLength, 0);
    PutByte(out, outLength, 0);
    int length = *streamLength;
    PutLong(out, outLength, length);
    int codedModels = 0;
    const PlaneModel *model = &models->level[0].band[0].plane[0];
    for (int i = 0; i < levels * 9; i++, model++) {
        PutByte(out, outLength, model->scale);
        codedModels++;
        if (model->scale != 0) {
            PutByte(out, outLength, model->zeroRatio);
            PutByte(out, outLength, model->escapeRatio);
            PutByte(out, outLength, model->bias);
            codedModels += 3;
        }
    }
    int pixelBytes = widths[levels] * heights[levels] * 4;
    for (int i = 0; i < pixelBytes; i++)
        PutByte(out, outLength, pixelWords[i]);
    for (int i = 0; i < length; i++)
        PutByte(out, outLength, stream[i]);
    return codedModels;
}

// ---- BMPPack ----

// Exported entry point: compresses the 24-bit BGR bitmap bgr (width x height, rows tightly packed) into out and
// stores in *outLength the index of the last byte written. quality scales the quantiser (the coder works with
// quality << 6). The number of wavelet levels is the deepest one (up to eight) whose plane stays larger than 11
// pixels and whose scaled quality stays below PACK_QUALITY_LIMIT. Returns 1 on success, 0 when an allocation or the
// colour conversion fails.
extern "C" int BMPPack(const unsigned char *bgr, int width, int height, int quality, unsigned char *out,
                       int *outLength)
{
    int widths[PACK_MAX_LEVELS + 1];
    int heights[PACK_MAX_LEVELS + 1];
    int streamLength;
    LevelModelSet models;
    int attempt = 0;

    g_coderPrecision = 16;
    for (;;) {
        int scaledQuality = quality << 6;
        int levels = 0;
        widths[0] = width;
        heights[0] = height;
        for (int level = 1; level <= PACK_MAX_LEVELS; level++) {
            widths[level] = (widths[level - 1] + 1) / 2;
            heights[level] = (heights[level - 1] + 1) / 2;
            scaledQuality *= 2;
            if (widths[level - 1] >= PACK_MIN_PLANE_SIZE && heights[level - 1] >= PACK_MIN_PLANE_SIZE &&
                scaledQuality < PACK_QUALITY_LIMIT)
                levels = level;
        }
        int pixels = width * height;
        unsigned char *packedWords = (unsigned char *)operator new(widths[levels] * heights[levels] * 4);
        short *buffer;
        if (packedWords == 0 || (buffer = (short *)operator new(pixels * 8)) == 0)
            return 0;
        short *planeA = buffer + pixels;
        short *planeB = buffer + pixels * 2;
        short *planeC = buffer + pixels * 3;
        if (pixels * 4 > 0)
            memset(buffer, 0, pixels * 8);
        memset(&models, 0, sizeof(models.level));
        if (ConvertBgrToPlanes(bgr, buffer, planeA, planeB, planeC, width, height) != 0)
            break;
        for (int level = 0; level < levels; level++)
            TransformLevel(planeA, buffer, widths[level], heights[level], width);
        for (int level = 0; level < levels; level++)
            TransformLevel(planeB, buffer, widths[level], heights[level], width);
        for (int level = 0; level < levels; level++)
            TransformLevel(planeC, buffer, widths[level], heights[level], width);
        QuantiseBands(planeA, planeB, planeC, widths, heights, quality << 6, levels, &models);
        PackPlanesToWords(planeA, planeB, planeC, packedWords, widths[levels], heights[levels], width);
        PackLevels(planeA, buffer, widths, heights, levels);
        PackLevels(planeB, planeA, widths, heights, levels);
        PackLevels(planeC, planeB, widths, heights, levels);
        EncodeStream(buffer, planeA, planeB, (unsigned char *)planeC, widths, heights, levels, &streamLength,
                     &models);
        WriteStream(packedWords, (unsigned char *)planeC, widths, heights, quality << 6, levels, &streamLength,
                    &models, out, outLength);
        free(packedWords);
        free(buffer);
        attempt++;
        if (attempt > 0)
            return 1;
    }
    return 0;
}
