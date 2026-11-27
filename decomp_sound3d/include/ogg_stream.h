// ogg_stream.h : OggStream, sound3d.dll's Ogg Vorbis file decoder (the decoder object of WaveStream kind STREAM_OGG).
//
// Open loads the whole .ogg file into memory (fileData, fileSize bytes); pages are fed to libogg 4096 bytes at a
// time and the decoded float samples are converted to 16-bit PCM scaled by a gain. Read presents the result as a
// WAV file: a synthesised 44 byte header, then the PCM.
#ifndef OGG_STREAM_H
#define OGG_STREAM_H

#include "mmio_file.h"
#include "wav_header.h"
#include "ogg_vorbis_types.h"

// Bytes handed to the libogg sync layer per feed.
const int OGG_FEED_BYTES = 0x1000;
// 16-bit samples held by the conversion buffer (8 KB).
const int OGG_PCM_SAMPLES = 0x1000;
// Size of the PCM buffer one decode step fills; Decode copies nothing once the 16-bit result would reach it.
const int OGG_PCM_BUFFER_BYTES = 240000;
// Header packets a stream starts with (identification, comment, setup); two follow the first.
const int OGG_EXTRA_HEADER_PACKETS = 2;
// Size of the file name buffer kept for reopening.
const int OGG_PATH_BYTES = 0x400;
// The reader reports a length three seconds longer than the file needs.
const int OGG_EXTRA_SECONDS = 3;

class OggStream {
public:
    int totalBytes;                   // +0x0000 header plus presented data size
    MmioFile file;                    // +0x0004 the file being read
    BYTE *pcmBuffer;                  // +0x0014 decoded PCM of the current step (OGG_PCM_BUFFER_BYTES), freed by Close
    WavHeader header;                 // +0x0018 the header presented at the start of the stream
    int m_0044;                       // +0x0044
    VorbisDspState dsp;               // +0x0048 synthesis state
    VorbisBlock block;                // +0x00b8 synthesis work area
    OggSyncState sync;                // +0x0128 raw file bytes
    OggPage page;                     // +0x0144 current page
    int m_0154;                       // +0x0154 (padding in front of the 8-byte aligned stream state)
    OggStreamState stream;            // +0x0158 packet reassembly
    OggPacket packet;                 // +0x02c0 current packet
    VorbisInfo info;                  // +0x02e0 stream parameters
    VorbisComment comment;            // +0x0300 tags
    short pcm[OGG_PCM_SAMPLES];       // +0x0310 converted samples of one chunk
    int samplesPerChunk;              // +0x2310 OGG_FEED_BYTES / channels
    char m_2314;                      // +0x2314
    char isOpen;                      // +0x2315 libvorbis state initialised
    char m_2316[0x2324 - 0x2316];     // +0x2316
    char path[OGG_PATH_BYTES];        // +0x2324 the name the file was opened with
    float gain;                       // +0x2724 volume factor applied to the 16-bit samples, 1.0 after Open
    int position;                     // +0x2728 bytes delivered so far, header included
    int blockPosition;                // +0x272c position the current PCM block was decoded for
    int blockOffset;                  // +0x2730 read position inside the current PCM block
    int m_2734;                       // +0x2734 (cleared by reads and seeks)
    int blockLength;                  // +0x2738 bytes in the current PCM block
    BYTE *fileData;                   // +0x273c the whole compressed file in memory
    BYTE *readPtr;                    // +0x2740 next byte of fileData to hand to libogg
    int fileSize;                     // +0x2744 bytes in fileData

    // Starts closed (the member constructor), then clears the fields Open and Close test. Expanded in place by
    // WaveStream::Open (0x10004860); the out-of-line copy (0x10001bc0) is never called.
    OggStream()
    {
        pcmBuffer = 0;
        fileData = 0;
        isOpen = 0;
    }
    int FeedChunk();                  // 0x10001be0 feeds libogg, 0 at the end of the file
    int StartDecoder();               // 0x10001ce0 parses the header packets and sets up the synthesis state
    int RewindDecoder();              // 0x10001ec0 restarts the decoder at the first page (headers only)
    int Decode(BYTE *output);         // 0x100024b0 decodes one block of PCM, returns its bytes
    int ReleaseReader();              // 0x100028d0
    int Release();                    // 0x10002900
    int Close();                      // 0x10002970 frees the buffers and releases the decoder
    int Open(const char *path);       // 0x10003cb0
    int Read(BYTE *buffer, int bytes);   // 0x10003e80
};

#endif
