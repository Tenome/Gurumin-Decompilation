// sound_channel.h : SoundChannel, one DirectSound voice of sound3d.dll (0x264 bytes), built on the WaveStream it
// streams from.
//
// The engine keeps 27 of them (sound_engine.h): the master that owns the DirectSound device, two music channels and
// 24 sound-effect channels. The methods are defined in sound_channel.cpp (0x100058a0-0x10008090), sound_bgm.cpp
// (music operations), sound_init.cpp (Init) and are called by the exported API in sound_api.cpp.
#ifndef SOUND_CHANNEL_H
#define SOUND_CHANNEL_H

#include <windows.h>
#include <math.h>
#include <prerelease/dsound.h>
#include "wave_stream.h"

// ---- constants -----------------------------------------------------------------------------------------------

const int VOLUME_SCALE_MAX = 10000;         // game volume scale: 0 (silent) .. 10000 (full), also the fade end value
const int CHANNEL_SECTION_COUNT = 3;        // the DirectSound buffer holds three one-second sections
const int DEBUG_MESSAGE_SIZE = 0x100;      // OutputDebugString text buffer of ReleaseBuffers
// Float constants are macros: cl emits a literal operand for them, a `const float` object is loaded and negated at run time.
#define TWO_PI_FLOAT 6.283185f              /* 0x40c90fdb: angle wrap period of the software 3D pan */
const float SOFT3D_FAR_DISTANCE = 800.0f;   // distance (scaled) at which the software 3D sound is fully attenuated
const float SOFT3D_VOLUME_DIVISOR = 11.0f;  // (far - distance) / divisor, times 100, is the attenuation in volume steps
const float SOFT3D_VOLUME_STEP = 100.0f;
const double SOFT3D_PAN_GAIN = 1.6;         // pan = sin(angle)^2 * gain, signed
const float SOFT3D_PAN_SCALE = 1500.0f;     // pan is multiplied by this to get DirectSound pan units
const float PAN_FULL = 10000.0f;            // DirectSound pan range is -10000..10000
const int CONE_ANGLE_OMNI = 360;            // inside and outside cone angle: the sound is not directional
#define SE_MIN_DISTANCE 50.0f               /* DirectSound 3D buffer min distance of sound effects (a literal: stored as an immediate) */
#define SE_MAX_DISTANCE 800.0f              /* DirectSound 3D buffer max distance of sound effects */

class SoundChannel : public WaveStream {
public:
    float worldPosition[3];             // +0x1a4 3D position of the sound (x, y, z)
    float velocity[3];                  // +0x1b0 3D velocity or offset
    float distanceScale;                // +0x1bc how strongly the distance moves the 3D parameters
    int usesSoftware3D;                 // +0x1c0 non-zero: pan and volume are computed from position (no DS3D buffer)
    int isPlaying;                      // +0x1c4 non-zero while a stream is attached and running (IGetPos)
    int m_01c8;                         // +0x1c8 cleared by the constructor and Stop, read by IsPlaying
    int isLooping;                      // +0x1cc non-zero: the stream loops between loopStart and loopEnd
    int endOffset;                      // +0x1d0 file offset of the end of the PCM data (data size + header)
    int loopStartBytes;                 // +0x1d4 loop start inside the PCM data, -1 = from the beginning
    int loopEndBytes;                   // +0x1d8 loop end inside the PCM data
    int volume;                         // +0x1dc current volume 0..10000
    int stopWhenFaded;                  // +0x1e0 non-zero: a finished fade stops the channel
    int fadeTarget;                     // +0x1e4 volume the fade ends at
    DWORD fadeTick;                     // +0x1e8 GetTickCount at the last fade step
    int fadeRemainingMs;                // +0x1ec time left of the fade, 0 = no fade
    int sectionFilled[3];               // +0x1f0 per section: refill pending flags of the three buffer thirds
    int m_01fc;                         // +0x1fc
    DWORD startTick;                    // +0x200 GetTickCount when the channel started
    int m_0204;                         // +0x204 tick of the last end-of-stream check
    PCMWAVEFORMAT format;               // +0x208 format of the DirectSound buffer (nAvgBytesPerSec = one section)
    WavHeader fileHeader;               // +0x218 RIFF/WAVE header of the file being played (read by Open)
    DWORD playCursor;                   // +0x244 DirectSound play cursor at the last update
    IDirectSound8 *directSound;         // +0x248 the device (created by the master's Init, shared by the rest)
    int sharesDirectSound;              // +0x24c non-zero: the device belongs to another channel, do not release
    IDirectSoundBuffer *primaryBuffer;  // +0x250
    int sharesPrimaryBuffer;            // +0x254 non-zero: the primary buffer belongs to another channel
    IDirectSoundBuffer *buffer;         // +0x258 streaming secondary buffer
    IDirectSound3DBuffer *buffer3d;     // +0x25c
    IDirectSound3DListener *listener;   // +0x260

    SoundChannel();                                         // 0x100058a0 (run for the global channel tables)
    int IsPlaying();                                        // 0x100058f0
    int SetWindow(HWND window);                             // 0x10005970
    int SetListener(float rolloff, float x, float y, float z, float frontX, float frontY, float frontZ,
                    float topX, float topY, float topZ);    // 0x10005990
    int SetLoopPos(int startSamples, int endSamples);       // 0x10005ad0
    int Commit();                                           // 0x10005b30
    int GetPosition();                                      // 0x10005b40
    int Stop();                                             // 0x10005c60
    int ApplyVolume(int newVolume, int force);              // 0x10005cd0
    void CancelFade();                                      // 0x10005d80
    int SetVolume(int newVolume);                           // 0x10005da0
    int StartFade(int target, int durationMs);              // 0x10005dd0
    int FillSection(DWORD bytes, int reserved, DWORD offset);  // 0x10005e30
    int UpdateFade();                                       // 0x100060b0
    int ReleaseBuffers();                                   // 0x10006700
    int Update3DPan();                                      // 0x10007840
    int UpdateSE();                                         // 0x10007b50
    int SetPosition(float *x, float *y, float *z, float *vx, float *vy, float *vz);  // 0x10007c50
    int Open(const char *file, int loop, int startSamples, int loopStartSamples,
             unsigned int loopEndSamples);                  // 0x10007dd0
    int PlaySE(const char *name, int loop, float *x, float *y, float *z, float *vx, float *vy, float *vz, int hasPos,
               float vol);                                  // not reconstructed yet

    // Music channel operations (sound_bgm.cpp)
    int FadeOut();                                          // 0x10008090
    int FadeOutEx(int durationMs);                          // 0x100080d0
    int PlayB(const char *file, int loop, int startSamples, int loopStartSamples, int loopEndSamples);   // 0x10008110
    int PlayFadeB(const char *file, int loop, int startSamples, int loopStartSamples,
                  int loopEndSamples);                      // 0x100081e0
    int PlayFadeBEx(const char *file, int loop, int startSamples, int loopStartSamples, int loopEndSamples,
                    int fadeMs);                            // 0x100082b0
    int Init(HWND window, IDirectSound8 *sharedDevice, IDirectSoundBuffer *sharedPrimary);   // 0x10008360
};

#endif
