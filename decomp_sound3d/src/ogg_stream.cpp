// ogg_stream.cpp : OggStream, the Ogg Vorbis file decoder of sound3d.dll (0x10001bc0-0x10002970).
//
// WaveStream (kind STREAM_OGG) keeps one OggStream. Open and Read (ogg_source.cpp) put the whole file in memory and
// hand out the PCM; StartDecoder / RewindDecoder parse the three header packets, Decode turns pages into 16-bit PCM,
// Close releases everything.
#include <stdlib.h>
#include <string.h>
#include "ogg_stream.h"

// Feeds the next piece of the file to libogg: 4096 bytes while at least 8192 bytes remain, otherwise whatever is
// left. Returns 1 when something was fed, 0 at the end of the file. Called by Read (0x10003e80).
int OggStream::FeedChunk()
{
    char *buffer;
    int remaining;

    if ((readPtr - fileData) + 2 * OGG_FEED_BYTES >= fileSize) {
        remaining = fileSize - (readPtr - fileData);
        if (remaining > 0) {
            buffer = ogg_sync_buffer(&sync, remaining);
            memcpy(buffer, readPtr, remaining);
            readPtr += remaining;
            ogg_sync_wrote(&sync, remaining);
            return 1;
        }
        return 0;
    }
    buffer = ogg_sync_buffer(&sync, OGG_FEED_BYTES);
    memcpy(buffer, readPtr, OGG_FEED_BYTES);
    readPtr += OGG_FEED_BYTES;
    ogg_sync_wrote(&sync, OGG_FEED_BYTES);
    return 1;
}

// Opens the stream for playback: feeds the first 4096 bytes, reads the first page and the three header packets
// (info, comments, codebooks), then sets up the synthesis state. samplesPerChunk is how many samples per channel
// fit the 4096-sample conversion buffer. Returns 1 on success, 0 when the file is not a usable Ogg Vorbis stream.
// Called by Open (0x10003cb0).
int OggStream::StartDecoder()
{
    int result;
    int headerPackets;
    char **userComment;
    OggStreamState *os;
    OggPacket *op;
    VorbisComment *vc;

    readPtr = fileData;
    ogg_sync_init(&sync);
    memcpy(ogg_sync_buffer(&sync, OGG_FEED_BYTES), readPtr, OGG_FEED_BYTES);
    readPtr += OGG_FEED_BYTES;
    if (ogg_sync_wrote(&sync, OGG_FEED_BYTES) != 0) {
        return 0;
    }
    if (ogg_sync_pageout(&sync, &page) != 1) {
        return 0;
    }
    os = &stream;
    if (ogg_stream_init(os, ogg_page_serialno(&page)) != 0) {
        return 0;
    }
    vorbis_info_init(&info);
    vc = &comment;
    vorbis_comment_init(vc);
    if (ogg_stream_pagein(os, &page) < 0) {
        return 0;
    }
    op = &packet;
    if (ogg_stream_packetout(os, op) != 1) {
        return 0;
    }
    if (vorbis_synthesis_headerin(&info, vc, op) < 0) {
        return 0;
    }
    headerPackets = 0;
    while (headerPackets < OGG_EXTRA_HEADER_PACKETS) {
        result = ogg_sync_pageout(&sync, &page);
        if (result == 0) {
            continue;
        }
        if (result == 1) {
            ogg_stream_pagein(os, &page);
            do {
                result = ogg_stream_packetout(os, op);
                if (result == 0) {
                    break;
                }
                if (result < 0) {
                    return 0;
                }
                vorbis_synthesis_headerin(&info, vc, op);
                headerPackets++;
            } while (headerPackets < OGG_EXTRA_HEADER_PACKETS);
        }
    }
    userComment = vc->userComments;
    while (*userComment != 0) {
        userComment++;
    }
    samplesPerChunk = OGG_FEED_BYTES / info.channels;
    vorbis_synthesis_init(&dsp, &info);
    vorbis_block_init(&dsp, &block);
    isOpen = 1;
    return 1;
}

