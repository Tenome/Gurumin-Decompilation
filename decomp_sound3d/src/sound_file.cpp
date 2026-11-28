// sound_file.cpp : the small members of WaveStream, the per-sound stream object of sound3d.dll (0x100029e0-0x10003170).
//
// Constructor, release of the ACM streams, seek, tell, end test and close. The openers and Read live in
// wave_stream.cpp (0x10004140 and up).
#include <stdlib.h>
#include "wave_stream.h"

// Constructor: the file starts closed (the member constructor), then the handles and buffer pointers are cleared.
// Run by the SoundChannel constructor 0x100058a0 and for the local stream of 0x10006250.
WaveStream::WaveStream()
{
    ready = 0;
    position = 0;
    adpcmFormat = 0;
    encodedFormat = 0;
    pcmFormat = 0;
    pcmFormat2 = 0;
    headerOffset = 0;
    decodeStream = 0;
    encodeStream = 0;
    spareStream = 0;
    fileCopy = 0;
    decodedBuffer = 0;
    compressedBuffer = 0;
}

// Releases the three ACM conversion streams (unprepare the stream header, close the stream) and the buffers they
// used. Returns 0 when PrepareBuffers never ran, otherwise 1. Called by the destructor, PrepareBuffers,
// SoundChannel::ReleaseBuffers (0x10006700) and 0x10006250.
int WaveStream::FreeBuffers()
{
    if (prepared == 0) {
        return 0;
    }
    if (encodeStream != 0) {
        acmStreamUnprepareHeader(encodeStream, &encodeHeader, 0);
        acmStreamClose(encodeStream, 0);
        encodeStream = 0;
    }
    if (decodeStream != 0) {
        acmStreamUnprepareHeader(decodeStream, &decodeHeader, 0);
        acmStreamClose(decodeStream, 0);
        decodeStream = 0;
    }
    if (spareStream != 0) {
        acmStreamUnprepareHeader(spareStream, &spareHeader, 0);
        acmStreamClose(spareStream, 0);
        spareStream = 0;
    }
    if (pcmFormat != 0) {
        g_freeHook(pcmFormat);
        pcmFormat = 0;
    }
    if (adpcmFormat != 0) {
        g_freeHook(adpcmFormat);
        adpcmFormat = 0;
    }
    if (pcmFormat2 != 0) {
        g_freeHook(pcmFormat2);
        pcmFormat2 = 0;
    }
    if (encodedFormat != 0) {
        g_freeHook(encodedFormat);
        encodedFormat = 0;
    }
    if (compressedBuffer != 0) {
        g_freeHook(compressedBuffer);
        compressedBuffer = 0;
    }
    if (decodedBuffer != 0) {
        g_freeHook(decodedBuffer);
        decodedBuffer = 0;
    }
    return 1;
}

// Moves the play position (bytes). For a memory image the position is stored in the decoder object (Ogg, MP3) or in
// position and a seek request is flagged; for a streamed file the mmio file is repositioned (positions below
// the 44 byte header map onto the file start, others are shifted by dataOffset). Called by the player 0x10005e30,
// 0x10006250 and the volume/loop code 0x10007dd0.
void WaveStream::Seek(unsigned int newPosition)
{
    if (isWav != 0) {
        if (newPosition < (unsigned int)WAV_HEADER_BYTES) {
            file.Seek(newPosition, SEEK_SET);
            filePosition = newPosition;
            return;
        }
        file.Seek(dataOffset + newPosition, SEEK_SET);
        filePosition = dataOffset + newPosition;
        return;
    }
    if (kind == STREAM_OGG) {
        OggStream *ogg = (OggStream *)decoder;
        ogg->Seek(newPosition);
        filePosition = newPosition;
    }
    if (kind == STREAM_MP3) {
        Mp3Stream *mp3 = (Mp3Stream *)decoder;
        mp3->Seek(newPosition);
        filePosition = newPosition;
    }
    position = newPosition;
    seekPending = 1;
    blockOffset = 0;
    filePosition = newPosition;
}

// Returns how many sample bytes were played so far: the file position minus dataOffset for a streamed file (0 when
// before the data), or the position kept by the decoder / position for a memory image. Called by the player
// 0x10005e30 and AtEnd.
int WaveStream::Tell()
{
    if (isWav != 0) {
        return file.Tell() - dataOffset < 0 ? 0 : file.Tell() - dataOffset;
    }
    if (kind == STREAM_OGG) {
        return ((OggStream *)decoder)->position - ((OggStream *)decoder)->m_2734;
    }
    if (kind == STREAM_MP3) {
        return ((Mp3Stream *)decoder)->position;
    }
    return position;
}

// True when the whole sound has been played (position at or past dataSize). Nothing in the game calls it.
bool WaveStream::AtEnd()
{
    if (isWav != 0) {
        return dataSize <= (unsigned int)Tell();
    }
    return position >= dataSize;
}

// Releases whatever the sound holds: the decoder object (MP3 / Ogg), the in-memory WAV copy or the mmio file.
// Returns the decoder's result, 1 for a closed file, 0 when nothing was open. Called by the player 0x10005c60,
// 0x10005e30, 0x10006250 and 0x10007dd0.
int WaveStream::Close()
{
    int result;

    if (isWav != 0) {
        if (file.handle == 0) {
            return 0;
        }
        file.Close();
        ready = 0;
        return 1;
    }
    // Each decoder kind frees its object the same way; the compiler merges the two tails.
    if (kind == STREAM_MP3) {
        result = ((Mp3Stream *)decoder)->Close();
        if (decoder != 0) {
            free(decoder);
        }
        decoder = 0;
        return result;
    }
    if (kind == STREAM_OGG) {
        result = ((OggStream *)decoder)->Close();
        if (decoder != 0) {
            free(decoder);
        }
        decoder = 0;
        return result;
    }
    if (file.handle == 0) {
        return 0;
    }
    if (fileCopy != 0) {
        free(fileCopy);
    }
    fileCopy = 0;
    file.Close();
    ready = 0;
    return 1;
}
