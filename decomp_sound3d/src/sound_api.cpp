// sound_api.cpp : the exported I... functions of sound3d.dll that SoundArc calls (see decomp_soundarc).
//
// Play commands for music and effects are copied into a ring of SoundCommand entries and executed later by
// ProcessCommandQueue on the 10 ms timer; the "B" variants run immediately under the channel lock. Volume, position
// and stop calls act directly on the channels.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sound_engine.h"

// Copies a zero-terminated string (the file name of a queued command).
inline void CopyString(char *dest, const char *src)
{
    while ((*dest++ = *src++) != 0)
        ;
}

// IReInit: hands the game window to the DirectSound device again (SoundArc calls it when the game window changes).
extern "C" int IReInit(void)
{
    g_master.SetWindow(g_window);
    return 0;
}

// ITimeGetTime: the multimedia clock in milliseconds, 0 before IInit.
extern "C" int ITimeGetTime(void)
{
    if (!g_ready)
        return 0;
    return timeGetTime();
}

// IStopSE: stops effect channel 0..11 immediately.
extern "C" int IStopSE(int channel)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > SE_STOP_LAST_INDEX)
        return 0;
    EnterCriticalSection(&g_seLock);
    g_seChannel[channel].Stop();
    LeaveCriticalSection(&g_seLock);
    return 1;
}

// ISetPlayer: accepted and ignored.
extern "C" int ISetPlayer(void)
{
    return 1;
}

// ISetSEPos: moves a playing effect (position and velocity are float pointers).
extern "C" int ISetSEPos(int channel, float *x, float *y, float *z, float *vx, float *vy, float *vz)
{
    if (channel < 0)
        return 0;
    if (channel > SE_POS_LAST_INDEX)
        return 0;
    g_seChannel[channel].SetPosition(x, y, z, vx, vy, vz);
    return 1;
}

// ISetSEVol: master volume of the effects.
extern "C" int ISetSEVol(int volume)
{
    if (!g_ready)
        return 0;
    g_seVolume = volume;
    g_seVolumeCopy = volume;
    return 1;
}

// ISetBGMVol: master volume of the music, applied to both music channels at once.
extern "C" int ISetBGMVol(int volume)
{
    if (!g_ready)
        return 0;
    EnterCriticalSection(&g_audioLock);
    g_bgmVolume = volume;
    g_bgmVolumeCopy = volume;
    for (int i = 0; i < BGM_CHANNEL_COUNT; i++)
        g_bgmChannel[i].SetVolume(g_bgmVolume);
    LeaveCriticalSection(&g_audioLock);
    return 1;
}

// IReset: reports whether IInit succeeded.
extern "C" int IReset(void)
{
    return g_ready != 0;
}

// ISetMem: stores the two memory-mode digits the effect loader reads.
extern "C" int ISetMem(int first, int second)
{
    g_memoryMode = second % 10;
    g_memoryModeA = first % 10;
    g_memoryModeB = second / 10;
    return 1;
}

// IEnter / ILeave: let the game hold the music lock around a group of calls.
extern "C" int IEnter(void)
{
    if (!g_ready)
        return 0;
    EnterCriticalSection(&g_audioLock);
    return 1;
}

extern "C" int ILeave(void)
{
    if (!g_ready)
        return 0;
    LeaveCriticalSection(&g_audioLock);
    return 1;
}

// IGetPos: playback position of music channel 0/1, 0 when nothing is attached.
extern "C" int IGetPos(int channel)
{
    if (!g_ready || channel < 0 || channel > BGM_LAST_CHANNEL || g_bgmChannel[channel].isPlaying == 0)
        return 0;
    return g_bgmChannel[channel].GetPosition();
}

// IVolFade: fades music channel 0/1 to a volume over a time.
extern "C" int IVolFade(int channel, int target, int durationMs)
{
    if (!g_ready || channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    g_bgmChannel[channel].StartFade(target, durationMs);
    return 1;
}

// ISetSEVolume: sets the volume of one effect channel (0..11).
extern "C" int ISetSEVolume(int channel, int volume)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > SE_STOP_LAST_INDEX)
        return 0;
    EnterCriticalSection(&g_seLock);
    g_seChannel[channel].CancelFade();
    g_seChannel[channel].SetVolume(volume);
    g_seChannel[channel].Commit();
    LeaveCriticalSection(&g_seLock);
    return 1;
}

