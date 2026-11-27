// mp3_stream.cpp : Mp3Stream, the MP3 file decoder of sound3d.dll (0x10001440-0x10001b8f).
//
// WaveStream (kind STREAM_MP3) keeps one Mp3Stream. LoadSource reads the whole .mp3 into memory, BuildWavHeader
// prepares the fake RIFF header, Read hands out header bytes first and decoded 16-bit PCM afterwards.
#include <stdlib.h>
#include <string.h>
#include "mp3_stream.h"

// Constructor: the file starts closed and the file name empty (the member constructors), then the two buffers are
// cleared. Called by WaveStream::Open (0x10004860) for MP3 names.
Mp3Stream::Mp3Stream() throw()
{
    frameBuffer = 0;
    source = 0;
}

// Frees the source image and the frame buffer and closes the file. Returns 1 when a file was open, else 0.
// Called by WaveStream::Close for MP3 streams and by Open.
int Mp3Stream::Close()
{
    if (source) {
        free(source);
    }
    source = 0;
    if (frameBuffer) {
        free(frameBuffer);
    }
    frameBuffer = 0;
    if (file.handle) {
        file.Close();
        return 1;
    }
    return 0;
}

// Fills the 44 byte RIFF header that Read serves in front of the decoded samples, from the format values
// the loader gathered. The header always advertises 16 bit stereo 44100 Hz, whatever the MP3 holds (the decoder
// resamples to that). Called by LoadSource.
void Mp3Stream::BuildWavHeader(const WavSourceFormat *format)
{
    unsigned int dataBytes;
    WORD blockAlign;

    dataBytes = frameCount * info.samplesPerFrame;
    blockAlign = (WORD)(((WORD)format->channels * (WORD)format->bitsPerSample) / 8);
    header.riffTag[2] = 'F';
    header.riffTag[3] = 'F';
    totalBytes = dataBytes + WAV_HEADER_BYTES;
    header.riffSize = dataBytes;
    header.riffTag[0] = 'R';
    header.riffTag[1] = 'I';
    header.bytesPerSecond = blockAlign * format->sampleRate;
    header.bitsPerSample = (WORD)format->bitsPerSample;
    header.channels = PCM_OUTPUT_CHANNELS;
    header.waveTag[0] = 'W';
    header.waveTag[1] = 'A';
    header.waveTag[2] = 'V';
    header.waveTag[3] = 'E';
    header.fmtTag[0] = 'f';
    header.fmtTag[1] = 'm';
    header.fmtTag[2] = 't';
    header.fmtTag[3] = ' ';
    header.channels = PCM_OUTPUT_CHANNELS;
    header.samplesPerSecond = PCM_OUTPUT_RATE;
    header.bytesPerSecond = PCM_OUTPUT_BYTES_PER_SECOND;
    header.blockAlign = PCM_OUTPUT_BLOCK_ALIGN;
    header.formatTag = 0;
    header.dataSize = dataBytes / ((format->bitsPerSample) / 8);
    header.fmtSize = WAV_FORMAT_CHUNK_SIZE;
    header.bitsPerSample = PCM_OUTPUT_BITS;
}

// Reads the whole .mp3 into memory, builds the WAV header, allocates the frame buffer and starts the decoder on the
// loaded data; the file is closed afterwards. Called by WaveStream::Open (0x10004860).
void Mp3Stream::LoadSource()
{
    WavSourceFormat format;
    unsigned int frameBytes;

    file.Seek(0, SEEK_SET);
    sourceSize = dataLength;
    if (source) {
        free(source);
    }
    source = operator new(dataLength);
    sourcePtr = (unsigned char *)source;
    sourceBytesRead = file.Read((HPSTR)source, 1, sourceSize);
    frameBytes = info.samplesPerFrame;
    format.channels = info.channels;
    format.bitsPerSample = PCM_OUTPUT_BITS;
    format.sampleRate = info.sampleRate;
    format.bytesPerFrame = frameBytes;
    BuildWavHeader(&format);
    if (frameBuffer) {
        free(frameBuffer);
    }
    frameBuffer = operator new(frameBytes);
    mp3_decoder_start(sourcePtr, dataLength);
    file.Close();
}
