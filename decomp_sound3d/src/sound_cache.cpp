// sound_cache.cpp : the decoded sound-effect cache of sound3d.dll (0x10006250).
#include <string.h>
#include <stdlib.h>
#include "sound_engine.h"
#include "sound_cache.h"

// Last letter of a file name that marks an archived effect (a whole file carrying a trailing key block).
const char SUFFIX_ARCHIVED_LOWER = 'm';
const char SUFFIX_ARCHIVED_UPPER = 'M';
// An archived effect ends in the checksum words (sum, xor) after an extra 4 bytes: 12 bytes after the sound data.
const int ARCHIVED_TRAILER_BYTES = 0xc;
const int ARCHIVED_KEY_COPY_BYTES = 0x20;     // name key buffer handed to GetBaseFileName, cleared first
const int WINDOW_BYTES = 4;                   // the four letters 'data' are searched for with a sliding window

// Returns the cache slot of the sound called path, loading it first when it is not cached (replacing the oldest slot).
// Names that end in 'm' or 'M' are archived effects: the whole file is read into the scratch buffer, its name key
// is verified (CheckNameKeyA), and the PCM bytes behind the 'data' tag are copied to a heap block. Every other name
// is opened through a WaveStream and read out whole. Returns -1 when the file cannot be opened or verified.
// Called by SoundChannel::PlaySE.
int FindOrLoadSound(const char *path)
{
    char window[WINDOW_BYTES];
    int slot;
    int i;

    for (i = 0; i < WAVE_CACHE_SLOTS; i++) {
        g_waveCacheAge[i]++;
    }
    for (i = 0; i < WAVE_CACHE_SLOTS; i++) {
        if (g_waveCache[i] != 0 && _stricmp(path, g_waveCacheName[i]) == 0) {
            g_waveCacheAge[i] = 0;
            return i;
        }
    }
    slot = 0;
    unsigned oldestAge = g_waveCacheAge[0];
    for (i = 1; i < WAVE_CACHE_SLOTS; i++) {
        if (g_waveCacheAge[i] > oldestAge) {
            oldestAge = g_waveCacheAge[i];
            slot = i;
        }
    }
    char lastLetter = path[strlen(path) - 1];
    if (lastLetter == SUFFIX_ARCHIVED_LOWER || lastLetter == SUFFIX_ARCHIVED_UPPER) {
        MmioFile file;
        int dataOffset;
        int fileLength;
        DWORD expectedSum;
        DWORD expectedXor;
        char key[ARCHIVED_KEY_COPY_BYTES];

        if (!file.Open(path, "rb")) {
            return -1;
        }
        memset(key, 0, sizeof(key));
        GetBaseFileName(key, path);
        file.Seek(0, SEEK_END);
        fileLength = file.Tell() - ARCHIVED_TRAILER_BYTES;
        file.Seek(0, SEEK_SET);
        file.Read(g_waveLoadBuffer, 1, fileLength);
        file.Read((HPSTR)&expectedSum, 1, 4);
        file.Read((HPSTR)&expectedSum, 1, 4);
        file.Read((HPSTR)&expectedXor, 1, 4);
        file.Close();
        if (!CheckNameKeyA(g_waveLoadBuffer, fileLength, key, expectedSum, expectedXor)) {
            return -1;
        }
        if (g_waveCache[slot] != 0) {
            free(g_waveCache[slot]);
            g_waveCache[slot] = 0;
        }
        dataOffset = 0;
        const char *scan = g_waveLoadBuffer;
        for (i = 0; i < WAV_DATA_SCAN_BYTES; i++) {
            window[i % 4] = *scan++;
            if (i > 4 && window[(i - 3) % 4] == 'd' && window[(i - 2) % 4] == 'a' && window[(i - 1) % 4] == 't' &&
                window[i % 4] == 'a') {
                dataOffset = i - WAV_DATA_TAG_OFFSET;
                goto dataTagFound;
            }
        }
        if (i != 0) {
            return -1;
        }
    dataTagFound:
        unsigned soundBytes = fileLength - dataOffset - WAV_HEADER_BYTES;
        g_waveCache[slot] = operator new(soundBytes);
        g_waveCacheHeader[slot] = *(WavHeader *)g_waveLoadBuffer;
        g_waveCacheHeader[slot].dataSize = *(DWORD *)scan;
        memcpy(g_waveCache[slot], &g_waveLoadBuffer[WAV_HEADER_BYTES + dataOffset], soundBytes);
        if (g_waveCacheHeader[slot].dataSize > soundBytes) {
            g_waveCacheHeader[slot].dataSize = soundBytes;
        }
    } else {
        WaveStream stream;

        stream.PrepareBuffers(1);
        if (!stream.Open(path)) {
            return -1;
        }
        stream.Seek(0);
        stream.Read((BYTE *)&g_waveCacheHeader[slot], WAV_HEADER_BYTES);
        if (g_waveCache[slot] != 0) {
            free(g_waveCache[slot]);
            g_waveCache[slot] = 0;
        }
        unsigned soundBytes = g_waveCacheHeader[slot].dataSize;
        g_waveCache[slot] = operator new(soundBytes);
        stream.Read((BYTE *)g_waveCache[slot], soundBytes);
        stream.Close();
        stream.FreeBuffers();
    }
    i = 0;
    do {
        g_waveCacheName[slot][i] = path[i];
    } while (path[i++] != 0);
    return slot;
}
