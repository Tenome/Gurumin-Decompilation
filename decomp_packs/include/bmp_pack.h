// bmp_pack.h : types, constants and globals of Packs.dll's BMP compressor (BMPPack and its helpers).
//
// BMPPack turns a 24-bit BGR bitmap into three 16-bit planes, runs a wavelet-style transform over each plane,
// quantises the sub-bands, run-length codes the coefficients and arithmetic-codes the result.
#ifndef BMP_PACK_H
#define BMP_PACK_H

// The encoder keeps up to this many decomposition levels (each level halves the plane size).
const int PACK_MAX_LEVELS = 8;

// BMPPack runs its pack loop this many times (the original left room for retries but does only one pass).
const int PACK_ATTEMPTS = 1;

// A run/level word holds the zero run in its low four bits and the level above them.
const int LEVEL_UNIT = 16;
const int COEFFICIENT_LIMIT = 0x7ff;  // levels saturate to +-this
const int NEGATIVE_LEVEL_BASE = 0x800;  // a negative level v is stored as this minus v

// Per-plane statistics of one sub-band, written by BandStatistics and read by the arithmetic coder.
struct PlaneModel {
    unsigned char scale;      // 0 when the band is empty, else the coefficient magnitude class
    unsigned char zeroRatio;  // 0..255 fraction of zero run/level pairs
    unsigned char escapeRatio;  // 0..255 fraction of large (escaped) coefficients
    unsigned char bias;       // quantiser reconstruction bias (0..255) the decoder adds back
};

// The three planes' models of one sub-band.
struct BandModel {
    PlaneModel plane[3];
};

// Plane models, band models and levels are laid out back to back, four bytes per plane model, so the whole set is
// one byte array: BMPPack's /GS cookie (cl 13 guards only locals that hold a char array of 8 or more bytes) shows
// the original declared it that way, and WriteStream walks it as one run of 4-byte plane models.
const int PLANE_MODEL_BYTES = 4;
const int PLANES_PER_BAND = 3;
const int BANDS_PER_LEVEL = 3;  // right, bottom and diagonal sub-band of a level
const int BAND_MODEL_BYTES = PLANES_PER_BAND * PLANE_MODEL_BYTES;
const int LEVEL_MODEL_BYTES = BANDS_PER_LEVEL * BAND_MODEL_BYTES;

// Everything the coder and the stream writer need from the encoding passes: the models of every decomposition
// level (three sub-bands each, three planes per sub-band).
struct LevelModelSet {
    unsigned char bytes[PACK_MAX_LEVELS * LEVEL_MODEL_BYTES];

    // Models of one sub-band (band 0 = right, 1 = bottom, 2 = diagonal) of a decomposition level.
    BandModel *bandModel(int level, int band) {
        return (BandModel *)(bytes + level * LEVEL_MODEL_BYTES + band * BAND_MODEL_BYTES);
    }
    const PlaneModel *firstPlaneModel() const { return (const PlaneModel *)bytes; }
};

// Side of the blocks the run-length coder scans, and the number of cells in one block.
const int PACK_PLANE_SIZE_FLOOR = 11;   // a plane is only halved again while both sides exceed this many pixels
const int PACK_QUALITY_LIMIT = 0x5a828;   // a level is only used while (quality << (6 + level)) stays below this
const int QUALITY_MAX = 0x10000;  // quality scale: 1..QUALITY_MAX, QUALITY_MAX = finest quantisation

const int SCAN_BLOCK_SIZE = 16;
const int SCAN_BLOCK_CELLS = 256;

// Scan order inside a block: column and row of cell i.
extern int g_scanColumn[SCAN_BLOCK_CELLS];
extern int g_scanRow[SCAN_BLOCK_CELLS];

// Run-length stream writer state.
extern int g_rleLength;  // number of 16-bit words written to the current plane stream

// Arithmetic coder state.
extern int g_coderPrecision;           // code register width in bits (BMPPack sets 16)
extern unsigned int g_coderLow;        // lower bound of the interval
extern unsigned int g_coderHigh;       // upper bound of the interval
extern unsigned int g_coderQuarter;    // 1 << (precision - 2)
extern unsigned int g_coderHalf;       // 2 * quarter
extern unsigned int g_coderThreeQuarters;  // 3 * quarter
extern unsigned int g_coderFull;       // 4 * quarter
extern unsigned int g_coderMax;        // full - 1, the initial upper bound
extern int g_coderPending;             // bits waiting on the underflow rule
extern unsigned char g_outputByte;     // byte being assembled
extern unsigned char g_outputBitMask;  // next bit position in g_outputByte
extern int g_outputLength;             // bytes written to the output stream

// Where the next plane's coded words go (in 16-bit words) in each of the three plane buffers.
extern int g_planeOffset0;
extern int g_planeOffset1;
extern int g_planeOffset2;

// Run-length coding helpers.
void StoreCoefficient(unsigned short *out, unsigned short zeroRun, short value);
void PackBlocks(const short *plane, unsigned short *out, int width, int height, int stride);
void PackLevels(const short *plane, unsigned short *out, const int *widths, const int *heights, int levels);

// Arithmetic coder.
void EmitBit(int bit, unsigned char *out);
void EncodeBit(int bit, unsigned short probability, unsigned char *out);

// Per-plane band coding.
int BandStatistics(PlaneModel *model, const unsigned short *words, int count);
int EncodeBand(const unsigned short *words, unsigned char *out, int count, int *length, PlaneModel *model);
int EncodeBandPlanes(unsigned short *plane0, unsigned short *plane1, unsigned short *plane2, unsigned char *out, int count, int *length,
                     BandModel *model);
int EncodeStream(unsigned short *plane0, unsigned short *plane1, unsigned short *plane2, unsigned char *out, const int *widths,
                 const int *heights, int levels, int *length, LevelModelSet *models);

// One level of the 23-tap wavelet analysis of a plane in place (scratch is a plane-sized work buffer): low-pass half
// first, high-pass half second, in both directions. Needs width and height of at least 12.
void TransformLevel(short *plane, short *scratch, int width, int height, int stride);

// Writes the compressed file: header, band models, packed pixel words and the arithmetic-coded stream.
int WriteStream(const unsigned char *pixelWords, const unsigned char *stream, const int *widths, const int *heights,
                int quality, int levels, const int *streamLength, const LevelModelSet *models, unsigned char *out,
                int *outLength);

// Quantisation of one sub-band of the three planes; stores each plane's bias in model.
void QuantiseBand(short *planeA, short *planeB, short *planeC, int width, int height, int stride, int quality,
                  int scale, BandModel *model);

// Quantises every sub-band of the three planes (coarsest approximation first) and records the biases in models.
void QuantiseBands(short *planeA, short *planeB, short *planeC, const int *widths, const int *heights, int quality,
                   int levels, LevelModelSet *models);

// Colour conversion and clamping.
int ConvertBgrToPlanes(const unsigned char *bgr, void *scratch, short *planeA, short *planeB, short *planeC,
                       int width, int height);
void PackPlanesToWords(short *planeA, short *planeB, short *planeC, unsigned char *out,
                       int width, int height, int stride);

// Exported: compresses a 24-bit BGR bitmap into out (outLength receives the last written index). Returns 1 on
// success, 0 when an allocation fails.
extern "C" int BMPPack(const unsigned char *bgr, int width, int height, int quality, unsigned char *out,
                       int *outLength);

#endif
