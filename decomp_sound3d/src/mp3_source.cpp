// mp3_source.cpp : Mp3Stream::Open, the MP3 file opener of sound3d.dll (0x10003ab0).
#include "mp3_stream.h"
#include <string.h>

// The divisor that turns bits into the unit the stream header uses (bits * 8 / 16).
const int PCM_UNIT_DIVISOR = 16;

// Opens an MP3 file for streaming: resets the decoder, reads the first 0x200 bytes, finds the first MPEG frame
// and parses its header, measures the file and works out the stream numbers (frame count, data size, length).
// Returns 1 on success, 0 when the file is missing or is not an MP3. Called by WaveStream::Open (0x10004860) for
// names ending in 'c', 'C' or '3'.
int Mp3Stream::Open(const char *path)
{
    BYTE probe[MP3_PROBE_BYTES];
    int probeLength;
    int decodedBytes;
    const char *baseName;
    const char *from;
    char *copy;
    char letter;

    decoder.outputUsed = 0;
    Mp3ResetDecoder();
    Mp3SetFlag(0);
    lastPosition = 0;
    position = 0;
    source = 0;
    frameBuffer = 0;
    m_0080 = 0x80;
    Close();
    file.Open(path, "rb");
    if (file.handle == 0) {
        return 0;
    }
    probeLength = file.Read((HPSTR)probe, 1, MP3_PROBE_BYTES);
    if (!Mp3FindFrameStart(probe, probeLength, &firstFrameOffset)) {
        Close();
        return 0;
    }
    if (!Mp3ParseFrameHeader(probe + firstFrameOffset, probeLength - firstFrameOffset, &info)) {
        Close();
        return 0;
    }
    file.Seek(0, SEEK_END);
    dataLength = file.Tell() - firstFrameOffset;
    file.Seek(0, SEEK_SET);
    if (info.frameCount == 0) {
        info.frameCount = (dataLength - firstFrameOffset) / info.frameBytes;
    }
    formatBitrate = info.bitrate;
    decodedBytes = info.samplesPerFrame * info.frameCount * BITS_PER_BYTE / PCM_UNIT_DIVISOR / info.channels;
    pcmBytes = decodedBytes;
    seconds = decodedBytes / info.sampleRate;
    formatChannels = info.channels;
    formatSampleRate = info.sampleRate;
    frameCount = info.frameCount;
    baseName = strrchr(path, '\\');
    if (baseName == NULL) {
        baseName = path;
    } else {
        baseName++;
    }
    fileName.text = (char *)realloc(fileName.text, strlen(baseName) + 1);
    copy = fileName.text;
    from = baseName;
    do {
        letter = *from++;
        *copy++ = letter;
    } while (letter != '\0');
    return 1;
}