// IPlaySE: queues a sound effect. Position (x, y, z) is used only when hasPos is set; loop = 1 makes the command
// never expire. Always returns -1 (the channel is chosen later).
extern "C" int IPlaySE(const char *name, float *x, float *y, float *z, int hasPos, int loop, float volume)
{
    if (g_seMuted != 0)
        return -1;
    if (!g_ready)
        return -1;
    CopyString(g_commandQueue[g_commandWrite].name, name);
    g_commandQueue[g_commandWrite].command = CMD_PLAY_SE;
    if (hasPos == 0 || x == 0) {
        g_commandQueue[g_commandWrite].value[0] = 0;
        g_commandQueue[g_commandWrite].value[1] = 0;
        g_commandQueue[g_commandWrite].value[2] = 0;
    } else {
        g_commandQueue[g_commandWrite].value[0] = *x;
        g_commandQueue[g_commandWrite].value[1] = *y;
        g_commandQueue[g_commandWrite].value[2] = *z;
    }
    g_commandQueue[g_commandWrite].arg3 = hasPos;
    g_commandQueue[g_commandWrite].arg4 = loop;
    g_commandQueue[g_commandWrite].value[5] = volume;
    g_commandQueue[g_commandWrite].queuedAt = GetTickCount();
    if (loop != 0)
        g_commandQueue[g_commandWrite].queuedAt = 0;
    g_commandWrite++;
    return -1;
}

// IPlaySEB: plays a sound effect immediately on the first idle effect channel. Returns the channel number, -1 when
// all 24 channels are busy or the engine is off.
extern "C" int IPlaySEB(const char *name, float *x, float *y, float *z, int hasPos, int loop, float volume)
{
    if (g_seMuted != 0)
        return -1;
    if (!g_ready)
        return -1;
    EnterCriticalSection(&g_seLock);
    int channel;
    for (channel = 0; channel < SE_CHANNEL_COUNT; channel++) {
        if (!g_seChannel[channel].IsPlaying()) {
            g_seChannel[channel].PlaySE(name, loop, x, y, z, 0, 0, 0, hasPos, volume);
            break;
        }
    }
    LeaveCriticalSection(&g_seLock);
    if (channel == SE_CHANNEL_COUNT)
        return -1;
    return channel;
}

// ISetListen: queues the 3D listener position and orientation (ten floats).
extern "C" int ISetListen(float first, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10)
{
    if (g_ready) {
        g_commandQueue[g_commandWrite].command = CMD_SET_LISTENER;
        g_commandQueue[g_commandWrite].value[0] = p2;
        g_commandQueue[g_commandWrite].value[1] = p3;
        g_commandQueue[g_commandWrite].value[2] = p4;
        g_commandQueue[g_commandWrite].value[3] = p5;
        g_commandQueue[g_commandWrite].value[4] = p6;
        g_commandQueue[g_commandWrite].value[5] = p7;
        g_commandQueue[g_commandWrite].value[6] = p8;
        g_commandQueue[g_commandWrite].value[7] = p9;
        g_commandQueue[g_commandWrite].value[8] = p10;
        g_commandQueue[g_commandWrite].listenerFirst = first;
        g_commandQueue[g_commandWrite++].queuedAt = GetTickCount();
    }
    return 0;
}

// IStopAllSE: stops every effect channel that is playing.
extern "C" int IStopAllSE(void)
{
    if (g_ready) {
        EnterCriticalSection(&g_seLock);
        for (int i = 0; i < SE_CHANNEL_COUNT; i++) {
            if (g_seChannel[i].IsPlaying())
                g_seChannel[i].Stop();
        }
        LeaveCriticalSection(&g_seLock);
    }
    return 0;
}

// IPlayFadeEx: queues a music file for channel 0/1 (the extra argument is stored but ProcessCommandQueue ignores it,
// as the command code is the plain IPlayFade one).
extern "C" int IPlayFadeEx(int channel, const char *name, int a, int b, int c, int d, int e)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    CopyString(g_commandQueue[g_commandWrite].name, name);
    g_commandQueue[g_commandWrite].command = CMD_PLAY_FADE;
    g_commandQueue[g_commandWrite].channel = channel;
    g_commandQueue[g_commandWrite].arg1 = a;
    g_commandQueue[g_commandWrite].arg2 = b;
    g_commandQueue[g_commandWrite].arg3 = c;
    g_commandQueue[g_commandWrite].arg4 = d;
    g_commandQueue[g_commandWrite].arg5 = e;
    g_commandQueue[g_commandWrite].queuedAt = 0;
    g_commandWrite++;
    return -1;
}

// IPlayFadeBEx: plays a music file at once with a fade-in of fadeMs.
extern "C" int IPlayFadeBEx(int channel, const char *name, int a, int b, int c, int d, int fadeMs)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    int result = g_bgmChannel[channel].PlayFadeBEx(name, d, a, b, c, fadeMs);
    LeaveCriticalSection(&g_audioLock);
    return result;
}

