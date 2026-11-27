// wav_header.h : the canonical RIFF/WAVE header every sound3d.dll stream presents at its start, and the PCM format
// the decoders deliver.
#ifndef WAV_HEADER_H
#define WAV_HEADER_H

#include <windows.h>

// The canonical 44 byte header of a PCM WAV file. The streams read it from a WAV file or synthesise it in front of
// decoded data; a sound channel keeps a copy of the one of the file it plays.
struct WavHeader {
    char riffTag[4];            // +0x00 "RIFF"
    DWORD riffSize;             // +0x04
    char waveTag[4];            // +0x08 "WAVE"
    char fmtTag[4];             // +0x0c "fmt "
    DWORD fmtSize;              // +0x10 chunk size, 16
    WORD formatTag;             // +0x14 1 = PCM
    WORD channels;              // +0x16
    DWORD samplesPerSecond;     // +0x18
    DWORD bytesPerSecond;       // +0x1c
    WORD blockAlign;            // +0x20
    WORD bitsPerSample;         // +0x22
    char dataTag[4];            // +0x24 "data"
    DWORD dataSize;             // +0x28
};

// Size of the header, the offset of the first sample byte of every stream.
const int WAV_HEADER_BYTES = 0x2c;
// Size of the "fmt " chunk of the synthesised headers.
const DWORD WAV_FORMAT_CHUNK_SIZE = 16;

const int BITS_PER_BYTE = 8;

// The format the MP3 and Ogg decoders deliver and the ACM conversions produce: 44.1 kHz, 16 bit, stereo.
const WORD PCM_OUTPUT_CHANNELS = 2;
const WORD PCM_OUTPUT_BITS = 16;
const WORD PCM_OUTPUT_BLOCK_ALIGN = 4;
const DWORD PCM_OUTPUT_RATE = 44100;
const DWORD PCM_OUTPUT_BYTES_PER_SECOND = 176400;

#endif
