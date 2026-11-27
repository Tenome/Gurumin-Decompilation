// exec_pack.cpp : ExecPack / ExecUnpack, the LZ77-style compressor behind Packs.dll's archives (format: exec_pack.h).
#include <string.h>
#include "exec_pack.h"

// ---- limits of the format ----
// Table entries cleared per memset call: the tables are cleared in 32 KB steps.
const int EXEC_CLEAR_BYTES = 0x8000;
const int EXEC_CLEAR_ENTRIES = EXEC_CLEAR_BYTES / sizeof(unsigned short);
const unsigned EXEC_MIN_RUN = 14;           // shortest run of one byte coded as a run
const unsigned EXEC_MAX_RUN_SCAN = 0xfff;   // longest run the scanner looks at
const unsigned EXEC_MAX_MATCH = 0x10d;      // match lengths stay below this
const unsigned EXEC_GOOD_ENOUGH = 0x40;     // a run this long skips the dictionary search
const unsigned EXEC_FIRST_MASK = 0x80;      // first flag word starts mid-way: its low byte is never used
const unsigned short EXEC_RUN_OFFSET = 1;   // offset value that marks a run
const unsigned EXEC_PROGRESS_STEP = 0x319c; // progress units one chunk is worth
const unsigned EXEC_PROGRESS_STAGE = 0x64000;
const unsigned EXEC_PROGRESS_CHUNK_WEIGHT = 0x7f;
const unsigned EXEC_PROGRESS_STAGE_WEIGHT = 0x1000;

// ---- decoder state ----
ExecUnpackStream *g_unpack;           // chunk being expanded
BitReader g_reader;
unsigned char *g_unpackChunkStart;    // start of g_unpackChunk, where every chunk is expanded
unsigned char *g_unpackDest;          // caller's buffer, next free byte
unsigned g_unpackLimit;               // original size: no chunk may grow past it
unsigned g_unpackChunkCount;          // chunk count read from the header
unsigned char g_unpackChunk[EXEC_CHUNK_SIZE];

// ---- encoder state ----
ExecPackStream *g_pack;               // chunk being compressed
const unsigned char *g_inBase;        // start of the chunk
unsigned g_inPos;                     // offset of the next source byte in the chunk
unsigned g_unusedCounter;             // cleared at the start of a chunk, never used
unsigned short g_unusedHash;          // cleared at the start of a chunk, never used
unsigned g_matchLength;               // length of the token being coded
unsigned g_matchPos;                  // chunk position the token copies from, EXEC_NO_POSITION for a run
unsigned g_literal;                   // first byte at the current position (the byte a run repeats)
unsigned short *g_flagWord;           // flag word being filled in the token stream
unsigned short g_flagMask;            // bit of g_flagWord the next flag goes to
unsigned char g_encodeBuffer[EXEC_CHUNK_SIZE * 2];  // token stream of one chunk
unsigned char g_packChunk[EXEC_CHUNK_SIZE];         // source bytes of one chunk
unsigned short g_nextTable[EXEC_CHAIN_MASK + 1];
unsigned short g_headTable[0x10000];
unsigned short g_tailTable[0x10000];

// ---- progress reporting ----
ExecProgressFn g_packProgress;
ExecProgressFn g_unpackProgress;
void *g_progressUser;
unsigned g_progressStage;
unsigned g_progressStageCount;
int g_cancelled;                      // set by a progress callback to abort

// ================= decoder =================

// Reads one flag bit from the packed stream, refilling the 16-bit buffer when it is empty. (DecodeChunk)
inline unsigned short GetBit(void)
{
    if (g_reader.count == 0) {
        g_reader.bits = *(const unsigned short *)g_unpack->in;
        g_reader.count = 16;
        g_unpack->in += 2;
        g_reader.consumed += 2;
    }
    unsigned short bit = g_reader.bits & 1;
    g_reader.bits >>= 1;
    g_reader.count--;
    return bit;
}

