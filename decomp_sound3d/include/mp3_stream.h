// mp3_stream.h : Mp3Stream, sound3d.dll's MP3 file decoder (the decoder object of WaveStream kind STREAM_MP3).
//
// Open finds the first MPEG frame and measures the file; LoadSource reads the whole .mp3 into memory and starts the
// built-in MPEG decoder (0x1000a8a0-0x1000b370). Callers see a WAV file: the first 44 bytes are a synthesised RIFF
// header, the rest is the decoded 16-bit PCM.
#ifndef MP3_STREAM_H
#define MP3_STREAM_H

#include <stdlib.h>
#include "mmio_file.h"
#include "wav_header.h"

// Bytes of source data the decoder is handed per frame (the decoder reports how many it used).
const int MP3_DECODE_WINDOW = 0x10376;
// Reads shorter than this many bytes are served after a 2 ms sleep (lets the loader thread breathe).
const int MP3_SMALL_READ_LIMIT = 6000;
const DWORD MP3_SMALL_READ_SLEEP_MS = 2;
// Number of bytes of the file start scanned for the first MPEG frame.
const int MP3_PROBE_BYTES = 0x200;

// The fields of one MPEG audio frame header, filled by Mp3ParseFrameHeader (0x1000abe1) for the first frame. The
// parser writes nothing past +0x54.
struct Mp3FrameInfo {
    int version;            // +0x00 0 = MPEG 1, 1 = MPEG 2, 2 = MPEG 2.5 (as the parser stores it)
    int layer;              // +0x04 1..3
    int noCrc;              // +0x08 1 when the frame has no CRC
    int bitrateIndex;       // +0x0c index into the bitrate table
    int frequencyIndex;     // +0x10 index into the sampling frequency table
    int padding;            // +0x14
    int privateBit;         // +0x18
    int channelMode;        // +0x1c 3 = mono
    int modeExtension;      // +0x20
    int copyright;          // +0x24
    int original;           // +0x28
    int emphasis;           // +0x2c
    int channels;           // +0x30
    int m_0034;             // +0x34
    int sampleRate;         // +0x38 samples per second
    int bitrate;            // +0x3c bits per second
    int frameCount;         // +0x40 number of frames, 0 = unknown
    int m_0044;             // +0x44
    int m_0048;             // +0x48
    int frameBytes;         // +0x4c bytes per frame
    int m_0050;             // +0x50
    int samplesPerFrame;    // +0x54 also the size of the decoded frame buffer LoadSource allocates
};

// Format values the loader gathers from the MP3 before the header is built (four ints in this order).
struct WavSourceFormat {
    int channels;
    int bitsPerSample;
    int sampleRate;
    unsigned int bytesPerFrame;
};

// State of the MPEG decoder; only the fields the stream touches are named.
struct Mp3DecoderState {
    char m_0000[0x34];
    unsigned char *inputPtr;    // +0x34 where decoding starts
    int inputSize;              // +0x38 bytes available / bytes consumed by the decoder
    unsigned char *outputPtr;   // +0x3c buffer the frame is decoded into
    int outputSize;             // +0x40 bytes one decoded frame holds
    int outputUsed;             // +0x44 bytes of the current frame already delivered
};

// Decoder functions in the MP3 library part of the DLL (0x1000a800 and up).
void Mp3ResetDecoder();                                                      // 0x1000a8a0
void Mp3SetFlag(int enable);                                                 // 0x1000b370
int Mp3FindFrameStart(const BYTE *data, int length, int *frameOffset);       // 0x1000aade, 0 = not found
int Mp3ParseFrameHeader(const BYTE *frame, int length, Mp3FrameInfo *info);  // 0x1000abe1
extern "C" {
void mp3_decoder_start(void *data, int size);                      // 0x1000b154
int mp3_decode_frame(Mp3DecoderState *state);                      // 0x1000b1ed, 0 = needs sync
}

// A heap string owned by its holder (the file name a decoder remembers). The vtable at 0x10061870 holds only the
// scalar deleting destructor (0x10003a80).
class OwnedText {
public:
    char *text;     // malloc'ed, freed by the destructor

    OwnedText() { text = NULL; }
    virtual ~OwnedText() { free(text); }
};

// MP3 file reader (0x178 bytes; constructed by WaveStream::Open, opened by Open).
class Mp3Stream {
public:
    int totalBytes;                 // +0x000 header plus decoded data bytes, set by LoadSource
    MmioFile file;                  // +0x004 the file being read
    int firstFrameOffset;           // +0x014 offset of the first MPEG frame inside the file
    Mp3FrameInfo info;              // +0x018 header fields of the first frame
    int sourceSize;                 // +0x070 copy of dataLength
    int sourceBytesRead;            // +0x074 what the file read returned
    void *source;                   // +0x078 the whole .mp3 file, freed by Close
    unsigned char *sourcePtr;       // +0x07c decoder position inside source
    int m_0080;                     // +0x080 0x80 after Open
    void *frameBuffer;              // +0x084 decoded frame (info.samplesPerFrame bytes), freed by Close
    unsigned int position;          // +0x088 bytes handed out so far (header included)
    unsigned int lastPosition;      // +0x08c position the decoder state corresponds to
    OwnedText fileName;             // +0x090 the file name without its directory
    int formatChannels;             // +0x098 copy of info.channels
    int formatSampleRate;           // +0x09c copy of info.sampleRate
    int formatBitrate;              // +0x0a0 copy of info.bitrate
    int frameCount;                 // +0x0a4 decoded size is frameCount * info.samplesPerFrame (computed from the
                                    //        file size when the header has none)
    int pcmBytes;                   // +0x0a8 size of the decoded data
    int seconds;                    // +0x0ac pcmBytes / sampleRate
    char m_00b0[0x100 - 0xb0];      // +0x0b0
    Mp3DecoderState decoder;        // +0x100
    int dataLength;                 // +0x148 file length minus the bytes before the first frame
    WavHeader header;               // +0x14c header presented in front of the decoded samples

    Mp3Stream() throw();                                    // 0x10001440
    int Close();                                            // 0x10001470
    void BuildWavHeader(const WavSourceFormat *format);     // 0x10001500
    void LoadSource();                                      // 0x10001630 sets totalBytes
    unsigned int Read(BYTE *buffer, unsigned int count);    // 0x10001710
    int Open(const char *path);                             // 0x10003ab0
};

#endif