// IPlayFade: queues a music file with a fade-in for channel 0/1.
extern "C" int IPlayFade(int channel, const char *name, int a, int b, int c, int d)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    CopyString(g_commandQueue[g_commandWrite].name, name);
    g_commandQueue[g_commandWrite].command = CMD_PLAY_FADE;
    g_commandQueue[g_commandWrite].channel = channel;
    g_commandQueue[g_commandWrite].arg1 = a;
    g_commandQueue[g_commandWrite].arg2 = b;
    g_commandQueue[g_commandWrite].arg3 = c;
    g_commandQueue[g_commandWrite].arg4 = d;
    g_commandQueue[g_commandWrite].queuedAt = 0;
    g_commandWrite++;
    return -1;
}

// IPlayFadeB: plays a music file at once with the default fade-in.
extern "C" int IPlayFadeB(int channel, const char *name, int a, int b, int c, int d)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    int result = g_bgmChannel[channel].PlayFadeB(name, d, a, b, c);
    LeaveCriticalSection(&g_audioLock);
    return result;
}

// IPlayB: plays a music file at once without a fade.
extern "C" int IPlayB(int channel, const char *name, int a, int b, int c, int d)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    int result = g_bgmChannel[channel].PlayB(name, d, a, b, c);
    LeaveCriticalSection(&g_audioLock);
    return result;
}

// IPlayStop: stops music channel 0/1.
extern "C" int IPlayStop(int channel)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    int result = g_bgmChannel[channel].Stop();
    LeaveCriticalSection(&g_audioLock);
    return result;
}

// ISetLoopPos: sets the loop start and end of music channel 0/1.
extern "C" void ISetLoopPos(int channel, int start, int end)
{
    if (g_ready && channel >= 0 && channel <= BGM_LAST_CHANNEL) {
        EnterCriticalSection(&g_audioLock);
        g_bgmChannel[channel].SetLoopPos(start, end);
        LeaveCriticalSection(&g_audioLock);
    }
}

// IFadeOut: fades music channel 0/1 out over the default time.
extern "C" int IFadeOut(int channel)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    int result = g_bgmChannel[channel].FadeOut();
    LeaveCriticalSection(&g_audioLock);
    return result;
}

// IFadeOutEx: fades music channel 0/1 out over durationMs.
extern "C" int IFadeOutEx(int channel, int durationMs)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    int result = g_bgmChannel[channel].FadeOutEx(durationMs);
    LeaveCriticalSection(&g_audioLock);
    return result;
}

