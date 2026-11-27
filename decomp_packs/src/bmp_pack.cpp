// bmp_pack.cpp : BMPPack, the BMP compressor of Packs.dll, and its helpers.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_pack.h"

// ---- run-length coding of quantised coefficients ----

// Stores one run/level pair: the zero run in the low nibble of the word and the level in the high twelve bits
// (positive levels as is, negative levels as 0x800 - level, saturated to +-0x7ff). (PackBlocks)
void StoreCoefficient(unsigned short *out, unsigned short zeroRun, short value)
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
void PackBlocks(const short *plane, unsigned short *out, int width, int height, int stride)
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
void PackLevels(const short *plane, unsigned short *out, const int *widths, const int *heights, int levels)
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
int EncodeBandPlanes(unsigned short *plane0, unsigned short *plane1, unsigned short *plane2, unsigned char *out, int count, int *length,
                     BandModel *model)
{
    g_planeOffset0 += EncodeBand(plane0 + g_planeOffset0, out, count, length, &model->plane[0]);
    g_planeOffset1 += EncodeBand(plane1 + g_planeOffset1, out, count, length, &model->plane[1]);
    g_planeOffset2 += EncodeBand(plane2 + g_planeOffset2, out, count, length, &model->plane[2]);
    return 0;
}

// Sets up the arithmetic coder, then codes every level's right, bottom and diagonal sub-bands (coarsest level
// first, as PackLevels wrote them) into out; *length receives the number of bytes produced. (BMPPack)
int EncodeStream(unsigned short *plane0, unsigned short *plane1, unsigned short *plane2, unsigned char *out, const int *widths,
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
        BandModel *rightBand = models->bandModel(0, 0);
        BandModel *bottomBand = models->bandModel(0, 1);
        BandModel *diagonalBand = models->bandModel(0, 2);
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
            unsigned short carryFromA = a >> 8;  // bits 8..10 of A continue in the next byte
            unsigned short b = planeB[y * stride + x] + 0x400;
            if ((short)b < 0)
                b = 0;
            else if ((short)b > 0x7ff)
                b = 0x7ff;
            out[outIndex++] = (b % 32) * 8 + carryFromA;
            unsigned short carryFromB = b >> 5;  // bits 5..10 of B continue in the next byte
            unsigned short c = planeC[y * stride + x] + 0x200;
            if ((short)c < 0)
                c = 0;
            else if ((short)c > 0x3ff)
                c = 0x3ff;
            out[outIndex++] = (c % 4) * 64 + carryFromB;
            out[outIndex++] = c >> 2;
            planeA[y * stride + x] = 0;
            planeB[y * stride + x] = 0;
            planeC[y * stride + x] = 0;
        }
    }
}

// ---- wavelet transform ----

// The analysis filter has 23 taps: the centre sample c and eleven on each side (l1..l11 to the left, r1..r11 to the
// right). TransformLevel keeps them in a window of 23 locals that shifts by one sample per output.
const int FILTER_GAIN = 19600;      // response of the low-pass filter to a constant signal
const int FILTER_ROUNDING = 9799;   // added to a filter sum's magnitude before the division
const int WINDOW_AHEAD = 11;        // the newest tap lies this many samples after the centre

// Low-pass filter of the even positions: a smooth bell with weight 198 at the centre, then 5741, 82 * 29 and so on,
// summing to FILTER_GAIN. (TransformLevel)
__forceinline int LowPassSum(int l11, int l10, int l9, int l8, int l7, int l6, int l5, int l4, int l3, int l2, int l1, int c, int r1, int r2, int r3, int r4, int r5, int r6, int r7, int r8, int r9, int r10, int r11)
{
    return 5 * (14 * (198 * c - r6 - l6) - 197 * (r3 + l3) + r9 + l9) +
           2 * (6 * (34 * (r4 + l4) + r8 + l8) - r10 - l10) - 29 * (82 * (r2 + l2) + r7 + l7) +
           5741 * (r1 + l1) + 169 * (r5 + l5) - l11 - r11;
}