// Reads `count` flag bits, first bit most significant. The result is a 16-bit value (an offset or length field),
// which is why `value` is an unsigned short. (DecodeChunk)
inline unsigned short GetBits(int count)
{
    unsigned short value = 0;
    while (count != 0) {
        value <<= 1;
        value += GetBit();
        count--;
    }
    return value;
}

// Expands one chunk record into g_unpackChunk: literals, offset/length copies and runs until the end marker.
// Returns 1 at the end marker, 0 when a copy would pass the original size. (UnpackRecords)
int DecodeChunk(void)
{
    g_reader.consumed = 4;
    g_reader.bits = g_unpack->in[1];
    g_reader.count = 8;
    g_unpack->in += 2;
    for (;;) {
        if (GetBit() == 0) {
            *g_unpack->out = *g_unpack->in;
            g_unpack->out++;
            g_unpack->in++;
            g_unpack->outCount++;
            g_reader.consumed++;
            continue;
        }
        unsigned offset;
        if (GetBit() == 0) {
            offset = *g_unpack->in++;
            g_reader.consumed++;
        } else {
            unsigned offsetHigh = GetBits(5);
            offset = offsetHigh * 256 + *g_unpack->in++;
            if (offset == 0)
                return 1;
            if (offset == EXEC_RUN_OFFSET) {
                unsigned runLength;
                if (GetBit() == 0) {
                    runLength = GetBits(4);
                    g_reader.consumed += 0xf + runLength;
                } else {
                    unsigned runHigh = GetBits(4);
                    runLength = runHigh * 256 + *g_unpack->in++;
                    g_reader.consumed += 0x10 + runLength;
                }
                unsigned char fill = *g_unpack->in++;
                for (unsigned n = runLength + EXEC_MIN_RUN; n > 0; n--)
                    *g_unpack->out++ = fill;
                g_unpack->outCount += runLength + EXEC_MIN_RUN;
                continue;
            }
        }
        unsigned length;
        if (GetBit() != 0)
            length = 2;
        else if (GetBit() != 0)
            length = 3;
        else if (GetBit() != 0)
            length = 4;
        else if (GetBit() != 0)
            length = 5;
        else if (GetBit() == 0) {
            length = *g_unpack->in++ + EXEC_MIN_RUN;
            g_reader.consumed++;
        } else
            length = GetBits(3) + 6;
        const unsigned char *from = g_unpack->out - offset;
        if (g_unpackLimit < g_unpack->outCount + length)
            return 0;
        for (unsigned n = length; n > 0; n--)
            *g_unpack->out++ = *from++;
        g_unpack->outCount += length;
    }
}

// Walks the chunk records of a packed buffer, expanding each into the caller's buffer and reporting progress.
// Returns 1 when done, EXEC_ERROR_CORRUPT for a bad chunk, 0 when cancelled. (ExecUnpack)
int UnpackRecords(ExecUnpackStream *stream)
{
    unsigned consumed = 0;
    g_unpack = stream;
    stream->outCount = 0;
    if (g_unpack->inSize > 0) {
        unsigned progress = 0;
        do {
            g_unpack->recordSize = *(const unsigned short *)g_unpack->in;
            consumed += g_unpack->recordSize;
            if (consumed >= g_unpack->inSize)
                return 1;
            g_unpack->in += 2;
            if (DecodeChunk() == 0)
                return EXEC_ERROR_CORRUPT;
            unsigned chunkBytes = g_unpack->outCount;
            if (g_unpackProgress != 0)
                g_cancelled = g_unpackProgress(g_progressUser, chunkBytes,
                    (g_progressStage * EXEC_PROGRESS_STAGE + EXEC_PROGRESS_STEP + progress) /
                    (g_progressStageCount * EXEC_PROGRESS_STAGE_WEIGHT + g_unpackChunkCount * EXEC_PROGRESS_CHUNK_WEIGHT));
            if (g_cancelled != 0)
                return 0;
            memcpy(g_unpackDest, g_unpackChunkStart, chunkBytes);
            g_unpackDest += chunkBytes;
            consumed++;
            g_unpack->out = g_unpackChunkStart;
            progress += EXEC_PROGRESS_STEP;
            g_unpack->outCount = 0;
            if (*g_unpack->in++ == 0)
                return 0;
        } while (consumed < g_unpack->inSize);
    }
    return 1;
}

