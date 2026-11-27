// sound_play.cpp : SoundChannel::PlaySE (0x10006810), the one function of sound3d.dll built without optimisation
// (/Od /Oi: ebp frame, every variable in memory), so it lives in src_od/.
#include <string.h>
#include "sound_engine.h"
#include "sound_cache.h"

#pragma intrinsic(memset, memcpy)

const int LOCK_WHOLE_BUFFER_OFFSET = 0;
const BYTE SILENCE_FILL = 0xff;                 // the fill byte that pads a sound shorter than its buffer
// Float constants are macros: cl emits a literal operand (an immediate store) for them.
#define PI_FLOAT 3.1415927f                     /* 0x40490fda */
const float ANGLE_WRAP = 2.0f;                  // angles are wrapped to +-2 pi
const float ANGLE_WRAP_LOW = -2.0f;
#define PAN_LIMIT_LOW -10000.0f
#define PAN_LIMIT_HIGH 10000.0f
const int CHANNELS_STEREO = 2;
const int SE_PLAY_TERMINATE_BY_PRIORITY = 8;    // DSBPLAY flag of every effect unless the memory mode is set
const int SE_FLAGS_SOFTWARE = 0xc4;
const int SE_FLAGS_HARDWARE = 0xc8;
const int SE_FLAGS_DEFAULT = 0x400c0;
const int SE_FLAGS_3D_SOFTWARE = 0x20014;
const int SE_FLAGS_3D_HARDWARE = 0x20018;
const int SE_FLAGS_3D_DEFAULT = 0x60010;
const int MS_PER_SECOND = 1000;
const int POSITION_MODE_SOFTWARE_PAN = 2;      // hasPos value that selects the software pan
const int POSITION_MODE_3D = 1;                // hasPos value that selects a DirectSound 3D buffer
const int MEMORY_MODE_HARDWARE = 2;
const int MEMORY_MODE_SOFTWARE = 1;