// ProcessCommandQueue: runs every queued command between g_commandRead and g_commandWrite, then updates the fades of
// the music channels. Called by TimerCallback every 10 ms. Commands older than a second are dropped. Queued effect
// plays of the same file are merged (only the last one of a burst plays). Stops early, acknowledging with
// g_shutdownState = 2, when IRelease asked for shutdown.
void ProcessCommandQueue(int unused)
{
    unsigned char writeIndex = g_commandWrite;
    unsigned char index = g_commandRead;
    while (writeIndex != index) {
        if (g_commandQueue[index].queuedAt != 0 && g_commandQueue[index].queuedAt < GetTickCount() - COMMAND_EXPIRY_MS)
            g_commandQueue[index].command = CMD_NONE;
        int code = g_commandQueue[index].command;
        if (code == CMD_PLAY_FADE_EX) {
            EnterCriticalSection(&g_audioLock);
            g_bgmChannel[g_commandQueue[index].channel].PlayFadeBEx(g_commandQueue[index].name,
                g_commandQueue[index].arg4, g_commandQueue[index].arg1, g_commandQueue[index].arg2,
                g_commandQueue[index].arg3, g_commandQueue[index].arg5);
            g_bgmChannel[g_commandQueue[index].channel].UpdateFade();
            g_commandQueue[g_commandRead].command = CMD_NONE;
            LeaveCriticalSection(&g_audioLock);
        } else if (code == CMD_PLAY_FADE) {
            EnterCriticalSection(&g_audioLock);
            g_bgmChannel[g_commandQueue[index].channel].PlayFadeB(g_commandQueue[index].name,
                g_commandQueue[index].arg4, g_commandQueue[index].arg1, g_commandQueue[index].arg2,
                g_commandQueue[index].arg3);
            g_bgmChannel[g_commandQueue[index].channel].UpdateFade();
            g_commandQueue[index].command = CMD_NONE;
            LeaveCriticalSection(&g_audioLock);
        } else if (code == CMD_PLAY) {
            EnterCriticalSection(&g_audioLock);
            g_bgmChannel[g_commandQueue[index].channel].PlayB(g_commandQueue[index].name,
                g_commandQueue[index].arg4, g_commandQueue[index].arg1, g_commandQueue[index].arg2,
                g_commandQueue[index].arg3);
            g_bgmChannel[g_commandQueue[index].channel].UpdateFade();
            g_commandQueue[index].command = CMD_NONE;
            LeaveCriticalSection(&g_audioLock);
        } else if (code == CMD_SET_LISTENER) {
            g_master.SetListener(g_commandQueue[index].listenerFirst, g_commandQueue[index].value[0],
                g_commandQueue[index].value[1], g_commandQueue[index].value[2], g_commandQueue[index].value[3],
                g_commandQueue[index].value[4], g_commandQueue[index].value[5], g_commandQueue[index].value[6],
                g_commandQueue[index].value[7], g_commandQueue[index].value[8]);
            g_commandQueue[index].command = CMD_NONE;
            for (int i = 0; i < SE_CHANNEL_COUNT; i++)
                g_seChannel[i].UpdateSE();
        } else if (code == CMD_PLAY_SE) {
            for (unsigned char other = index; writeIndex != other; other++) {
                if (other != index && g_commandQueue[other].command == CMD_PLAY_SE &&
                    stricmp(g_commandQueue[index].name, g_commandQueue[other].name) == 0)
                    g_commandQueue[other].command = CMD_NONE;
            }
            float volume = g_commandQueue[index].value[5];
            int hasPos = g_commandQueue[index].arg3;
            int loop = g_commandQueue[index].arg4;
            if (hasPos != 0) {
                IPlaySEB(g_commandQueue[index].name, &g_commandQueue[index].value[0], &g_commandQueue[index].value[1],
                         &g_commandQueue[index].value[2], hasPos, loop, volume);
            } else {
                IPlaySEB(g_commandQueue[index].name, 0, 0, 0, 0, loop, volume);
            }
            g_commandQueue[index].command = CMD_NONE;
        }
        if (g_shutdownState != 0) {
            g_shutdownState = 2;
            return;
        }
        index++;
    }
    g_commandRead = index;
    if (g_ready) {
        for (int i = 0; i < BGM_CHANNEL_COUNT; i++) {
            EnterCriticalSection(&g_audioLock);
            g_bgmChannel[i].UpdateFade();
            LeaveCriticalSection(&g_audioLock);
        }
    }
}

// TimerCallback: the multimedia timer procedure created by IInit; just pumps the command queue.
void CALLBACK TimerCallback(UINT timerId, UINT message, DWORD user, DWORD reserved1, DWORD reserved2)
{
    ProcessCommandQueue(0);
}

// IInit: starts the engine for the game window: creates the DirectSound device (master), initialises the 26
// channels, the two locks and the 10 ms timer. Returns 1 on success, 0 when already started or when DirectSound failed.
extern "C" int IInit(HWND window)
{
    if (g_ready == 1)
        return 0;
    InitTrackedAllocator();
    for (int i = 0; i < 0x30; i++)
        g_waveCache[i] = 0;
    if (g_window != 0)
        return 0;
    g_window = window;
    if (g_master.Init(window, 0, 0) == 0)
        return 0;
    g_commandRead = 0;
    g_commandWrite = 0;
    g_commandOverflow = 0;
    InitializeCriticalSection(&g_audioLock);
    InitializeCriticalSection(&g_seLock);
    timeBeginPeriod(TIMER_PERIOD_MIN_MS);
    g_shutdownState = 0;
    for (int i = 0; i < BGM_CHANNEL_COUNT; i++)
        g_bgmChannel[i].Init(g_window, g_master.directSound, g_master.primaryBuffer);
    for (int i = 0; i < SE_CHANNEL_COUNT; i++)
        g_seChannel[i].Init(g_window, g_master.directSound, g_master.primaryBuffer);
    g_timerId = timeSetEvent(CALLBACK_PERIOD_MS, CALLBACK_RESOLUTION_MS, TimerCallback, 0, TIME_PERIODIC);
    g_ready = 1;
    return 1;
}

