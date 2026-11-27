// ogg_source.cpp : OggStream::Open and OggStream::Read, the Ogg Vorbis reader of sound3d.dll (0x10003cb0 and
// 0x10003e80).
#include "ogg_stream.h"
#include <string.h>

// Opens an Ogg Vorbis file: reads the whole file into memory, parses the Vorbis headers and builds the WAV header
// the stream presents in front of the decoded data. The presented length is the file length converted with the
// nominal bitrate plus three seconds of 44.1 kHz, 16 bit stereo. Returns 1 on success, 0 when the file cannot be
// opened. Called by WaveStream::Open (0x10004860) for names ending in 'g' or 'G'.
int OggStream::Open(const char *fileName)
{
    int length;
    DWORD dataBytes;

    gain = 1.0f;
    Close();
    if (pcmBuffer == NULL) {
        pcmBuffer = new BYTE[OGG_PCM_BUFFER_BYTES];
    }
    strcpy(path, fileName);
    if (!file.Open(fileName, "rb")) {
        return 0;
    }
    file.Seek(0, SEEK_END);
    length = file.Tell();
    fileSize = length;
    file.Seek(0, SEEK_SET);
    if (fileData == NULL) {
        fileData = new BYTE[fileSize];
    }
    file.Read((HPSTR)fileData, 1, fileSize);
    file.Close();
    StartDecoder();
    header.riffTag[2] = 'F';
    header.riffTag[3] = 'F';
    header.riffTag[0] = 'R';
    header.riffTag[1] = 'I';
    header.waveTag[0] = 'W';
    header.waveTag[1] = 'A';
    header.waveTag[2] = 'V';
    header.waveTag[3] = 'E';
    header.fmtTag[0] = 'f';
    header.fmtTag[1] = 'm';
    header.fmtTag[2] = 't';
    header.fmtTag[3] = ' ';
    header.samplesPerSecond = PCM_OUTPUT_RATE;
    header.bytesPerSecond = PCM_OUTPUT_BYTES_PER_SECOND;
    header.blockAlign = PCM_OUTPUT_BLOCK_ALIGN;
    header.formatTag = 0;
    header.fmtSize = WAV_FORMAT_CHUNK_SIZE;
    header.channels = PCM_OUTPUT_CHANNELS;
    header.bitsPerSample = PCM_OUTPUT_BITS;
    position = 0;
    blockPosition = 0;
    dataBytes = (fileSize * BITS_PER_BYTE / info.bitrateNominal + OGG_EXTRA_SECONDS) * PCM_OUTPUT_BYTES_PER_SECOND;
    header.riffSize = dataBytes;
    totalBytes = dataBytes + WAV_HEADER_BYTES;
    header.dataSize = dataBytes / 2;
    return 1;
}

// Copies up to bytes bytes of the presented stream (WAV header, then decoded PCM) into buffer, decoding block after
// block. A read at a position other than the one the current block was decoded for rewinds the decoder and decodes
// forward to that position first. Returns the byte count requested, 0 when the decoder runs out of data, and 0x2c
// for the header read. Called by WaveStream::Read (0x10004c20) for Ogg streams.
int OggStream::Read(BYTE *buffer, int bytes)
{
    int remaining = bytes;
    int skipped;
    int lastBlock;
    int copied;

    if (position < WAV_HEADER_BYTES) {
        memcpy(buffer, &header, sizeof(header));
        position = WAV_HEADER_BYTES;
        blockPosition = WAV_HEADER_BYTES;
        blockOffset = 0;
        return WAV_HEADER_BYTES;
    }
    if (position == WAV_HEADER_BYTES || blockPosition != position) {
        if (isOpen) {
            ogg_stream_clear(&stream);
            ogg_sync_clear(&sync);
        }
        RewindDecoder();
        skipped = WAV_HEADER_BYTES;
        lastBlock = 0;
        while (position > skipped) {
            if (!FeedChunk()) {
                return 0;
            }
            lastBlock = blockLength = Decode(pcmBuffer);
            skipped += lastBlock;
        }
        blockOffset = lastBlock - skipped + position;
        m_2734 = 0;
        blockPosition = position;
    }
    while (remaining > 0) {
        if (blockOffset != 0) {
            copied = blockLength - blockOffset;
            if (copied > remaining) {
                memcpy(buffer + (bytes - remaining), pcmBuffer + blockOffset, remaining);
                position += remaining;
                blockPosition = position;
                blockOffset += remaining;
                m_2734 = 0;
                return bytes;
            }
            memcpy(buffer + (bytes - remaining), pcmBuffer + blockOffset, copied);
            remaining -= copied;
        } else {
            if (!FeedChunk()) {
                return 0;
            }
            copied = Decode(pcmBuffer);
            blockLength = copied;
            if (remaining < copied) {
                memcpy(buffer + (bytes - remaining), pcmBuffer, remaining);
                position += remaining;
                blockPosition = position;
                blockOffset = remaining;
                m_2734 = 0;
                return bytes;
            }
            memcpy(buffer + (bytes - remaining), pcmBuffer, copied);
            remaining -= copied;
        }
        position += copied;
        blockPosition = position;
        blockOffset = 0;
        m_2734 = 0;
    }
    return bytes;
}
