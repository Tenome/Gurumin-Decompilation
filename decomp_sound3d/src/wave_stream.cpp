// wave_stream.cpp : WaveStream, the file reader behind a sound channel (0x100040d0-0x10005490 of sound3d.dll).
//
// A stream is opened by file name; the last letter of the name selects the decoder ('c', 'C', '3' = MP3, 'g', 'G' =
// Ogg Vorbis, '?' = a pair of ADPCM files, '2', 'v', 'V' = compressed ADPCM blocks, anything else = a WAV file).
// Read then delivers PCM data from that source in the order of a WAV file: first the 44 byte header, then the data.
#include "sound_engine.h"
#include <string.h>

// Destructor: releases the conversion buffers, the WAV copy and the file (the MmioFile destructor, expanded in place;
// its unwind funclet 0x1005ffd0 calls the out-of-line copy). Called by the channel's destructor and by WaveStream::Open's callers when a stream is dropped.
WaveStream::~WaveStream()
{
    FreeBuffers();
    if (fileCopy != NULL) {
        free(fileCopy);
    }
    fileCopy = NULL;
}

// Creates the ACM formats and the two block buffers: a 44.1 kHz ADPCM format with its 16 bit stereo PCM
// counterpart (ACM fills in the details), a second PCM format with the ADPCM format ACM suggests for it, and the
// two ACMSTREAMHEADERs that convert between the compressed and decoded buffers. When ACM has no ADPCM encoder the
// sound effects are muted (g_seMuted). keepEmpty non-zero only clears the state and fails. Returns 1 when ready.
// Called by SoundChannel::Open (0x10006250) and 0x10007dd0.
int WaveStream::PrepareBuffers(int keepEmpty)
{
    prepared = 0;
    FreeBuffers();
    adpcmFormat = NULL;
    encodedFormat = NULL;
    pcmFormat = NULL;
    pcmFormat2 = NULL;
    decodeStream = NULL;
    encodeStream = NULL;
    spareStream = NULL;
    fileCopy = NULL;
    decodedBuffer = NULL;
    compressedBuffer = NULL;
    if (keepEmpty != 0) {
        return 0;
    }
    pcmFormat = (WAVEFORMATEX *)g_allocateHook(FORMAT_BUFFER_BYTES, "");
    adpcmFormat = (WAVEFORMATEX *)g_allocateHook(FORMAT_BUFFER_BYTES, "");
    pcmFormat2 = (WAVEFORMATEX *)g_allocateHook(FORMAT_BUFFER_BYTES, "");
    encodedFormat = (WAVEFORMATEX *)g_allocateHook(FORMAT_BUFFER_BYTES, "");
    compressedBuffer = (BYTE *)g_allocateHook(CONVERT_BUFFER_BYTES, "");
    decodedBuffer = (BYTE *)g_allocateHook(CONVERT_BUFFER_BYTES, "");
    pcmFormat->wFormatTag = WAVE_FORMAT_PCM;
    pcmFormat->wBitsPerSample = PCM_OUTPUT_BITS;
    pcmFormat->nChannels = PCM_OUTPUT_CHANNELS;
    pcmFormat->nBlockAlign = PCM_OUTPUT_BLOCK_ALIGN;
    pcmFormat->nSamplesPerSec = PCM_OUTPUT_RATE;
    pcmFormat->nAvgBytesPerSec = PCM_OUTPUT_BYTES_PER_SECOND;
    pcmFormat->cbSize = 0;
    pcmFormat2->wFormatTag = WAVE_FORMAT_PCM;
    pcmFormat2->wBitsPerSample = PCM_OUTPUT_BITS;
    pcmFormat2->nChannels = PCM_OUTPUT_CHANNELS;
    pcmFormat2->nBlockAlign = PCM_OUTPUT_BLOCK_ALIGN;
    pcmFormat2->nSamplesPerSec = PCM_OUTPUT_RATE;
    pcmFormat2->nAvgBytesPerSec = PCM_OUTPUT_BYTES_PER_SECOND;
    pcmFormat2->cbSize = 0;
    adpcmFormat->wFormatTag = WAVE_FORMAT_ADPCM;
    adpcmFormat->wBitsPerSample = ADPCM_BITS;
    adpcmFormat->nChannels = PCM_OUTPUT_CHANNELS;
    adpcmFormat->nBlockAlign = ADPCM_BLOCK_ALIGN;
    adpcmFormat->nSamplesPerSec = PCM_OUTPUT_RATE;
    adpcmFormat->nAvgBytesPerSec = ADPCM_BYTES_PER_SECOND;
    adpcmFormat->cbSize = ADPCM_EXTRA_BYTES;
    acmFormatSuggest(NULL, adpcmFormat, pcmFormat, FORMAT_BUFFER_BYTES,
                     ACM_FORMATSUGGESTF_WFORMATTAG | ACM_FORMATSUGGESTF_NCHANNELS);
    memset(encodedFormat, 0, FORMAT_BUFFER_BYTES);
    encodedFormat->wFormatTag = WAVE_FORMAT_ADPCM;
    if (acmFormatSuggest(NULL, pcmFormat2, encodedFormat, FORMAT_BUFFER_BYTES, ACM_FORMATSUGGESTF_WFORMATTAG) != 0) {
        g_seMuted = 1;
    }
    memset(&decodeHeader, 0, sizeof(decodeHeader));
    decodeHeader.cbStruct = sizeof(ACMSTREAMHEADER);
    decodeHeader.pbSrc = compressedBuffer;
    decodeHeader.cbSrcLength = ACM_SOURCE_BYTES;
    decodeHeader.pbDst = decodedBuffer;
    decodeHeader.cbDstLength = ACM_DESTINATION_BYTES;
    memset(&encodeHeader, 0, sizeof(encodeHeader));
    encodeHeader.cbStruct = sizeof(ACMSTREAMHEADER);
    encodeHeader.cbSrcLength = ACM_SOURCE_BYTES;
    encodeHeader.pbSrc = compressedBuffer;
    encodeHeader.pbDst = decodedBuffer;
    encodeHeader.cbDstLength = ACM_DESTINATION_BYTES;
    prepared = 1;
    return 1;
}