// High-pass filter of the odd positions (weights sum to zero). (TransformLevel)
__forceinline int HighPassSum(int l11, int l10, int l9, int l8, int l7, int l6, int l5, int l4, int l3, int l2, int l1, int c, int r1, int r2, int r3, int r4, int r5, int r6, int r7, int r8, int r9, int r10, int r11)
{
    return 5 * (14 * (198 * c - r6 - l6) + 197 * (r3 + l3) - r9 - l9) + 29 * (r7 + l7 - 82 * (r2 + l2)) +
           2 * (6 * (34 * (r4 + l4) + r8 + l8) - r10 - l10) - 5741 * (r1 + l1) - 169 * (r5 + l5) + l11 + r11;
}

// Divides a filter sum by FILTER_GAIN, rounding to nearest with the sign kept symmetric. (TransformLevel)
__forceinline int ScaleFilterSum(int sum)
{
    if (sum >= 0)
        return (sum + FILTER_ROUNDING) / FILTER_GAIN;
    return -((FILTER_ROUNDING - sum) / FILTER_GAIN);
}

// One level of the two-dimensional analysis of a plane in place (scratch is a plane-sized work buffer): the rows are
// filtered into scratch, then the columns of scratch back into plane. In each line the even positions become the
// first half and the odd positions the second half, so the low-pass quarter ends up in the top-left corner. Samples
// beyond the line ends are mirrored without repeating the edge sample. Needs width and height of at least 12.
// (BMPPack)
void TransformLevel(short *plane, short *scratch, int width, int height, int stride)
{
    // Number of low-pass outputs in a row, and the row multiples of the column pass (rows below the top one that the
    // 12-sample start window and the mirror formula address).
    const int halfWidth = (width + 1) / 2;
    const int stride2 = 2 * stride;
    const int stride3 = 3 * stride;
    const int stride5 = 5 * stride;
    const int stride6 = 6 * stride;
    const int stride7 = 7 * stride;
    const int stride9 = 9 * stride;
    const int stride10 = 10 * stride;
    const int stride11 = 11 * stride;
    const int stride13 = 13 * stride;
    // The 23-tap window. Before the first shift it holds the samples around position -1 (mirrored at the line start),
    // so l11 is dropped unread by that shift.
    int l11, l10, l9, l8, l7, l6, l5, l4, l3, l2, l1, c, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11;
    for (int row = 0; row < height; row++) {
        const short *line = plane + row * stride;
        // Start window: line[0..11] mirrored about line[0].
        l10 = line[11];
        l9 = line[10];
        l8 = line[9];
        l7 = line[8];
        l6 = line[7];
        l5 = line[6];
        l4 = line[5];
        l3 = line[4];
        l2 = line[3];
        l1 = line[2];
        c = line[1];
        r1 = line[0];
        r2 = c;
        r3 = l1;
        r4 = l2;
        r5 = l3;
        r6 = l4;
        r7 = l5;
        r8 = l6;
        r9 = l7;
        r10 = l8;
        r11 = l9;
        int highCount = 0;
        for (int position = 0; position < width; position++) {
            l11 = l10;
            l10 = l9;
            l9 = l8;
            l8 = l7;
            l7 = l6;
            l6 = l5;
            l5 = l4;
            l4 = l3;
            l3 = l2;
            l2 = l1;
            l1 = c;
            c = r1;
            r1 = r2;
            r2 = r3;
            r3 = r4;
            r4 = r5;
            r5 = r6;
            r6 = r7;
            r7 = r8;
            r8 = r9;
            r9 = r10;
            r10 = r11;
            if (position + WINDOW_AHEAD < width)
                r11 = line[position + WINDOW_AHEAD];
            else
                r11 = line[2 * width - position - 13];  // mirror of position + 11 about the last sample
            if (position % 2 == 0) {
                scratch[row * stride + highCount] = ScaleFilterSum(LowPassSum(l11, l10, l9, l8, l7, l6, l5, l4, l3, l2, l1, c, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11));
            } else {
                scratch[row * stride + halfWidth + highCount] = ScaleFilterSum(HighPassSum(l11, l10, l9, l8, l7, l6, l5, l4, l3, l2, l1, c, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11));
                highCount++;
            }
        }
    }
    for (int column = 0; column < width; column++) {
        // Start window: rows 0..11 of the column mirrored about row 0.
        l10 = scratch[column + stride11];
        l9 = scratch[column + stride10];
        l8 = scratch[column + stride9];
        l7 = scratch[column + 8 * stride];
        l6 = scratch[column + stride7];
        l5 = scratch[column + stride6];
        l4 = scratch[column + stride5];
        l3 = scratch[column + 4 * stride];
        l2 = scratch[column + stride3];
        l1 = scratch[column + stride2];
        c = scratch[column + stride];
        r1 = scratch[column];
        r2 = c;
        r3 = l1;
        r4 = l2;
        r5 = l3;
        r6 = l4;
        r7 = l5;
        r8 = l6;
        r9 = l7;
        r10 = l8;
        r11 = l9;
        int highCount = 0;
        for (int position = 0; position < height; position++) {
            l11 = l10;
            l10 = l9;
            l9 = l8;
            l8 = l7;
            l7 = l6;
            l6 = l5;
            l5 = l4;
            l4 = l3;
            l3 = l2;
            l2 = l1;
            l1 = c;
            c = r1;
            r1 = r2;
            r2 = r3;
            r3 = r4;
            r4 = r5;
            r5 = r6;
            r6 = r7;
            r7 = r8;
            r8 = r9;
            r9 = r10;
            r10 = r11;
            if (position + WINDOW_AHEAD < height)
                r11 = scratch[column + (position + WINDOW_AHEAD) * stride];
            else
                r11 = scratch[column + 2 * height * stride - position * stride - stride13];  // row 2 * height - position - 13
            if (position % 2 == 0) {
                plane[highCount * stride + column] = ScaleFilterSum(LowPassSum(l11, l10, l9, l8, l7, l6, l5, l4, l3, l2, l1, c, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11));
            } else {
                plane[((height + 1) / 2 + highCount) * stride + column] = ScaleFilterSum(HighPassSum(l11, l10, l9, l8, l7, l6, l5, l4, l3, l2, l1, c, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11));
                highCount++;
            }
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

// Quantiser divisor per unit of strength (QuantiseBand's step is 20 * scale).
const int QUANT_DIVISOR_PER_SCALE = 20;

// Two-pass dead-zone quantiser of one sub-band: pass one measures each plane's mean rounding error, pass two
// quantises in place with offsets derived from it and records the final biases in model. scale is the quantiser
// strength (the step is 20 * scale / quality), quality the fixed-point denominator. (BMPPack quantiser driver)
void QuantiseBand(short *planeA, short *planeB, short *planeC, int width, int height, int stride, int quality,
                  int scale, BandModel *model)
{
    scale *= QUANT_DIVISOR_PER_SCALE;  // from here on scale is the quantiser divisor (step), not the strength
    int roundOffset = (scale - 1) / 2;
    int reconstructionOffset = (quality - 1) / 2;
    int errorSumA = 0, errorSumB = 0, errorSumC = 0;
    int countA = 0, countB = 0, countC = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            MeasureCoefficient(planeA[y * stride + x], roundOffset, scale, quality, reconstructionOffset,
                               &errorSumA, &countA);
            MeasureCoefficient(planeB[y * stride + x], roundOffset, scale, quality, reconstructionOffset,
                               &errorSumB, &countB);
            MeasureCoefficient(planeC[y * stride + x], roundOffset, scale, quality, reconstructionOffset,
                               &errorSumC, &countC);
        }
    }
    double divisorValue = (double)scale;
    // One double: first the strength (divisor / 20), then the sharpness derived from it; the original keeps both in
    // one frame slot (a separate scaleValue made the frame 4 bytes smaller).
    double sharpness = divisorValue / 20.0;
    sharpness = (16.0 - log(sharpness) / log(2.0)) / 16.0;
    int offsetA = RoundingOffset(errorSumA, countA, quality, divisorValue, sharpness);
    int offsetB = RoundingOffset(errorSumB, countB, quality, divisorValue, sharpness);
    int offsetC = RoundingOffset(errorSumC, countC, quality, divisorValue, sharpness);

    errorSumA = errorSumB = errorSumC = 0;
    countA = countB = countC = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            planeA[y * stride + x] = QuantiseCoefficient(planeA[y * stride + x], offsetA, scale, quality,
                                                         reconstructionOffset, &errorSumA, &countA);
            planeB[y * stride + x] = QuantiseCoefficient(planeB[y * stride + x], offsetB, scale, quality,
                                                         reconstructionOffset, &errorSumB, &countB);
            planeC[y * stride + x] = QuantiseCoefficient(planeC[y * stride + x], offsetC, scale, quality,
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
                 models->bandModel(0, 0));
    if (levels > 0) {
        BandModel *rightBand = models->bandModel(0, 0);
        BandModel *bottomBand = models->bandModel(0, 1);
        BandModel *diagonalBand = models->bandModel(0, 2);
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

// Mask and shifts of a run/level word: zero run in the low nibble, level magnitude above it, sign in bit 15.
const unsigned short WORD_RUN_MASK = 0xf;
const unsigned short WORD_LEVEL_MASK = 0x7ff;
const int WORD_LEVEL_SHIFT = 4;

// Model byte scaling: the magnitude class is 16 * mean ^ 0.25, the ratios are fractions of 256.
const double SCALE_FACTOR = 16.0;
const double SCALE_EXPONENT = 0.25;
const double RATIO_FACTOR = 256.0;
const double ROUND_HALF = 0.5;
const double NO_WEIGHT = 0.0;
const double FULL_WEIGHT = 1.0;

// Measures one plane's run/level words and writes the three model bytes into model: scale (clamped to 1..255) and the
// slope of a first-order fit of each level against its predecessor, before and after two refinement passes that
// replay the zero runs. An empty band gets scale 0. Always returns 0. (EncodeBand)
int BandStatistics(PlaneModel *model, const unsigned short *words, int count)
{
    double total = count;
    unsigned short firstWord = words[0];
    unsigned short firstLevel;
    if (firstWord & WORD_RUN_MASK)
        firstLevel = 0;
    else
        firstLevel = (firstWord >> WORD_LEVEL_SHIFT) & WORD_LEVEL_MASK;
    double sumCurrent = 0.0;
    double sumLagged;
    double sumSquares = 0.0;
    double sumProducts = 0.0;
    unsigned short level = 0;
    unsigned short previous = 0;
    int consumed = 0;
    const unsigned short *word = words;
    while (consumed < count) {
        unsigned short entry = *word++;
        unsigned short run = entry & WORD_RUN_MASK;
        level = (entry >> WORD_LEVEL_SHIFT) & WORD_LEVEL_MASK;
        consumed += run + 1;
        sumCurrent += level;
        sumSquares += level * level;
        sumProducts += previous * level;
        previous = run == 0 ? level : 0;
    }
    if (sumCurrent == 0.0) {
        model->scale = 0;
        return 0;
    }
    double mean = sumCurrent / total;
    sumLagged = mean + sumCurrent - level;
    sumSquares = mean * mean + sumSquares - level * level;
    sumProducts = firstLevel * mean + sumProducts;
    double slope = (sumProducts * total - sumCurrent * sumLagged) / (sumSquares * total - sumLagged * sumLagged);
    sumCurrent = 0.0;
    sumProducts = 0.0;
    sumSquares = 0.0;
    sumLagged = 0.0;
    double lagged = mean;
    double current = mean;
    double retain = (FULL_WEIGHT - slope) * NO_WEIGHT;
    double inject = (FULL_WEIGHT - slope) * mean;
    word = words;
    consumed = 0;
    while ((double)consumed < total) {
        unsigned short entry = *word++;
        unsigned short run = entry & WORD_RUN_MASK;
        level = (entry >> WORD_LEVEL_SHIFT) & WORD_LEVEL_MASK;
        consumed += run + 1;
        for (unsigned short step = run; step != 0; step--) {
            current = retain * current + inject;
            sumLagged += lagged;
            sumCurrent += current;
            sumSquares += lagged * lagged;
            sumProducts += lagged * current;
            lagged = current;
        }
        current = level * slope + retain * current + inject;
        sumLagged += lagged;
        sumCurrent += current;
        sumSquares += lagged * lagged;
        sumProducts += lagged * current;
        lagged = current;
    }
    slope = (sumProducts * total - sumCurrent * sumLagged) / (sumSquares * total - sumLagged * sumLagged);
    sumCurrent = 0.0;
    sumProducts = 0.0;
    sumSquares = 0.0;
    sumLagged = 0.0;
    current = mean;
    lagged = mean;
    retain = FULL_WEIGHT - slope;
    inject = (FULL_WEIGHT - slope) * mean * NO_WEIGHT;
    word = words;
    consumed = 0;
    while ((double)consumed < total) {
        unsigned short entry = *word++;
        unsigned short run = entry & WORD_RUN_MASK;
        level = (entry >> WORD_LEVEL_SHIFT) & WORD_LEVEL_MASK;
        consumed += run + 1;
        for (unsigned short step = run; step != 0; step--) {
            current = retain * current + inject;
            sumLagged += lagged;
            sumCurrent += current;
            sumSquares += lagged * lagged;
            sumProducts += lagged * current;
            lagged = current;
        }
        current = level * slope + retain * current + inject;
        sumLagged += lagged;
        sumCurrent += current;
        sumSquares += lagged * lagged;
        sumProducts += lagged * current;
        lagged = current;
    }
    double refinedSlope = (sumProducts * total - sumCurrent * sumLagged) / (sumSquares * total - sumLagged * sumLagged);
    int value = (int)(pow(mean, SCALE_EXPONENT) * SCALE_FACTOR + ROUND_HALF);
    model->scale = value;
    if (value <= 1)
        model->scale = 1;
    if (value >= 255)
        model->scale = 255;
    value = (int)(slope * RATIO_FACTOR + ROUND_HALF);
    model->zeroRatio = value;
    if (value <= 0)
        model->zeroRatio = 0;
    if (value >= 255)
        model->zeroRatio = 255;
    value = (int)(refinedSlope * RATIO_FACTOR + ROUND_HALF);
    model->escapeRatio = value;
    if (value <= 0)
        model->escapeRatio = 0;
    if (value >= 255)
        model->escapeRatio = 255;
    return 0;
}


// Codes one plane's run/level words of a sub-band with the adaptive arithmetic coder: zero runs as 0 bits, levels as
// unary magnitude then a terminating 0 and a sign bit, the probability following the model measured by
// BandStatistics. Returns the number of words consumed. out receives the coded bytes. (EncodeBandPlanes)
int EncodeBand(const unsigned short *words, unsigned char *out, int count, int *length, PlaneModel *model)
{
    BandStatistics(model, words, count);
    unsigned int estimate = model->scale;
    if (estimate == 0)
        return 0;
    int zeroRatio = model->zeroRatio;
    int escapeRatio = model->escapeRatio;
    estimate = estimate * estimate * estimate * estimate;
    int remainingStep = 256 - zeroRatio;
    int offset = (256 - escapeRatio) * remainingStep;
    remainingStep = remainingStep * escapeRatio;
    int magnitudeStep = zeroRatio << 8;
    g_coderHigh = g_coderMax;
    unsigned int probability = (((estimate >> 1) - estimate) - 0x8000) / (estimate + 0x10000) + 1;
    offset = ((estimate & 0xffff) * offset + 0x8000 >> 16) + (estimate >> 16) * offset;
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
        unsigned short run = (word & 0xf) + 1;
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
                EncodeBit(level >> 11, 0x8000, out);
                estimate = (level & 0x7ff) * magnitudeStep + (estimate >> 16) * remainingStep + offset +
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
// Callers reduce wider values with % 256 themselves. (WriteStream)
__forceinline void PutByte(unsigned char *out, int *length, unsigned char value)
{
    out[++*length] = value;
}

// Appends a 16-bit little-endian value. (WriteStream)
__forceinline void PutWord(unsigned char *out, int *length, int value)
{
    PutByte(out, length, value % 256);
    PutByte(out, length, value / 256 % 256);
}

// Appends a 32-bit little-endian value. (WriteStream)
__forceinline void PutLong(unsigned char *out, int *length, int value)
{
    PutByte(out, length, value % 256);
    PutByte(out, length, value / 256 % 256);
    PutByte(out, length, value / 65536 % 256);
    PutByte(out, length, value / 65536 / 256 % 256);
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
    const PlaneModel *model = models->firstPlaneModel();
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
// colour conversion fails. The pack loop runs PACK_ATTEMPTS (one) time: the original keeps the counter and its
// `inc; cmp 1; jl` back edge, so the loop was in the source with a bound of one.
extern "C" int BMPPack(const unsigned char *bgr, int width, int height, int quality, unsigned char *out,
                       int *outLength)
{
    int widths[PACK_MAX_LEVELS + 1];
    int heights[PACK_MAX_LEVELS + 1];
    int streamLength;
    LevelModelSet models;  // byte array: gives the function its /GS cookie, like the original

    g_coderPrecision = 16;
    int scaledQuality = quality << 6;  // the coder's quality scale, passed on to QuantiseBands and WriteStream
    for (int attempt = 0; attempt < PACK_ATTEMPTS; attempt++) {
        int levelQuality = scaledQuality;
        int levels = 0;
        widths[0] = width;
        heights[0] = height;
        for (int level = 1; level <= PACK_MAX_LEVELS; level++) {
            widths[level] = (widths[level - 1] + 1) / 2;
            heights[level] = (heights[level - 1] + 1) / 2;
            levelQuality *= 2;
            if (widths[level - 1] > PACK_PLANE_SIZE_FLOOR && heights[level - 1] > PACK_PLANE_SIZE_FLOOR &&
                levelQuality < PACK_QUALITY_LIMIT)
                levels = level;
        }
        int pixels = widths[0] * heights[0];  // level 0 is the whole bitmap
        unsigned char *packedWords = (unsigned char *)operator new(widths[levels] * heights[levels] * 4);
        short *buffer;
        if (packedWords == 0 || (buffer = (short *)operator new(pixels * 8)) == 0)
            return 0;
        short *planeA = buffer + pixels;
        short *planeB = buffer + pixels * 2;
        short *planeC = buffer + pixels * 3;
        for (int i = 0; i < pixels * 4; i++)  // clear all four planes (the compiler turns this into rep stos)
            buffer[i] = 0;
        memset(&models, 0, sizeof(models));
        if (ConvertBgrToPlanes(bgr, buffer, planeA, planeB, planeC, width, height) != 0)
            return 0;
        for (int level = 0; level < levels; level++)
            TransformLevel(planeA, buffer, widths[level], heights[level], width);
        for (int level = 0; level < levels; level++)
            TransformLevel(planeB, buffer, widths[level], heights[level], width);
        for (int level = 0; level < levels; level++)
            TransformLevel(planeC, buffer, widths[level], heights[level], width);
        QuantiseBands(planeA, planeB, planeC, widths, heights, scaledQuality, levels, &models);
        PackPlanesToWords(planeA, planeB, planeC, packedWords, widths[levels], heights[levels], width);
        PackLevels(planeA, (unsigned short *)buffer, widths, heights, levels);
        PackLevels(planeB, (unsigned short *)planeA, widths, heights, levels);
        PackLevels(planeC, (unsigned short *)planeB, widths, heights, levels);
        EncodeStream((unsigned short *)buffer, (unsigned short *)planeA, (unsigned short *)planeB,
                     (unsigned char *)planeC, widths, heights, levels, &streamLength, &models);
        WriteStream(packedWords, (unsigned char *)planeC, widths, heights, scaledQuality, levels, &streamLength,
                    &models, out, outLength);
        free(packedWords);
        free(buffer);
    }
    return 1;
}