// ================= encoder =================

// Opens a new 16-bit flag word at the end of the token stream. (PutZeroBit, PutOneBit, PutBits)
inline void StartFlagWord(void)
{
    g_flagWord = (unsigned short *)g_pack->out;
    g_flagMask = 1;
    *g_flagWord = 0;
    g_pack->out += 2;
    g_pack->outCount += 2;
}

// Appends a 0 flag bit to the token stream. (EncodeChunk, PutBits)
inline void PutZeroBit(void)
{
    g_flagMask <<= 1;
    if (g_flagMask == 0)
        StartFlagWord();
    *g_flagWord &= ~g_flagMask;
}

// Appends a 1 flag bit to the token stream. PutBits calls it (the original does not expand it there, unlike
// PutZeroBit), so it is not declared inline. (EncodeChunk, PutBits)
void PutOneBit(void)
{
    g_flagMask <<= 1;
    if (g_flagMask == 0)
        StartFlagWord();
    *g_flagWord |= g_flagMask;
}

// Appends the low `count` bits of `value`, most significant first. (EncodeChunk)
void PutBits(unsigned char value, int count)
{
    unsigned char bit = 0x80;
    for (int index = 7; index >= 0; index--) {
        if (index < count) {
            if (value & bit)
                PutOneBit();
            else
                PutZeroBit();
        }
        bit >>= 1;
    }
}

// Links the current position into the hash chains (after unlinking the one that just left the window) and moves
// on, once for every byte of the token just coded. (EncodeChunk)
void AdvanceOverToken(void)
{
    do {
        if (g_inPos >= EXEC_WINDOW_SPAN) {
            unsigned short expiredKey = *(const unsigned short *)(g_pack->in - EXEC_WINDOW_SPAN);
            unsigned short following = g_pack->next[(g_inPos - EXEC_WINDOW_SPAN) & EXEC_CHAIN_MASK];
            g_pack->head[expiredKey] = following;
            if (following == EXEC_NO_POSITION)
                g_pack->tail[expiredKey] = EXEC_NO_POSITION;
        }
        unsigned short key = *(const unsigned short *)g_pack->in;
        unsigned short *tailSlot = &g_pack->tail[key];
        unsigned short previous = *tailSlot;
        *tailSlot = g_inPos;
        if (previous != EXEC_NO_POSITION)
            g_pack->next[previous & EXEC_CHAIN_MASK] = g_inPos;
        else
            g_pack->head[key] = g_inPos;
        g_pack->next[g_inPos & EXEC_CHAIN_MASK] = EXEC_NO_POSITION;
        g_pack->in++;
        g_inPos++;
        g_matchLength--;
    } while (g_matchLength != 0);
}