// Opens a WAV file (names that end in 'wav'): measures it, finds the 'data' chunk tag in the first 0x78 bytes with
// a four byte sliding window and keeps the header; the dword after the tag (the true length of the data chunk) is read
// over the header's dataSize field. Returns 1 on success. Called by WaveStream::Open (0x10004860),
// directly for plain names and as the first try for names ending in '?'.
int WaveStream::OpenWav(const char *path)
{
    char window[4];
    WavHeader fileHeader;
    int dataLength;
    int i;

    ready = 0;
    if (!file.Open(path, "rb")) {
        return 0;
    }
    file.Seek(0, SEEK_END);
    dataLength = file.Tell() - WAV_HEADER_BYTES;
    file.Seek(0, SEEK_SET);
    file.Read((HPSTR)&fileHeader, 1, WAV_HEADER_BYTES);
    dataOffset = 0;
    file.Seek(0, SEEK_SET);
    for (i = 0; i < WAV_DATA_SCAN_BYTES; i++) {
        file.Read(&window[i % 4], 1, 1);
        if (i > 4 && window[(i - 3) % 4] == 'd' && window[(i - 2) % 4] == 'a' && window[(i - 1) % 4] == 't' &&
            window[i % 4] == 'a') {
            dataOffset = i - WAV_DATA_TAG_OFFSET;
            goto dataTagFound;
        }
    }
    if (i != 0) {
        file.Close();
        return 0;
    }
dataTagFound:
    file.Read((HPSTR)&fileHeader.dataSize, 1, 4);
    file.Seek(0, SEEK_SET);
    header = fileHeader;
    dataSize = dataLength - dataOffset;
    ready = 1;
    isWav = 1;
    return 1;
}

