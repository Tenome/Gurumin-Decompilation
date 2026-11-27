// sound_engine.h : types, constants and globals of sound3d.dll shared by the exported API (sound_api.cpp), the
// mixer helpers (sound_mixer.cpp) and the channel methods.
//
// sound3d.dll is the sound engine of Gurumin. The exported I... functions either queue a command (the game thread
// must not touch DirectSound itself) or act on one of 27 SoundChannel objects: one master object that owns the
// DirectSound device (g_master), two music channels (g_bgmChannel) and 24 sound-effect channels (g_seChannel).
// A 10 ms multimedia timer (TimerCallback) drains the command queue and runs the channels' fade updates.
#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

#include <windows.h>
#include <mmsystem.h>
#include "sound_channel.h"
#include "tracked_allocator.h"

// ---- constants --------------------------------------------------------------------------------------------

const int BGM_CHANNEL_COUNT = 2;       // music channels (IPlay and friends take slot 0 or 1)
const int BGM_LAST_CHANNEL = 1;        // highest music channel index
const int SE_CHANNEL_COUNT = 24;       // sound-effect channels
const int COMMAND_QUEUE_SIZE = 256;    // entries of the byte-indexed command ring
const int COMMAND_NAME_SIZE = 0x100;   // file name of a queued play command
const int SE_STOP_LAST_INDEX = 0xb;    // IStopSE and ISetSEVolume accept only 0..11 (the original's bound)
const int SE_POS_LAST_INDEX = 0x17;    // ISetSEPos accepts 0..23

const int FADE_MIN_MS = 1;             // IFadeOutEx clamps its length to 1..60000 ms
const int FADE_MAX_MS = 60000;
const int FADE_DEFAULT_MS = 1000;      // IFadeOut fade length
const LONG VOLUME_SILENT = -10000;     // 0xffffd8f0, DirectSound's minimum attenuation

const int MAX_PATH_BUFFER = 0x100;     // path buffers of the IPlay integrity check
const int INTEGRITY_TABLE_OFFSET = 0x17000;   // where soundarc.dll keeps its checksum words

const DWORD COMMAND_EXPIRY_MS = 1000;  // a queued play command older than this is dropped
const int CALLBACK_RESOLUTION_MS = 5;  // timeSetEvent accuracy
const int CALLBACK_PERIOD_MS = 10;     // command queue pump period
const int TIMER_PERIOD_MIN_MS = 1;     // timeBeginPeriod

// Command codes stored in SoundCommand::command.
enum SoundCommandCode {
    CMD_NONE = 0,
    CMD_PLAY_FADE = 100,        // IPlayFade and IPlayFadeEx (the original stores 100 for both): music with fade-in
    CMD_PLAY = 101,             // IPlay       : start a music file
    CMD_PLAY_FADE_EX = 102,     // executed by ProcessCommandQueue but never queued by any export
    CMD_SET_LISTENER = 150,     // ISetListen
    CMD_PLAY_SE = 200           // IPlaySE
};

// ---- types ----------------------------------------------------------------------------------------------------

// One entry of the command ring (0x1c0 bytes). The game thread fills it, TimerCallback executes it.
struct SoundCommand {
    char name[COMMAND_NAME_SIZE];   // +0x000 file name
    int command;                    // +0x100 SoundCommandCode
    int channel;                    // +0x104 music channel 0/1
    int arg1;                       // +0x108
    int arg2;                       // +0x10c
    int arg3;                       // +0x110 (IPlaySE: has-position flag)
    int arg4;                       // +0x114 (IPlaySE: loop flag)
    int arg5;                       // +0x118
    char m_011c[0x20];              // +0x11c
    DWORD queuedAt;                 // +0x13c GetTickCount when queued, 0 = never expires
    char m_0140[0x24];              // +0x140
    float value[9];                 // +0x164 IPlaySE position/volume, ISetListen parameters 2..10
    float listenerFirst;            // +0x188 ISetListen parameter 1
    char m_018c[0x34];              // +0x18c
};

// File checksums of the IPlay integrity check (sound_init.cpp) and the command pump (sound_api.cpp).
int ComputeFileChecksum(const char *path, int *sumOut, unsigned *xorOut);
int ReadFileWords(const char *path, int offset, int index, void *first, void *second);
void ProcessCommandQueue(int unused);