// Plays a sound effect from the cache (FindOrLoadSound) on this channel: copies the PCM data into a fresh DirectSound
// buffer made for it (a mono 3D buffer when a position is given and the sound is stereo, a plain buffer with software
// pan or volume otherwise), sets the volume or the position, and starts it. loop makes the buffer loop; hasPos is 1
// for a DirectSound 3D position, 2 for the software pan, anything else for no position. Returns 1 when the sound is
// playing, 0 when no device is attached, the file cannot be loaded or DirectSound refuses the buffer.
// Called by IPlaySEB.
int SoundChannel::PlaySE(const char *name, int loop, float *x, float *y, float *z, float *vx, float *vy, float *vz,
                         int hasPos, float vol)
{
    IDirectSoundBuffer *created;
    int mono;
    unsigned dataBytes;
    int slot;
    int flags;
    DSBUFFERDESC description;
    int stereo;
    HRESULT result;
    int unusedFlag;
    ULONG releaseResult;

    if (directSound == 0) {
        return 0;
    }
    if (primaryBuffer == 0) {
        return 0;
    }
    slot = FindOrLoadSound(name);
    if (slot < 0) {
        return 0;
    }
    fileHeader = g_waveCacheHeader[slot];
    dataBytes = fileHeader.dataSize;
    stereo = 1 < fileHeader.channels;
    if (loop != 0) {
        unusedFlag = 0;
        unusedFlag = 0xf;
        unusedFlag += unusedFlag;
    }
    memset(&format, 0, sizeof(format));
    format.wf.wFormatTag = WAVE_FORMAT_PCM;
    format.wf.nChannels = (stereo != 0) ? 2 : 1;
    format.wf.nSamplesPerSec = fileHeader.samplesPerSecond;
    format.wf.nBlockAlign = fileHeader.blockAlign;
    format.wf.nAvgBytesPerSec = format.wf.nSamplesPerSec * format.wf.nBlockAlign;
    format.wBitsPerSample = fileHeader.bitsPerSample;
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(DSBUFFERDESC);
    flags = 0;
    if (g_memoryMode == 0) {
        flags = SE_PLAY_TERMINATE_BY_PRIORITY;
    } else {
        flags = 0;
    }
    if (x != 0 && hasPos == POSITION_MODE_3D) {
        if (g_memoryMode == MEMORY_MODE_HARDWARE) {
            description.dwFlags = SE_FLAGS_3D_SOFTWARE;
        } else if (g_memoryMode == MEMORY_MODE_SOFTWARE) {
            description.dwFlags = SE_FLAGS_3D_HARDWARE;
        } else {
            description.dwFlags = SE_FLAGS_3D_DEFAULT;
        }
        if (g_memoryModeB == 0) {
            description.guid3DAlgorithm = DS3DALG_HRTF_LIGHT;
        } else {
            description.guid3DAlgorithm = DS3DALG_HRTF_FULL;
        }
    } else {
        if (g_memoryMode == MEMORY_MODE_HARDWARE) {
            description.dwFlags = SE_FLAGS_SOFTWARE;
        } else if (g_memoryMode == MEMORY_MODE_SOFTWARE) {
            description.dwFlags = SE_FLAGS_HARDWARE;
        } else {
            description.dwFlags = SE_FLAGS_DEFAULT;
        }
    }
    mono = 0;
    description.dwBufferBytes = dataBytes;
    description.lpwfxFormat = (WAVEFORMATEX *)&format;
    if (hasPos == POSITION_MODE_3D && x != 0 && description.lpwfxFormat->nChannels >= CHANNELS_STEREO) {
        description.lpwfxFormat->nChannels = 1;
        description.lpwfxFormat->nAvgBytesPerSec = description.lpwfxFormat->nAvgBytesPerSec >> 1;
        description.lpwfxFormat->nBlockAlign = description.lpwfxFormat->nBlockAlign / 2;
        dataBytes = dataBytes >> 1;
        description.dwBufferBytes = dataBytes;
        mono = 1;
    }
    if (buffer != 0) {
        IsPlaying();
        buffer->Stop();
        m_01c8 = 0;
        releaseResult = buffer->Release();
        buffer = 0;
        if (buffer3d != 0) {
            buffer3d->Release();
        }
        buffer3d = 0;
        m_0204 = 0;
    }
    created = 0;
    if (buffer == 0) {
        directSound->CreateSoundBuffer(&description, &created, 0);
    }
    if (created == 0) {
        return 0;
    }
    result = created->QueryInterface(IID_IDirectSoundBuffer8, (void **)&buffer);
    created->Release();
    if (buffer == 0) {
        return 0;
    }
    if (x != 0 && hasPos == POSITION_MODE_3D) {
        result = buffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&buffer3d);
    }
    usesSoftware3D = 0;
    distanceScale = vol;
    if (buffer3d == 0) {
        BYTE *region1;
        BYTE *region2;
        DWORD size1;
        DWORD size2;
        HRESULT lockResult;
        lockResult = buffer->Lock(LOCK_WHOLE_BUFFER_OFFSET, dataBytes, (void **)&region1, &size1, (void **)&region2,
                                  &size2, 0);
        if (lockResult == 0) {
            memcpy(region1, g_waveCache[slot], dataBytes);
            if (size1 - dataBytes) {
                memset(region1 + dataBytes, SILENCE_FILL, size1 - dataBytes);
            }
            buffer->Unlock(region1, size1, region2, size2);
            endOffset = size1;
            if (hasPos == POSITION_MODE_SOFTWARE_PAN) {
                float distance;
                float attenuation;
                float relativeX;
            float relativeY;
                    float soundAngle;
                float frontAngle;
                float pi;
                float pan;

                worldPosition[0] = *x;
                worldPosition[1] = *y;
                worldPosition[2] = *z;
                usesSoftware3D = 1;
                distance = (float)sqrt((worldPosition[0] - g_listenerPosition[0]) * (worldPosition[0] - g_listenerPosition[0]) +
                                       (worldPosition[1] - g_listenerPosition[1]) * (worldPosition[1] - g_listenerPosition[1]) +
                                       (worldPosition[2] - g_listenerPosition[2]) * (worldPosition[2] - g_listenerPosition[2]));
                attenuation = (SOFT3D_FAR_DISTANCE - distance * distanceScale) / SOFT3D_VOLUME_DIVISOR;
                relativeX = worldPosition[0] - g_listenerPosition[0];
                    relativeY = worldPosition[1] - g_listenerPosition[1];
            soundAngle = (float)atan2(relativeX, relativeY);
                frontAngle = (float)atan2(g_listenerFront[0], g_listenerFront[1]);
                pi = PI_FLOAT;
                while (frontAngle > ANGLE_WRAP * pi) {
                    frontAngle = frontAngle - ANGLE_WRAP * pi;
                }
                while (frontAngle < ANGLE_WRAP_LOW * pi) {
                    frontAngle = frontAngle + ANGLE_WRAP * pi;
                }
                while (soundAngle > ANGLE_WRAP * pi) {
                    soundAngle = soundAngle - ANGLE_WRAP * pi;
                }
                while (soundAngle < ANGLE_WRAP_LOW * pi) {
                    soundAngle = soundAngle + ANGLE_WRAP * pi;
                }
                pan = soundAngle - frontAngle;
                if (sin(pan) < 0.0) {
                    pan = (float)-(sin(pan) * sin(pan) * SOFT3D_PAN_GAIN);
                } else {
                    pan = (float)(sin(pan) * sin(pan) * SOFT3D_PAN_GAIN);
                }
                ApplyVolume((int)max(0.0f, VOLUME_SCALE_MAX - attenuation * SOFT3D_VOLUME_STEP), 1);
                buffer->SetPan((LONG)min(max(PAN_LIMIT_LOW, pan * SOFT3D_PAN_SCALE), PAN_LIMIT_HIGH));
            } else if (x == 0) {
                ApplyVolume((int)max(VOLUME_SCALE_MAX - (VOLUME_SCALE_MAX - g_seVolumeCopy) * distanceScale, 0.0f), 1);
            }
            buffer->Restore();
            if (loop != 0) {
                buffer->Play(0, 0, flags | DSBPLAY_LOOPING);
            } else {
                buffer->Play(0, 0, flags);
            }
            m_0204 = GetTickCount() + (size1 / format.wf.nAvgBytesPerSec + 1) * MS_PER_SECOND;
            if (loop != 0) {
                m_0204 = 0;
            }
            m_01c8 = 1;
        } else {
            buffer->Release();
            buffer = 0;
            if (buffer3d != 0) {
                buffer3d->Release();
            }
            buffer3d = 0;
            m_01c8 = 0;
            m_0204 = 0;
            return 0;
        }
    } else {
        if (buffer3d != 0) {
            DS3DBUFFER params;

            params.dwSize = sizeof(DS3DBUFFER);
            buffer3d->GetAllParameters(&params);
            params.dwMode = DS3DMODE_NORMAL;
            params.flMinDistance = SE_MIN_DISTANCE;
            params.flMaxDistance = SE_MAX_DISTANCE;
            params.dwInsideConeAngle = CONE_ANGLE_OMNI;
            params.dwOutsideConeAngle = CONE_ANGLE_OMNI;
            if (vx != 0) {
                velocity[0] = *vx;
                velocity[1] = *vy;
                velocity[2] = *vz;
            } else {
                velocity[0] = 0;
                velocity[1] = 0;
                velocity[2] = 0;
            }
            params.vVelocity.x = velocity[0];
            params.vVelocity.y = velocity[1];
            params.vVelocity.z = velocity[2];
            worldPosition[0] = *x;
            worldPosition[1] = *y;
            worldPosition[2] = *z;
            params.vPosition.x = (worldPosition[0] - g_listenerPosition[0]) * distanceScale + g_listenerPosition[0];
            params.vPosition.y = (worldPosition[1] - g_listenerPosition[1]) * distanceScale + g_listenerPosition[1];
            params.vPosition.z = (worldPosition[2] - g_listenerPosition[2]) * distanceScale + g_listenerPosition[2];
            result = buffer3d->SetAllParameters(&params, DS3D_IMMEDIATE);
        }
        {
            BYTE *region1;
            BYTE *region2;
            DWORD size1;
            DWORD size2;
            HRESULT lockResult;
            lockResult = buffer->Lock(LOCK_WHOLE_BUFFER_OFFSET, dataBytes, (void **)&region1, &size1,
                                      (void **)&region2, &size2, 0);
            if (lockResult == 0) {
                if (mono != 0) {
                    WORD *source = (WORD *)g_waveCache[slot];
                    WORD *destination = (WORD *)region1;
                    unsigned i;
                    for (i = 0; i < dataBytes >> 1; i++) {
                        destination[i] = source[i * 2];
                    }
                    if (size1 - dataBytes) {
                        memset(region1 + dataBytes, SILENCE_FILL, size1 - dataBytes);
                    }
                } else {
                    memcpy(region1, g_waveCache[slot], dataBytes);
                    if (size1 - dataBytes) {
                        memset(region1 + dataBytes, SILENCE_FILL, size1 - dataBytes);
                    }
                }
                buffer->Unlock(region1, size1, region2, size2);
                endOffset = size1;
                if (hasPos == POSITION_MODE_SOFTWARE_PAN) {
                } else if (x == 0) {
                    ApplyVolume(g_seVolumeCopy, 1);
                }
                buffer->Restore();
                if (loop != 0) {
                    buffer->Play(0, 0, flags | DSBPLAY_LOOPING);
                } else {
                    buffer->Play(0, 0, flags);
                }
                m_0204 = GetTickCount() + (size1 / format.wf.nAvgBytesPerSec + 1) * MS_PER_SECOND;
                if (loop != 0) {
                    m_0204 = 0;
                }
                m_01c8 = 1;
            } else {
                buffer->Release();
                buffer = 0;
                if (buffer3d != 0) {
                    buffer3d->Release();
                }
                buffer3d = 0;
                m_01c8 = 0;
                m_0204 = 0;
                return 0;
            }
        }
    }
    isPlaying = 0;
    return 1;
}