// Opens an obfuscated ADPCM file (names that end in 'v', 'V' or '2'... see Open): four header words, the first two
// of which are checked against the file name (CheckKey); the other two give the offset of the first block.
// Measures the stream from the file size: each block of adpcmBlockBytes + 8 compressed bytes decodes to
// ADPCM_DECODED_BLOCK_BYTES. Returns 1 on success. Called by WaveStream::Open (0x10004860).
int WaveStream::OpenEncoded(const char *path)
{
    int keyA;
    int keyB;
    int keyC;
    int keyD;
    char baseName[BASE_NAME_BYTES];

    ready = 0;
    if (file.handle != 0) {
        return 0;
    }
    if (!file.Open(path, "rb")) {
        return 0;
    }
    file.Read((HPSTR)&keyA, 1, 4);
    file.Read((HPSTR)&keyB, 1, 4);
    file.Read((HPSTR)&keyC, 1, 4);
    file.Read((HPSTR)&keyD, 1, 4);
    memset(baseName, 0, sizeof(baseName));
    GetBaseFileName(baseName, path);
    if (!CheckKey((unsigned char *)baseName, keyA, keyB)) {
        file.Close();
        return 0;
    }
    keyC ^= keyA;
    keyD ^= keyB;
    headerOffset = keyD + ENC_HEADER_SKIP;
    position = 0;
    file.Seek(headerOffset + ENC_BLOCKS_START, SEEK_SET);
    file.Read((HPSTR)&adpcmBlockBytes, 1, 4);
    file.Seek(0, SEEK_END);
    dataSize = (int)((unsigned __int64)(DWORD)(file.Tell() - ENC_BLOCKS_START - headerOffset) *
                     ADPCM_DECODED_BLOCK_BYTES / (__int64)(adpcmBlockBytes + ADPCM_BLOCK_OVERHEAD)) +
               ENC_BLOCKS_START;
    ready = 1;
    isCompressed = 1;
    return 1;
}

// Opens a plain ADPCM file without the key (names that end in '?' after the 'enc' try failed): the first block
// starts at 0x2c, the block length follows. Returns 1 on success. Called by WaveStream::Open (0x10004860).
int WaveStream::OpenDecoded(const char *path)
{
    ready = 0;
    if (file.handle != 0) {
        return 0;
    }
    if (!file.Open(path, "rb")) {
        return 0;
    }
    position = 0;
    headerOffset = 0;
    file.Seek(ENC_BLOCKS_START, SEEK_SET);
    file.Read((HPSTR)&adpcmBlockBytes, 1, 4);
    file.Seek(0, SEEK_END);
    dataSize = (int)((unsigned __int64)(DWORD)(file.Tell() - ENC_BLOCKS_START) * ADPCM_DECODED_BLOCK_BYTES /
                     (__int64)(adpcmBlockBytes + ADPCM_BLOCK_OVERHEAD)) +
               ENC_BLOCKS_START;
    ready = 1;
    isCompressed = 1;
    return 1;
}

// Characters of a file name's last letter that select the reader. The original tests them as (lower case, upper case)
// pairs; the digit suffixes '2' and '3' have no upper case and are tested twice (the binary compares twice, and the
// strlen call keeps the compiler from folding the repeat).
const char SUFFIX_WAV_PAIR = '?';
const char SUFFIX_ENCODED = '2';
const char SUFFIX_WAV_LOWER = 'v';
const char SUFFIX_WAV_UPPER = 'V';
const char SUFFIX_DECODED_LOWER = 'c';
const char SUFFIX_DECODED_UPPER = 'C';
const char SUFFIX_MP3 = '3';
const char SUFFIX_OGG_LOWER = 'g';
const char SUFFIX_OGG_UPPER = 'G';
const int PATH_BUFFER_BYTES = 512;
const char STRING_END = 0;
// A plain file is presented as 'dec' data: its sample count is the file length times this factor.
const int PLAIN_FILE_SIZE_FACTOR = 0xb;