// IRelease: stops the timer, releases every channel and the device, deletes the locks and frees cached waves.
extern "C" int IRelease(void)
{
    if (!g_ready)
        return 0;
    g_ready = 0;
    if (g_timerId != -1)
        timeKillEvent(g_timerId);
    g_timerId = -1;
    g_shutdownState = 1;
    for (int i = 0; i < BGM_CHANNEL_COUNT; i++)
        g_bgmChannel[i].ReleaseBuffers();
    for (int i = 0; i < SE_CHANNEL_COUNT; i++)
        g_seChannel[i].ReleaseBuffers();
    g_master.ReleaseBuffers();
    g_window = 0;
    timeEndPeriod(TIMER_PERIOD_MIN_MS);
    DeleteCriticalSection(&g_audioLock);
    DeleteCriticalSection(&g_seLock);
    for (int i = 0; i < 0x30; i++) {
        if (g_waveCache[i] != 0) {
            free(g_waveCache[i]);
            g_waveCache[i] = 0;
        }
    }
    return 1;
}

// IPlay: queues a music file for channel 0/1 (plain play without fade). Before queuing, one second in eight, it
// verifies the game's own files: it sums and xors one of four files in the game directory (app.dll, app8.dll,
// map.itm or sound3d.dll) and compares the result with the two words stored in soundarc.dll at 0x17000 + 8 * test
// number. On a mismatch it posts WM_CLOSE to the game window and returns 0.
extern "C" int IPlay(int channel, const char *name, int a, int b, int c, int d)
{
    if (!g_ready)
        return 0;
    if (channel < 0 || channel > BGM_LAST_CHANNEL)
        return 0;
    char directory[MAX_PATH_BUFFER];
    char checkedPath[MAX_PATH_BUFFER];
    char storePath[MAX_PATH_BUFFER];
    int sum, storedSum;
    unsigned mask, storedMask;
    GetModuleFileName(0, directory, sizeof(directory));
    char *last = directory + strlen(directory) - 1;
    if (*last != '\\') {
        do {
            *last = 0;
            last--;
        } while (*last != '\\');
    }
    int test = (GetTickCount() / 1000) & 7;
    if (test == 0) {
        sprintf(checkedPath, "%s%s%s%s", directory, "ap", "p.d", "ll");
        sprintf(storePath, "%s%s%s%s", directory, "soun", "darc.d", "ll");
        ComputeFileChecksum(checkedPath, &sum, &mask);
        ReadFileWords(storePath, INTEGRITY_TABLE_OFFSET, 0, &storedSum, &storedMask);
        if (sum != storedSum || mask != storedMask) {
            PostMessage(g_window, WM_CLOSE, 0, 0);
            return 0;
        }
    } else if (test == 1) {
        sprintf(checkedPath, "%s%s%s%s", directory, "ap", "p8.d", "ll");
        sprintf(storePath, "%s%s%s%s", directory, "sou", "nda", "rc.dll");
        ComputeFileChecksum(checkedPath, &sum, &mask);
        ReadFileWords(storePath, INTEGRITY_TABLE_OFFSET, 1, &storedSum, &storedMask);
        if (sum != storedSum || mask != storedMask) {
            PostMessage(g_window, WM_CLOSE, 0, 0);
            return 0;
        }
    } else if (test == 2) {
        sprintf(checkedPath, "%s%s%s%s", directory, "ma", "p.i", "tm");
        sprintf(storePath, "%s%s%s%s", directory, "sound", "arc.d", "ll");
        ComputeFileChecksum(checkedPath, &sum, &mask);
        ReadFileWords(storePath, INTEGRITY_TABLE_OFFSET, 2, &storedSum, &storedMask);
        if (sum != storedSum || mask != storedMask) {
            PostMessage(g_window, WM_CLOSE, 0, 0);
            return 0;
        }
    } else if (test == 3) {
        sprintf(checkedPath, "%s%s%s%s", directory, "sound", "3d.d", "ll");
        sprintf(storePath, "%s%s%s%s", directory, "sou", "ndar", "c.dll");
        ComputeFileChecksum(checkedPath, &sum, &mask);
        ReadFileWords(storePath, INTEGRITY_TABLE_OFFSET, 3, &storedSum, &storedMask);
        if (sum != storedSum || mask != storedMask) {
            PostMessage(g_window, WM_CLOSE, 0, 0);
            return 0;
        }
    }
    CopyString(g_commandQueue[g_commandWrite].name, name);
    g_commandQueue[g_commandWrite].command = CMD_PLAY;
    g_commandQueue[g_commandWrite].channel = channel;
    g_commandQueue[g_commandWrite].arg1 = a;
    g_commandQueue[g_commandWrite].arg2 = b;
    g_commandQueue[g_commandWrite].arg3 = c;
    g_commandQueue[g_commandWrite].arg4 = d;
    g_commandQueue[g_commandWrite].queuedAt = 0;
    g_commandWrite++;
    return -1;
}