// ---- mixer (sound_mixer.cpp) ------------------------------------------------------------------------------

const int MIXER_PARAM_KINDS = 4;          // GetParam/SetParam line number: 0 auxiliary, 1 CD, 2 telephone, 3 master
const int MIXER_PARAM_STRIDE = 64;        // cache slots per kind
const int MIXER_CACHE_SIZE = 0x1000;      // cache slots
const int MIXER_NO_CONTROL = -1;          // control id of a control the mixer does not have

// Mixer control ids found by MixOpen (static members: the original initialises them in a constructor).
class MixerControls {
public:
    MixerControls();
    static HMIXER handle;
    static DWORD peakMeter;
    static DWORD masterVolume;
    static DWORD masterSpare;
    static DWORD masterPan;
    static DWORD auxVolume;
    static DWORD auxPan;
    static DWORD auxSpare;
    static DWORD phoneVolume;
    static DWORD phonePan;
    static DWORD phoneSpare;
    static DWORD cdVolume;
    static DWORD cdPan;
    static DWORD cdPanAgain;
};

extern MIXERLINECONTROLS g_lineControls;           // 0x1029c398
extern MIXERLINE g_mixerLine;                      // 0x1029c3b0
extern MIXERCONTROL g_mixerControl;                // 0x1029c458
extern int g_paramValue[MIXER_CACHE_SIZE];         // 0x10297398 cached GetParam results
extern unsigned char g_paramCached[MIXER_CACHE_SIZE];   // 0x1029b398 1 once slot was read

extern "C" int MixOpen(void);
extern "C" int MixClose(void);
extern "C" int GetParam(int kind, int index);
extern "C" int SetParam(int kind, int index, int value);
void CloseMixerHandle(void);
void ClearWaveCache(void);
int FreeWaveCache(void);

// ---- globals --------------------------------------------------------------------------------------------------

extern int g_seMuted;                              // 0x1007f380: non-zero rejects IPlaySE/IPlaySEB
extern void *g_waveCache[0x30];                    // 0x1007f390: buffers freed by IRelease
extern int g_timerId;                              // 0x100760bc: timeSetEvent handle, -1 = none
extern int g_seVolumeCopy;                         // 0x10282d50: last ISetSEVol value
extern int g_bgmVolumeCopy;                        // 0x10282d54: last ISetBGMVol value
extern int g_memoryMode;                           // 0x10282d58 : ISetMem(second argument) % 10
extern int g_memoryModeA;                          // 0x10282d5c : ISetMem(first argument) % 10, picks the DirectSound buffer location
extern int g_memoryModeB;                          // 0x10282d60 : ISetMem(second argument) / 10
extern float g_listenerPosition[3];                // 0x10282d64: listener position set by SetListener
extern float g_listenerFront[3];                   // 0x10282d70: listener front vector set by SetListener

extern CRITICAL_SECTION g_audioLock;               // 0x1029c528 guards the music channels
extern unsigned char g_commandRead;                // 0x1029c540 next entry TimerCallback executes
extern HWND g_window;                              // 0x1029c544 the game window
extern unsigned char g_commandWrite;               // 0x1029c548 next entry the API fills
extern unsigned char g_shutdownState;              // 0x1029c549 0 running, 1 release requested, 2 acknowledged
extern CRITICAL_SECTION g_seLock;                  // 0x1029c54c guards the effect channels
extern SoundCommand g_commandQueue[COMMAND_QUEUE_SIZE];   // 0x1029c568

extern int g_commandOverflow;                      // 0x102b8568
extern int g_seVolume;                             // 0x102b856c
extern int g_bgmVolume;                            // 0x102b8570
extern int g_ready;                                // 0x102b8574: 1 between IInit and IRelease
extern SoundChannel g_master;                      // 0x102b8578
extern SoundChannel g_bgmChannel[BGM_CHANNEL_COUNT];   // 0x102b87e0
extern SoundChannel g_seChannel[SE_CHANNEL_COUNT];     // 0x102b8ca8

#endif
