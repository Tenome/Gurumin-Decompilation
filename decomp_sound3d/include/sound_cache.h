// sound_cache.h : the cache of decoded sound effects of sound3d.dll (sound_cache.cpp).
//
// PlaySE does not decode a file every time: the first play of a name loads the whole PCM data into a heap block, and
// the next plays copy it into the DirectSound buffer. 0x30 slots, replaced oldest first (every load ages all slots
// by one; a hit makes its slot the youngest). The slot pointers themselves are g_waveCache (sound_engine.h).
#ifndef SOUND_CACHE_H
#define SOUND_CACHE_H

#include "wav_header.h"

const int WAVE_CACHE_SLOTS = 0x30;                  // cached sounds
const int WAVE_CACHE_NAME_BYTES = 0x100;            // longest cached path
const int WAVE_LOAD_BUFFER_BYTES = 0x200000;        // scratch buffer an archived sound is read into whole

extern unsigned g_waveCacheAge[WAVE_CACHE_SLOTS];                           // 0x1007f450 loads since last use
extern WavHeader g_waveCacheHeader[WAVE_CACHE_SLOTS];                       // 0x1007f510 header of each cached sound
extern char g_waveLoadBuffer[WAVE_LOAD_BUFFER_BYTES];                       // 0x1007fd50
extern char g_waveCacheName[WAVE_CACHE_SLOTS][WAVE_CACHE_NAME_BYTES];       // 0x1027fd50

int FindOrLoadSound(const char *path);              // 0x10006250
#endif