// Opens a stream by file name; the last letter of the name picks the reader (see the file header). A name ending in
// '?' tries "<name>wav" as a WAV file and then "<name>dec" as a decoded ADPCM file. MP3 and Ogg streams get a
// freshly allocated decoder object kept in decoder; anything else is a plain file presented as decoded data.
// Returns 1 on success, 0 on failure. Called by SoundChannel::Open (0x10006250) and 0x10007dd0.
int WaveStream::Open(const char *path)
{
    char baseName[PATH_BUFFER_BYTES];
    char fullPath[PATH_BUFFER_BYTES];
    Mp3Stream *mp3;
    OggStream *ogg;

    isWav = 0;
    kind = STREAM_PLAIN;
    isCompressed = 0;
    if (path == NULL) {
        return 0;
    }
    if (path[strlen(path) - 1] == SUFFIX_WAV_PAIR) {
        strcpy(baseName, path);
        baseName[strlen(baseName) - 1] = STRING_END;
        sprintf(fullPath, "%swav", baseName);
        if (OpenWav(fullPath)) {
            return 1;
        }
        sprintf(fullPath, "%sdec", baseName);
        return OpenDecoded(fullPath) != 0;
    }
    if (path[strlen(path) - 1] == SUFFIX_ENCODED || path[strlen(path) - 1] == SUFFIX_ENCODED) {
        return OpenEncoded(path);
    }
    if (path[strlen(path) - 1] == SUFFIX_WAV_LOWER || path[strlen(path) - 1] == SUFFIX_WAV_UPPER) {
        return OpenWav(path);
    }
    if (path[strlen(path) - 1] == SUFFIX_DECODED_LOWER || path[strlen(path) - 1] == SUFFIX_DECODED_UPPER) {
        return OpenDecoded(path);
    }
    if (path[strlen(path) - 1] == SUFFIX_MP3 || path[strlen(path) - 1] == SUFFIX_MP3) {
        kind = STREAM_MP3;
        mp3 = new Mp3Stream;
        decoder = mp3;
        if (!mp3->Open(path)) {
            return 0;
        }
        ((Mp3Stream *)decoder)->LoadSource();
        dataSize = ((Mp3Stream *)decoder)->totalBytes;
        ready = 1;
        return 1;
    }
    if (path[strlen(path) - 1] == SUFFIX_OGG_LOWER || path[strlen(path) - 1] == SUFFIX_OGG_UPPER) {
        kind = STREAM_OGG;
        ogg = new OggStream;
        decoder = ogg;
        if (!ogg->Open(path)) {
            return 0;
        }
        dataSize = ((OggStream *)decoder)->totalBytes;
        ready = 1;
        return 1;
    }
    ready = 0;
    if (file.handle != 0) {
        return 0;
    }
    if (!file.Open(path, "rb")) {
        return 0;
    }
    position = 0;
    file.Seek(WAV_HEADER_BYTES, SEEK_SET);
    file.Seek(0, SEEK_END);
    dataSize = file.Tell() * PLAIN_FILE_SIZE_FACTOR;
    ready = 1;
    seekPending = 1;
    blockOffset = 0;
    return 1;
}