// Compresses the whole chunk into the token stream: at every position find the longest run of one byte or the
// longest earlier match, emit one token, and move past it. (EncodeStart)
void EncodeChunk(void)
{
    g_flagWord = (unsigned short *)g_pack->out;
    *g_flagWord = 0;
    g_flagMask = EXEC_FIRST_MASK;
    g_pack->out += 2;
    g_pack->outCount += 2;
    unsigned short code;
    do {
        unsigned limit = g_pack->inSize - g_inPos;
        const unsigned char *scan = g_pack->in;
        unsigned char first = *scan;
        unsigned run = 0;
        if (limit >= EXEC_MAX_RUN_SCAN + 1)
            limit = EXEC_MAX_RUN_SCAN;
        if (limit > 0) {
            do {
                if (*scan++ != first)
                    break;
                run++;
            } while (run < limit);
        }
        g_matchLength = run;
        if (g_matchLength < EXEC_MIN_RUN)
            g_matchLength = 0;
        g_matchPos = EXEC_NO_POSITION;
        g_literal = *g_pack->in;
        if (g_matchLength < EXEC_GOOD_ENOUGH && g_inPos <= g_pack->inSize - 4) {
            unsigned short candidate = g_pack->head[*(const unsigned short *)g_pack->in];
            while (candidate != EXEC_NO_POSITION) {
                unsigned length = 2;
                do {
                    if (g_inPos + length >= g_pack->inSize ||
                        g_inBase[candidate + length] != g_pack->in[length])
                        break;
                    length++;
                } while (length < EXEC_MAX_MATCH);
                if ((unsigned short)length >= 2 && (unsigned short)length >= g_matchLength) {
                    g_matchLength = (unsigned short)length;
                    g_matchPos = candidate;
                }
                candidate = g_pack->next[candidate & EXEC_CHAIN_MASK];
            }
        }
        if (g_matchLength == 0) {
            PutZeroBit();
            *g_pack->out = *g_pack->in;
            g_pack->out++;
            g_pack->outCount++;
            g_matchLength = 1;
        } else {
            PutOneBit();
            if (g_matchPos != EXEC_NO_POSITION) {
                code = g_inPos - g_matchPos;
                if (code < 0x100)
                    PutZeroBit();
                else {
                    PutOneBit();
                    PutBits(code >> 8, 5);
                }
                *g_pack->out = code;
                g_pack->out++;
                g_pack->outCount++;
                switch (g_matchLength) {
                case 5:
                    PutZeroBit();
                case 4:
                    PutZeroBit();
                case 3:
                    PutZeroBit();
                case 2:
                    PutOneBit();
                    break;
                default:
                    PutBits(0, 4);
                    if (g_matchLength >= EXEC_MIN_RUN) {
                        PutZeroBit();
                        *g_pack->out = g_matchLength - EXEC_MIN_RUN;
                        g_pack->out++;
                        g_pack->outCount++;
                    } else {
                        PutOneBit();
                        PutBits(g_matchLength - 6, 3);
                    }
                }
            } else {
                PutBits(0x20, 6);
                *g_pack->out = EXEC_RUN_OFFSET;
                g_pack->out++;
                code = g_matchLength - EXEC_MIN_RUN;
                if (code < 0x10) {
                    PutZeroBit();
                    PutBits(code, 4);
                } else {
                    PutOneBit();
                    PutBits(code >> 8, 4);
                    *g_pack->out = code;
                    g_pack->out++;
                    g_pack->outCount++;
                }
                *g_pack->out = g_literal;
                g_pack->out++;
                g_pack->outCount += 2;
            }
        }
        AdvanceOverToken();
        if (g_pack->afterToken != 0)
            g_pack->afterToken(g_pack);
    } while (g_inPos < g_pack->inSize);
    PutBits(0x60, 7);
    *g_pack->out = 0;
    g_pack->out++;
    g_pack->outCount++;
}

// Resets the encoder state for a new chunk, reserves the two record-size bytes and runs EncodeChunk. (ExecPack)
int EncodeStart(ExecPackStream *stream)
{
    g_pack = stream;
    g_inBase = stream->in;
    g_inPos = 0;
    g_unusedCounter = 0;
    g_unusedHash = 0;
    g_matchLength = 0;
    g_literal = 0;
    g_matchPos = EXEC_NO_POSITION;
    stream->out += 2;
    g_pack->outCount += 2;
    EncodeChunk();
    return 0;
}

