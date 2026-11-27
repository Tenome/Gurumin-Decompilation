// exec_pack.h : Packs.dll's general-purpose compressor (ExecPack / ExecUnpack), a 16-bit-flag LZ77 variant.
//
// Packed layout (little endian):
//   ExecPackHeader { originalSize, chunkCount }
//   then chunkCount records of  { ushort recordSize; recordSize - 2 bytes of token stream; byte moreChunksFollow }
// Every chunk compresses up to EXEC_CHUNK_SIZE source bytes on its own (the dictionary restarts per chunk).
//
// Token stream, read one bit at a time from 16-bit words (low bit first); the first word is read from its upper
// byte only, so the stream starts with 8 bits:
//   0                    literal byte
//   1 0  <offset byte>   copy from 1..255 bytes back, then a length code
//   1 1  <5 bits> <offset byte>
//                        13-bit offset 2..8191 back, then a length code
//                        offset 1 = run of one repeated byte (see below), offset 0 = end of chunk
// Length code: 1 = 2 bytes, 01 = 3, 001 = 4, 0001 = 5, 00001 + 3 bits = 6..13, 00000 + byte = 14..269.
// Run (offset 1): 0 + 4 bits, or 1 + 4 bits + byte, gives the run length minus 14; the byte to repeat follows.
#ifndef EXEC_PACK_H
#define EXEC_PACK_H

// Source bytes compressed per chunk.
const int EXEC_CHUNK_SIZE = 0x7ff0;

// Largest original size ExecUnpack accepts (4,120,832 bytes).
const unsigned EXEC_MAX_UNPACKED_SIZE = 0x3ee100;

// ExecUnpack's block-corrupt result inside the decoder (a chunk would overrun the original size).
const int EXEC_ERROR_CORRUPT = 0x29a;

// First eight bytes of a packed buffer.
struct ExecPackHeader {
    unsigned originalSize;  // size of the data before packing
    unsigned chunkCount;    // number of chunk records that follow
};

// A position stays in the hash chains for this many bytes; older ones are unlinked as the encoder advances.
const unsigned EXEC_WINDOW_SPAN = 0x1fff;

// Chain links are positions masked with this (the chain table holds one more entry).
const unsigned EXEC_CHAIN_MASK = 0x7fff;

// Chain terminator / empty hash slot.
const unsigned short EXEC_NO_POSITION = 0xffff;

// Encoder state shared with the helpers: one chunk being compressed. The three tables are chained hash tables
// keyed by the two bytes at a position.
struct ExecPackStream {
    unsigned char *out;          // next free byte of the token stream
    const unsigned char *in;     // next source byte of the chunk
    unsigned short *next;        // next[position & EXEC_CHAIN_MASK]: the following position with the same two bytes
    unsigned short *head;        // head[two bytes]: oldest position still in the window, EXEC_NO_POSITION when none
    unsigned short *tail;        // tail[two bytes]: newest position
    unsigned reserved;           // set to 8 by ExecPack, never read
    unsigned outCount;           // bytes written to the token stream
    unsigned inSize;             // size of the chunk in bytes
    void (*afterToken)(ExecPackStream *stream);  // optional hook called after every token (always 0)
};

// Decoder state: one chunk record being expanded.
struct ExecUnpackStream {
    unsigned char *out;          // next free byte of the expanded chunk
    const unsigned char *in;     // next unread byte of the packed data
    unsigned outCount;           // bytes expanded so far in this chunk
    unsigned inSize;             // size of the whole packed buffer
    unsigned recordSize;         // size of the chunk record being read
    unsigned reserved;           // never used here; the original frame holds 0x18 bytes, so the stream keeps a sixth field like ExecPackStream
};

// Flag-bit reader of the decoder. It has to be one struct: as three separate globals cl keeps the bit buffer in a
// register inside GetBit, the original reads and writes it in memory.
struct BitReader {
    unsigned short bits;    // bit buffer, next flag in the low bit
    unsigned char count;    // bits left in the buffer
    int consumed;           // packed bytes consumed in the chunk (written, never read)
};

// Progress callback shared by ExecPack and ExecUnpack: (user data, bytes of the current chunk, percent of the job).
// A non-zero result aborts the job.
typedef int (*ExecProgressFn)(void *user, unsigned chunkBytes, unsigned percent);

// ---- shared state (file-scope data of exec_pack.cpp) ----
extern ExecUnpackStream *g_unpack;  // chunk being expanded
extern BitReader g_reader;
extern unsigned char *g_unpackChunkStart;  // start of g_unpackChunk, where every chunk is expanded
extern unsigned char *g_unpackDest;  // caller's buffer, next free byte
extern unsigned g_unpackLimit;  // original size: no chunk may grow past it
extern unsigned g_unpackChunkCount;  // chunk count read from the header
extern unsigned char g_unpackChunk[EXEC_CHUNK_SIZE];
extern ExecPackStream *g_pack;  // chunk being compressed
extern const unsigned char *g_inBase;  // start of the chunk
extern unsigned g_inPos;  // offset of the next source byte in the chunk
extern unsigned g_unusedCounter;  // cleared at the start of a chunk, never used
extern unsigned short g_unusedHash;  // cleared at the start of a chunk, never used
extern unsigned g_matchLength;  // length of the token being coded
extern unsigned g_literal;  // first byte at the current position (the byte a run repeats)
extern unsigned g_matchPos;  // chunk position the token copies from, EXEC_NO_POSITION for a run
extern unsigned short *g_flagWord;  // flag word being filled in the token stream
extern unsigned short g_flagMask;  // bit of g_flagWord the next flag goes to
extern unsigned char g_encodeBuffer[EXEC_CHUNK_SIZE * 2];  // token stream of one chunk
extern unsigned char g_packChunk[EXEC_CHUNK_SIZE];  // source bytes of one chunk
extern unsigned short g_nextTable[EXEC_CHAIN_MASK + 1];
extern unsigned short g_headTable[0x10000];
extern unsigned short g_tailTable[0x10000];
extern ExecProgressFn g_packProgress;
extern ExecProgressFn g_unpackProgress;
extern void *g_progressUser;
extern unsigned g_progressStage;
extern unsigned g_progressStageCount;
extern int g_cancelled;  // set by a progress callback to abort

// Compresses `size` bytes at `source` into `dest` (sized for the worst case); returns the packed size in bytes.
extern "C" int ExecPack(const unsigned char *source, unsigned char *dest, unsigned size);

// Expands a packed buffer of `packedSize` bytes into `dest`; returns the original size, or 0 on failure.
extern "C" unsigned ExecUnpack(const unsigned char *packed, unsigned char *dest, unsigned packedSize);

#endif