// Reads the next compressed block of a 'dec' file (a 4 byte length, then that many ADPCM bytes) into the compressed
// buffer and converts it through the decode ACM stream, opening and preparing the stream on first use. The result
// is left in the decode header's destination buffer and seekPending is cleared. Returns 0 when the stream cannot be
// opened. Used by WaveStream::Read (0x10004c20), which expands it three times.
inline int WaveStream::DecodeNextBlock()
{
    DWORD blockBytes;

    file.Read((HPSTR)&blockBytes, 1, 4);
    file.Read((HPSTR)compressedBuffer, 1, blockBytes);
    decodeHeader.cbSrcLength = blockBytes;
    if (decodeStream == 0) {
        if (acmStreamOpen(&decodeStream, NULL, adpcmFormat, pcmFormat, NULL, 0, 0, 0) != 0) {
            return 0;
        }
        decodeHeader.cbDstLengthUsed = 0;
        decodeHeader.cbSrcLengthUsed = 0;
        acmStreamPrepareHeader(decodeStream, &decodeHeader, 0);
    }
    acmStreamConvert(decodeStream, &decodeHeader, ACM_CONVERT_ONE_BLOCK);
    decodeHeader.cbDstLengthUsed = ADPCM_DECODED_BLOCK_BYTES;
    seekPending = 0;
    if (decodeStream != 0) {
        acmStreamUnprepareHeader(decodeStream, &decodeHeader, 0);
        acmStreamClose(decodeStream, 0);
        decodeStream = 0;
    }
    return 1;
}