// Compresses `size` bytes at `source` into `dest`, chunk by chunk, and returns the packed size (cancelled: the size
// so far). Exported. Callers: BMPPack.
extern "C" int ExecPack(const unsigned char *source, unsigned char *dest, unsigned size)
{
    g_cancelled = 0;
    *(unsigned *)dest = size;
    dest += sizeof(unsigned);
    unsigned chunksLeft = (size + 0xffef) / EXEC_CHUNK_SIZE;
    *(unsigned *)dest = chunksLeft;
    dest += sizeof(unsigned);
    unsigned chunkCount = chunksLeft;
    ExecPackStream stream;
    stream.reserved = 8;
    stream.afterToken = 0;
    int packedSize = sizeof(ExecPackHeader);
    stream.next = g_nextTable;
    stream.head = g_headTable;
    stream.tail = g_tailTable;
    int remaining = size;
    if (chunksLeft == 0)
        return packedSize;
    do {
        chunksLeft--;
        if (remaining >= EXEC_CHUNK_SIZE) {
            memcpy(g_packChunk, source, EXEC_CHUNK_SIZE);
            stream.inSize = EXEC_CHUNK_SIZE;
            source += EXEC_CHUNK_SIZE;
            remaining -= EXEC_CHUNK_SIZE;
        } else {
            memcpy(g_packChunk, source, remaining);
            stream.inSize = remaining;
            source += remaining;
            remaining = 0;
        }
        // empty all three hash tables, 32 KB at a time
        memset(g_nextTable, 0xff, EXEC_CLEAR_BYTES);
        memset(g_nextTable + EXEC_CLEAR_ENTRIES, 0xff, EXEC_CLEAR_BYTES);
        memset(g_headTable, 0xff, EXEC_CLEAR_BYTES);
        memset(g_headTable + EXEC_CLEAR_ENTRIES, 0xff, EXEC_CLEAR_BYTES);
        memset(g_headTable + EXEC_CLEAR_ENTRIES * 2, 0xff, EXEC_CLEAR_BYTES);
        memset(g_headTable + EXEC_CLEAR_ENTRIES * 3, 0xff, EXEC_CLEAR_BYTES);
        memset(g_tailTable, 0xff, EXEC_CLEAR_BYTES);
        memset(g_tailTable + EXEC_CLEAR_ENTRIES, 0xff, EXEC_CLEAR_BYTES);
        memset(g_tailTable + EXEC_CLEAR_ENTRIES * 2, 0xff, EXEC_CLEAR_BYTES);
        memset(g_tailTable + EXEC_CLEAR_ENTRIES * 3, 0xff, EXEC_CLEAR_BYTES);
        stream.outCount = 0;
        stream.out = g_encodeBuffer;
        stream.in = g_packChunk;
        EncodeStart(&stream);
        *(unsigned short *)g_encodeBuffer = stream.outCount;
        memcpy(dest, g_encodeBuffer, stream.outCount);
        dest += stream.outCount;
        packedSize += stream.outCount;
        *dest++ = chunksLeft != 0;
        stream.out = 0;
        if (g_packProgress != 0)
            g_cancelled = g_packProgress(g_progressUser, stream.outCount,
                ((chunkCount - chunksLeft) * EXEC_PROGRESS_STEP + g_progressStage * EXEC_PROGRESS_STAGE) /
                (g_progressStageCount * EXEC_PROGRESS_STAGE_WEIGHT + chunkCount * EXEC_PROGRESS_CHUNK_WEIGHT));
        if (g_cancelled != 0)
            return packedSize;
        packedSize++;
    } while (chunksLeft != 0);
    return packedSize;
}

// Expands a buffer made by ExecPack into `dest`; returns the original size, 0 when the arguments are bad, the data is
// corrupt or the progress callback cancelled. Exported. Callers: BMPUnpack, MMV3.
extern "C" unsigned ExecUnpack(const unsigned char *packed, unsigned char *dest, unsigned packedSize)
{
    g_cancelled = 0;
    unsigned originalSize = *(const unsigned *)packed;
    packed += sizeof(unsigned);
    ExecUnpackStream stream;
    if (dest == 0)
        return 0;
    stream.inSize = packedSize;
    if (originalSize == 0 || originalSize > EXEC_MAX_UNPACKED_SIZE)
        return 0;
    g_unpackLimit = originalSize;
    stream.out = g_unpackChunk;
    g_unpackChunkStart = g_unpackChunk;
    g_unpackChunkCount = *(const unsigned *)packed;
    packed += sizeof(unsigned);
    stream.in = packed;
    g_unpackDest = dest;
    int result = UnpackRecords(&stream);
    return result != EXEC_ERROR_CORRUPT ? originalSize : 0;
}