// Like StartDecoder but only parses the headers again (no vorbis_info_init, no synthesis state): restarts the
// decoder at the first page when Read seeks. Returns 1 on success, 0 on failure. Called by Read (0x10003e80).
int OggStream::RewindDecoder()
{
    int result;
    int headerPackets;
    char **userComment;

    readPtr = fileData;
    ogg_sync_init(&sync);
    memcpy(ogg_sync_buffer(&sync, OGG_FEED_BYTES), readPtr, OGG_FEED_BYTES);
    readPtr += OGG_FEED_BYTES;
    if (ogg_sync_wrote(&sync, OGG_FEED_BYTES) != 0) {
        return 0;
    }
    if (ogg_sync_pageout(&sync, &page) != 1) {
        return 0;
    }
    if (ogg_stream_init(&stream, ogg_page_serialno(&page)) != 0) {
        return 0;
    }
    vorbis_comment_init(&comment);
    if (ogg_stream_pagein(&stream, &page) < 0) {
        return 0;
    }
    if (ogg_stream_packetout(&stream, &packet) != 1) {
        return 0;
    }
    if (vorbis_synthesis_headerin(&info, &comment, &packet) < 0) {
        return 0;
    }
    headerPackets = 0;
    while (headerPackets < OGG_EXTRA_HEADER_PACKETS) {
        result = ogg_sync_pageout(&sync, &page);
        if (result == 0) {
            continue;
        }
        if (result == 1) {
            ogg_stream_pagein(&stream, &page);
            do {
                result = ogg_stream_packetout(&stream, &packet);
                if (result == 0) {
                    break;
                }
                if (result < 0) {
                    return 0;
                }
                vorbis_synthesis_headerin(&info, &comment, &packet);
                headerPackets++;
            } while (headerPackets < OGG_EXTRA_HEADER_PACKETS);
        }
    }
    userComment = comment.userComments;
    while (*userComment != 0) {
        userComment++;
    }
    samplesPerChunk = OGG_FEED_BYTES / info.channels;
    return 1;
}

// Decodes pages into output (16-bit interleaved PCM, scaled by gain) until the data runs out or the end-of-stream
// page was seen. Samples are converted per channel in chunks of at most samplesPerChunk, clamped to 16 bits.
// Nothing is copied once the result would reach OGG_PCM_BUFFER_BYTES. Returns the bytes written, -1 on a corrupt
// page or packet. Called by Read (0x10003e80).
int OggStream::Decode(BYTE *output)
{
    int written;
    int result;
    int samples;
    int chunk;
    int channel;
    int i;
    int clamped;
    float savedGain;
    float **channelData;
    float *mono;
    short *out;

    written = 0;
    do {
        result = ogg_sync_pageout(&sync, &page);
        if (result == 0) {
            return written;
        }
        if (result < 0) {
            return -1;
        }
        ogg_stream_pagein(&stream, &page);
        while ((result = ogg_stream_packetout(&stream, &packet)) != 0) {
            if (result < 0) {
                return -1;
            }
            if (vorbis_synthesis(&block, &packet) == 0) {
                vorbis_synthesis_blockin(&dsp, &block);
            }
            samples = vorbis_synthesis_pcmout(&dsp, &channelData);
            while (samples > 0) {
                if (samples < samplesPerChunk) {
                    chunk = samples;
                } else {
                    chunk = samplesPerChunk;
                }
                savedGain = gain;
                for (channel = 0; channel < info.channels; channel++) {
                    gain = savedGain;
                    mono = channelData[channel];
                    out = pcm + channel;
                    for (i = 0; i < chunk; i++) {
                        // 32768.0f scales a Vorbis sample (-1..1) to 16-bit; a named const float changes the code
                        clamped = (int)(mono[i] * 32768.0f);
                        if (clamped > 32767) {
                            clamped = 32767;
                        } else if (clamped < -32768) {
                            clamped = -32768;
                        }
                        *out = (short)(clamped * gain);
                        out += info.channels;
                    }
                }
                if (info.channels * chunk * 2 + written < OGG_PCM_BUFFER_BYTES) {
                    memcpy(output + written, pcm, info.channels * chunk * 2);
                }
                written += info.channels * chunk * 2;
                vorbis_synthesis_read(&dsp, chunk);
                samples = vorbis_synthesis_pcmout(&dsp, &channelData);
            }
        }
    } while (ogg_page_eos(&page) == 0);
    return written;
}

// Clears the page and packet layers only (nothing calls this: the unused sibling of Release).
int OggStream::ReleaseReader()
{
    if (isOpen) {
        ogg_stream_clear(&stream);
        ogg_sync_clear(&sync);
    }
    return 1;
}

// Clears every libogg/libvorbis state when the stream was opened. Called by Close.
int OggStream::Release()
{
    if (isOpen) {
        ogg_stream_clear(&stream);
        ogg_sync_clear(&sync);
        vorbis_block_clear(&block);
        vorbis_dsp_clear(&dsp);
        vorbis_comment_clear(&comment);
        vorbis_info_clear(&info);
    }
    isOpen = 0;
    return 1;
}

// Frees the file image and the PCM buffer and releases the decoder. Called by WaveStream::Close and Open.
int OggStream::Close()
{
    if (pcmBuffer) {
        free(pcmBuffer);
    }
    pcmBuffer = 0;
    if (fileData) {
        free(fileData);
    }
    fileData = 0;
    if (isOpen) {
        ogg_stream_clear(&stream);
        ogg_sync_clear(&sync);
        Release();
    }
    return 1;
}
