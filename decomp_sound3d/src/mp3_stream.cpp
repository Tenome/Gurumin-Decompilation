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
    unsigned int sampleCount;
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
    sampleCount = dataBytes / ((format->bitsPerSample) / 8);
    header.channels = PCM_OUTPUT_CHANNELS;
    header.samplesPerSecond = PCM_OUTPUT_RATE;
    header.bytesPerSecond = PCM_OUTPUT_BYTES_PER_SECOND;
    header.blockAlign = PCM_OUTPUT_BLOCK_ALIGN;
    header.formatTag = 0;
    header.dataSize = sampleCount;
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

// Hands out count bytes of the presented stream into buffer: the 44 byte WAV header first, then decoded 16-bit PCM,
// decoding frame after frame. A request at a position other than the one the decoder state belongs to restarts the
// decoder and decodes forward to it. Short requests (below MP3_SMALL_READ_LIMIT) sleep 2 ms first and decode into
// the frame buffer; long ones decode straight into the caller's buffer. When the data runs out the rest of buffer is
// zeroed. Returns count (the header read returns 0x2c). Called by WaveStream::Read (0x10004c20) for MP3 streams.
unsigned int Mp3Stream::Read(BYTE *buffer, unsigned int count)
{
    int remaining = count;
    int skipped;
    int step;
    int delivered;

    if (position < WAV_HEADER_BYTES) {
        memcpy(buffer, &header, sizeof(header));
        position += WAV_HEADER_BYTES;
        lastPosition = position;
        decoder.outputUsed = 0;
        return WAV_HEADER_BYTES;
    }
    if (position == WAV_HEADER_BYTES) {
        decoder.outputUsed = 0;
        sourcePtr = (unsigned char *)source;
        mp3_decoder_start(source, sourceBytesRead);
        decoder.inputPtr = sourcePtr;
        decoder.inputSize = MP3_DECODE_WINDOW;
        lastPosition = position;
    }
    if (lastPosition != position) {
        decoder.outputUsed = 0;
        sourcePtr = (unsigned char *)source;
        mp3_decoder_start(source, sourceBytesRead);
        decoder.inputSize = MP3_DECODE_WINDOW;
        decoder.inputPtr = sourcePtr;
        lastPosition = position;
        decoder.outputPtr = (unsigned char *)frameBuffer;
        skipped = WAV_HEADER_BYTES;
        while (skipped < (int)position) {
            decoder.inputPtr = sourcePtr;
            decoder.inputSize = MP3_DECODE_WINDOW;
            mp3_decode_frame(&decoder);
            sourcePtr += decoder.inputSize;
            skipped += decoder.outputSize;
        }
        decoder.outputUsed = decoder.outputSize - skipped + position;
        lastPosition = position;
    }
    // One pass per block of output: a block delivered whole advances position itself in every branch (the compiler
    // merges the identical tails, as it does for the pending-block copies), a block that does not fit returns after
    // a partial copy, and when the data runs out the rest of buffer is zeroed after the loop.
    for (;;) {
        if (position + remaining >= header.riffSize) {
            break;
        }
        if (remaining >= MP3_SMALL_READ_LIMIT) {
            if (decoder.outputUsed == 0) {
                int frameOffset;

                decoder.outputPtr = buffer + (count - remaining);
                decoder.inputSize = MP3_DECODE_WINDOW;
                while (!mp3_decode_frame(&decoder)) {
                    sourcePtr++;
                    while (!Mp3FindFrameStart(sourcePtr, sourceBytesRead, &frameOffset)) {
                    }
                    sourcePtr += frameOffset;
                    decoder.inputPtr = sourcePtr;
                }
                sourcePtr += decoder.inputSize;
                decoder.inputPtr = sourcePtr;
                step = decoder.outputSize;
                if (step > remaining) {
                    return count;
                }
                remaining -= step;
                position += step;
                lastPosition = position;
            } else {
                decoder.outputPtr = (unsigned char *)frameBuffer;
                if (decoder.outputSize > remaining) {
                    if (remaining != 0) {
                        memcpy(buffer + (count - remaining), decoder.outputPtr + decoder.outputUsed, remaining);
                    }
                    position += remaining;
                    lastPosition = position;
                    decoder.outputUsed += remaining;
                    return count;
                }
                memcpy(buffer + (count - remaining), decoder.outputPtr + decoder.outputUsed,
                       decoder.outputSize - decoder.outputUsed);
                remaining += decoder.outputUsed - decoder.outputSize;
                step = decoder.outputSize - decoder.outputUsed;
                decoder.outputUsed = 0;
                position += step;
                lastPosition = position;
            }
        } else {
            Sleep(MP3_SMALL_READ_SLEEP_MS);
            if (decoder.outputUsed == 0) {
                int frameOffset;

                decoder.inputSize = MP3_DECODE_WINDOW;
                decoder.outputPtr = (unsigned char *)frameBuffer;
                while (!mp3_decode_frame(&decoder)) {
                    sourcePtr++;
                    while (!Mp3FindFrameStart(sourcePtr, sourceBytesRead, &frameOffset)) {
                    }
                    sourcePtr += frameOffset;
                    decoder.inputPtr = sourcePtr;
                }
                sourcePtr += decoder.inputSize;
                decoder.inputPtr = sourcePtr;
                if (decoder.outputSize > remaining) {
                    if (remaining != 0) {
                        memcpy(buffer + (count - remaining), decoder.outputPtr, remaining);
                    }
                    decoder.outputUsed += remaining;
                    position += remaining;
                    lastPosition = position;
                    return count;
                }
                memcpy(buffer + (count - remaining), decoder.outputPtr, decoder.outputSize);
                step = decoder.outputSize;
                remaining -= step;
                position += step;
                lastPosition = position;
            } else {
                if (decoder.outputSize > remaining) {
                    if (remaining != 0) {
                        memcpy(buffer + (count - remaining), decoder.outputPtr + decoder.outputUsed, remaining);
                    }
                    position += remaining;
                    lastPosition = position;
                    decoder.outputUsed += remaining;
                    return count;
                }
                memcpy(buffer + (count - remaining), decoder.outputPtr + decoder.outputUsed,
                       decoder.outputSize - decoder.outputUsed);
                remaining += decoder.outputUsed - decoder.outputSize;
                step = decoder.outputSize - decoder.outputUsed;
                decoder.outputUsed = 0;
                position += step;
                lastPosition = position;
            }
        }
    }
    memset(buffer + (count - remaining), 0, remaining);
    return count;
}
