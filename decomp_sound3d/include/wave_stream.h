// wave_stream.h : WaveStream, the reader a sound channel pulls its PCM data from (0x1a4 bytes), and its constants.
//
// A stream is opened by file name and reads a WAV file, ADPCM block files through ACM, or an MP3 / Ogg Vorbis file
// through a decoder object (Mp3Stream, OggStream). The small members (constructor, FreeBuffers, Seek, Tell, AtEnd,
// Close) are in sound_file.cpp, the openers and Read in wave_stream.cpp, CheckKey in file_name_utils.cpp.
#ifndef WAVE_STREAM_H
#define WAVE_STREAM_H

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmreg.h>
#include <msacm.h>
#include "mmio_file.h"
#include "wav_header.h"
#include "mp3_stream.h"
#include "ogg_stream.h"
#include "file_name_utils.h"
#include "tracked_allocator.h"

// Which decoder the stream uses (WaveStream::kind).
enum WaveStreamKind {
    STREAM_PLAIN = 0,       // PCM or ADPCM data read straight from the file
    STREAM_MP3 = 4,         // Mp3Stream ('c', 'C' and '3' file names)
    STREAM_OGG = 5          // OggStream ('g' and 'G' file names)
};

// Sizes of the buffers and formats PrepareBuffers creates.
const int FORMAT_BUFFER_BYTES = 0x400;          // each WAVEFORMATEX is allocated with room for extra data
const int CONVERT_BUFFER_BYTES = 0x1da60;       // the compressed and the decoded block buffer
const int ACM_SOURCE_BYTES = 0x5000;            // compressed bytes handed to the ACM stream per conversion
const int ACM_DESTINATION_BYTES = 0x1c6d8;      // decoded bytes one conversion may produce
const int ADPCM_BLOCK_ALIGN = 0x800;
const DWORD ADPCM_BYTES_PER_SECOND = 0xad47;
const WORD ADPCM_BITS = 4;
const WORD ADPCM_EXTRA_BYTES = 0x20;

// Size and decoded size of one block of the 'enc'/'dec' compressed files: every block stores a 4 byte length and
// that many ADPCM bytes and decodes to ADPCM_DECODED_BLOCK_BYTES of 16 bit stereo PCM.
const int ADPCM_DECODED_BLOCK_BYTES = 0x13e20;
// Every decoded block starts with this many bytes of ACM output that are not part of the sound.
const int ADPCM_DECODED_SKIP = 0x16c0;
// ACM stream flags used when a block is converted (block align, start and end of stream).
const DWORD ACM_CONVERT_ONE_BLOCK = ACM_STREAMCONVERTF_BLOCKALIGN | ACM_STREAMCONVERTF_START | ACM_STREAMCONVERTF_END;
// Largest compressed block ReadEncoded accepts.
const int ENC_MAX_BLOCK_BYTES = 0x5000;
const int ADPCM_BLOCK_OVERHEAD = 8;             // bytes of the per block length field and padding

// Where the 'data' chunk tag is searched for in a WAV file, and how far the tag lies behind the chunk start.
const int WAV_DATA_SCAN_BYTES = 0x78;
const int WAV_DATA_TAG_OFFSET = 0x27;
// Offsets inside the 'enc' file header and the bytes before its first block.
const int ENC_HEADER_SKIP = 0x10;
const int ENC_BLOCKS_START = 0x2c;

// Bytes of the base name buffer OpenEncoded hands to GetBaseFileName.
const int BASE_NAME_BYTES = 32;

class WaveStream {
public:
    int prepared;                       // +0x000 1 once PrepareBuffers has run (FreeBuffers does nothing before)
    WAVEFORMATEX *adpcmFormat;          // +0x004 ADPCM source format (FORMAT_BUFFER_BYTES, ACM suggests the rest)
    WAVEFORMATEX *pcmFormat;            // +0x008 PCM destination of adpcmFormat
    WAVEFORMATEX *encodedFormat;        // +0x00c format ACM suggests for pcmFormat2
    WAVEFORMATEX *pcmFormat2;           // +0x010 PCM source for encodedFormat
    WavHeader header;                   // +0x014 header presented at the start of the stream
    HACMSTREAM decodeStream;            // +0x040 ACM stream adpcmFormat -> pcmFormat, open while converting
    HACMSTREAM encodeStream;            // +0x044 ACM stream encodedFormat -> pcmFormat2
    HACMSTREAM spareStream;             // +0x048 third ACM stream, closed by FreeBuffers (no opener found yet)
    ACMSTREAMHEADER spareHeader;        // +0x04c header of spareStream
    ACMSTREAMHEADER decodeHeader;       // +0x0a0
    ACMSTREAMHEADER encodeHeader;       // +0x0f4
    BYTE *decodedBuffer;                // +0x148 CONVERT_BUFFER_BYTES, decoded blocks
    BYTE *compressedBuffer;             // +0x14c CONVERT_BUFFER_BYTES, compressed blocks
    int m_0150;                         // +0x150
    int filePosition;                   // +0x154 current read position in the file
    int kind;                           // +0x158 WaveStreamKind
    MmioFile file;                      // +0x15c the file being read
    DWORD dataSize;                     // +0x16c size of the stream (header included) in bytes
    DWORD position;                     // +0x170 read position in the stream (non-WAV streams)
    int ready;                          // +0x174 non-zero when a file is open and sized
    int isWav;                          // +0x178 the file is a WAV file ('data' chunk found): read straight from it
    int m_017c;                         // +0x17c
    BYTE *fileCopy;                     // +0x180 in-memory copy of a WAV file, freed by Close and the destructor
    int blockOffset;                    // +0x184 read position inside the decoded block
    int seekPending;                    // +0x188 1: the next Read walks the blocks from the file start to position
                                        //        (set by Open for a plain file and by Seek)
    int isCompressed;                   // +0x18c 1 when the data is a sequence of ADPCM blocks
    int adpcmBlockBytes;                // +0x190 bytes of one compressed block
    int dataOffset;                     // +0x194 offset of the data chunk in a WAV file
    int m_0198;                         // +0x198
    int headerOffset;                   // +0x19c bytes before the first block in a compressed file
    void *decoder;                      // +0x1a0 Mp3Stream or OggStream, by kind

    WaveStream();                               // 0x100029e0
    ~WaveStream();                              // 0x100040d0
    int FreeBuffers();                          // 0x10002a30
    void Seek(unsigned int newPosition);        // 0x10002b30
    int Tell();                                 // 0x10002c50
    bool AtEnd();                               // 0x10002d30
    int CheckKey(unsigned char *name, int expectedSum, unsigned int expectedXor);   // 0x10002e70
    int Close();                                // 0x100030a0
    int PrepareBuffers(int keepEmpty);          // 0x10004140
    int OpenWav(const char *path);              // 0x10004390
    int OpenEncoded(const char *path);          // 0x10004590
    int OpenDecoded(const char *path);          // 0x10004750
    int Open(const char *path);                 // 0x10004860
    int Read(BYTE *buffer, int bytes);          // 0x10004c20
    int DecodeNextBlock();                      // inline: reads one compressed block and converts it (dec files)
};

#endif