// Delivers bytes bytes of PCM into buffer from the current stream position. WAV files are read straight from the
// file (the first read returns the built header). MP3 and Ogg streams hand over to their decoder. ADPCM files are
// decoded block by block through ACM: 'enc' files (isCompressed) seek to the block that holds the position, 'dec'
// files walk the length-prefixed blocks from the start, and a decoded block is kept in decodeHeader's buffer so the
// next read continues inside it (blockOffset). The first 0x2c bytes of a stream are read from the file as they are.
// Returns 1 on success, 0 when the stream is not ready or ACM fails (MP3 and Ogg return their decoder's result).
// Called by SoundChannel's refill code (0x10006250 and the mixer 0x10007dd0).
int WaveStream::Read(BYTE *buffer, int bytes)
{
    MMIOINFO info;
    DWORD blockBytes;
    DWORD skippedBytes;
    DWORD streamOffset;
    DWORD blockStart;
    DWORD chunk;
    DWORD copyBytes;
    int length;

    if (isWav) {
        if (!ready) {
            return 0;
        }
        filePosition = file.Tell();
        if (filePosition == 0) {
            file.Seek(dataOffset + WAV_HEADER_BYTES, SEEK_SET);
            memcpy(buffer, &header, bytes);
            filePosition += dataOffset + WAV_HEADER_BYTES;
            return 1;
        }
        file.Read((HPSTR)buffer, 1, bytes);
        filePosition += bytes;
        return 1;
    }
    if (kind == STREAM_MP3) {
        filePosition += bytes;
        return ((Mp3Stream *)decoder)->Read(buffer, bytes);
    }
    if (kind == STREAM_OGG) {
        filePosition += bytes;
        return ((OggStream *)decoder)->Read(buffer, bytes);
    }
    if (isCompressed) {
        if ((DWORD)position < (DWORD)WAV_HEADER_BYTES) {
            file.Seek(headerOffset + position, SEEK_SET);
            file.Read((HPSTR)buffer, 1, bytes);
            filePosition = 0;
            return 1;
        }
        streamOffset = position - ENC_BLOCKS_START;
        while (bytes > 0) {
            blockStart = streamOffset / ADPCM_DECODED_BLOCK_BYTES * ADPCM_DECODED_BLOCK_BYTES;
            file.Seek(streamOffset / ADPCM_DECODED_BLOCK_BYTES * (adpcmBlockBytes + ADPCM_BLOCK_OVERHEAD) +
                          headerOffset + ENC_BLOCKS_START,
                      SEEK_SET);
            if ((DWORD)position >= (DWORD)dataSize) {
                return 1;
            }
            file.Read((HPSTR)&blockBytes, 1, 4);
            if (blockBytes > ENC_MAX_BLOCK_BYTES) {
                return 1;
            }
            file.Read((HPSTR)&skippedBytes, 1, 4);
            file.Read((HPSTR)compressedBuffer, 1, blockBytes);
            if (acmStreamOpen(&encodeStream, NULL, encodedFormat, pcmFormat2, NULL, 0, 0,
                              ACM_STREAMOPENF_NONREALTIME) != 0) {
                return 0;
            }
            acmStreamPrepareHeader(encodeStream, &encodeHeader, 0);
            acmStreamConvert(encodeStream, &encodeHeader, 0);
            acmStreamUnprepareHeader(encodeStream, &encodeHeader, 0);
            acmStreamClose(encodeStream, 0);
            encodeStream = 0;
            chunk = blockStart - streamOffset + ADPCM_DECODED_BLOCK_BYTES;
            if (chunk > (DWORD)bytes) {
                chunk = bytes;
            }
            memcpy(buffer, decodedBuffer + (streamOffset - blockStart), chunk);
            bytes -= chunk;
            buffer += chunk;
            streamOffset += chunk;
            position += chunk;
            filePosition += chunk;
        }
        return 1;
    }
    if ((DWORD)position < (DWORD)WAV_HEADER_BYTES) {
        file.Seek(position, SEEK_SET);
        file.Read((HPSTR)buffer, 1, bytes);
        position += WAV_HEADER_BYTES;
        return 1;
    }
    while (bytes > 0) {
        if (blockOffset != 0) {
            copyBytes = decodeHeader.cbDstLengthUsed - blockOffset;
            if ((DWORD)bytes <= copyBytes) {
                memcpy(buffer, decodeHeader.pbDst + ADPCM_DECODED_SKIP + blockOffset, bytes);
                position += bytes;
                blockOffset += bytes;
                filePosition += bytes;
                return 1;
            }
            memcpy(buffer, decodeHeader.pbDst + ADPCM_DECODED_SKIP + blockOffset, copyBytes);
            buffer += copyBytes;
            bytes -= copyBytes;
            position += copyBytes;
            filePosition += copyBytes;
            blockOffset = 0;
        }
        if (seekPending) {
            file.Seek(WAV_HEADER_BYTES, SEEK_SET);
            skippedBytes = WAV_HEADER_BYTES;
            while ((int)(position - ADPCM_DECODED_BLOCK_BYTES) > (int)skippedBytes) {
                file.Read((HPSTR)&length, 1, 4);
                file.Seek(length, SEEK_CUR);
                skippedBytes += ADPCM_DECODED_BLOCK_BYTES;
            }
            if (skippedBytes < (DWORD)position) {
                do {
                    if (!DecodeNextBlock()) {
                        return 0;
                    }
                    skippedBytes += ADPCM_DECODED_BLOCK_BYTES;
                } while (skippedBytes < (DWORD)position);
            }
            chunk = skippedBytes - position;
            if ((DWORD)bytes < chunk) {
                memcpy(buffer, decodeHeader.pbDst + ADPCM_DECODED_SKIP, bytes);
                blockOffset = decodeHeader.cbDstLengthUsed - skippedBytes + position + bytes;
                position += bytes;
                filePosition += bytes;
                return 1;
            }
            memcpy(buffer, decodeHeader.pbDst + decodeHeader.cbDstLengthUsed - skippedBytes + position +
                               ADPCM_DECODED_SKIP,
                   chunk);
            bytes += position - skippedBytes;
            buffer += skippedBytes - position;
            position = skippedBytes;
            blockOffset = 0;
        }
        if (!DecodeNextBlock()) {
            return 0;
        }
        if (decodeHeader.cbDstLengthUsed > (DWORD)bytes) {
            memcpy(buffer, decodeHeader.pbDst + ADPCM_DECODED_SKIP, bytes);
            position += bytes;
            blockOffset = bytes;
            filePosition += bytes;
            return 1;
        }
        memcpy(buffer, decodeHeader.pbDst + ADPCM_DECODED_SKIP, decodeHeader.cbDstLengthUsed);
        copyBytes = decodeHeader.cbDstLengthUsed;
        bytes -= copyBytes;
        buffer += copyBytes;
        position += copyBytes;
        filePosition += copyBytes;
        blockOffset = 0;
    }
    return 1;
}
