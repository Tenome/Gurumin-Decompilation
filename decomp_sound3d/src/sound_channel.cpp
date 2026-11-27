// sound_channel.cpp : SoundChannel, one streaming DirectSound voice of sound3d.dll (0x100058a0-0x10008090).
//
// A channel owns a three-section DirectSound buffer (each section is one second of PCM, WAVEFORMAT::nAvgBytesPerSec
// bytes), a WaveStream it decodes from, and volume/fade/3D state. The timer thread calls UpdateFade/UpdateSE for every
// channel every few milliseconds; the exported I... functions (sound_api.cpp) call the other methods.
#include <stdio.h>
#include <string.h>
#include "sound_engine.h"

// x*x + y*y + z*z as an inline helper: its float arguments are rounded to floats first.
inline float LengthSquared(float x, float y, float z)
{
    return x * x + y * y + z * z;
}

// The engine's atan2 wrapper: float arguments (each difference is rounded to a float first), float result.
inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

// Constructor: the WaveStream base is constructed, then the device pointers and the play flags are cleared. Run
// for the global channel tables (g_master, g_bgmChannel, g_seChannel) when the DLL starts.
SoundChannel::SoundChannel()
{
    sharesDirectSound = 0;
    directSound = 0;
    sharesPrimaryBuffer = 0;
    primaryBuffer = 0;
    buffer = 0;
    buffer3d = 0;
    isPlaying = 0;
    m_01c8 = 0;
    listener = 0;
}

// True while the channel is audible: it is started, or it was started and DirectSound still reports the buffer
// playing/looping. Restores a lost buffer first. Called by Stop, SetVolume, StartFade, Open and the API.
int SoundChannel::IsPlaying()
{
    if (buffer == 0)
        return 0;
    DWORD status;
    buffer->GetStatus(&status);
    if (status & DSBSTATUS_BUFFERLOST)
        buffer->Restore();
    if (isPlaying != 0)
        return 1;
    if (m_01c8 == 0)
        return 0;
    if (status & DSBSTATUS_TERMINATED)
        return 0;
    if (status & DSBSTATUS_PLAYING)
        return 1;
    if (status & DSBSTATUS_LOOPING)
        return 1;
    if (m_0204 != 0)
        // The original measured the idle time here but never used it.
        DWORD idleMs = GetTickCount() - m_0204;
    return 0;
}

// Gives the DirectSound device the window that decides audio focus (exclusive cooperative level). Called by
// IReInit. Always returns 0.
int SoundChannel::SetWindow(HWND window)
{
    if (directSound != 0)
        directSound->SetCooperativeLevel(window, DSSCL_EXCLUSIVE);
    return 0;
}

// Sets the 3D listener of the master device (ISetListen): position and front/top orientation, no velocity,
// doppler factor 1. A zero front vector falls back to -1 on y, a zero top z to -1. The position and front are also
// remembered in globals for the software 3D pan of every sound effect. Always returns 1.
int SoundChannel::SetListener(float rolloff, float x, float y, float z, float frontX, float frontY, float frontZ,
                              float topX, float topY, float topZ)
{
    if (listener != 0) {
        DS3DLISTENER params;
        params.dwSize = sizeof(DS3DLISTENER);
        listener->GetAllParameters(&params);
        params.flRolloffFactor = rolloff;
        params.flDopplerFactor = 1.0f;
        params.vPosition.x = x;
        params.vPosition.y = y;
        params.vPosition.z = z;
        params.vVelocity.x = 0;
        params.vVelocity.y = 0;
        params.vVelocity.z = 0;
        float correctedFrontY = (frontX != 0 || frontY != 0 || frontZ != 0) ? frontY : -1.0f;
        params.vOrientFront.x = frontX;
        params.vOrientFront.y = correctedFrontY;
        params.vOrientFront.z = frontZ;
        float correctedTopZ = (topZ == 0) ? -1.0f : topZ;
        params.vOrientTop.x = topX;
        params.vOrientTop.y = topY;
        params.vOrientTop.z = correctedTopZ;
        listener->SetAllParameters(&params, DS3D_IMMEDIATE);
        g_listenerFront[0] = frontX;
        g_listenerPosition[0] = x;
        g_listenerFront[2] = frontZ;
        g_listenerPosition[1] = y;
        g_listenerPosition[2] = z;
        g_listenerFront[1] = correctedFrontY;
    }
    return 1;
}

// Sets the loop range from sample numbers (ISetLoopPos): converts them to byte offsets with the file's bits per
// sample and the buffer's channel count. loopEnd is the data end for the stream reader (+ header). Returns 1.
int SoundChannel::SetLoopPos(int startSamples, int endSamples)
{
    loopStartBytes = fileHeader.bitsPerSample * startSamples / BITS_PER_BYTE * format.wf.nChannels;
    loopEndBytes = fileHeader.bitsPerSample * endSamples / BITS_PER_BYTE * format.wf.nChannels;
    endOffset = loopEndBytes + WAV_HEADER_BYTES;
    return 1;
}

// No-op that returns 1; called by ISetSEVolume after it changes a volume.
int SoundChannel::Commit()
{
    return 1;
}

// Play position of the stream in samples (IGetPos). The DirectSound buffer is a ring of three sections that the
// refill code (UpdateFade) rewrites one at a time; each section that was refilled shifts where the bytes the play
// cursor is on come from in the file. The three sections are tracked in sectionFilled[]. The result is the number
// of samples played since the start. Returns 0 while no buffer is attached.
int SoundChannel::GetPosition()
{
    if (directSound == 0)
        return 0;
    if (primaryBuffer == 0)
        return 0;
    if (buffer == 0)
        return 0;
    if (prepared == 0)
        return 0;
    // One buffer section is format.wf.nAvgBytesPerSec bytes (one second of PCM). The play cursor is in section 0, 1
    // or 2 and base shifts it by the sections already refilled. The member is used directly, not copied to a local:
    // the original reloads it where a local would be cached in a register.
    int base;
    if (playCursor < format.wf.nAvgBytesPerSec && sectionFilled[0] == 0)
        base = playCursor - format.wf.nAvgBytesPerSec * 2;
    if (playCursor < format.wf.nAvgBytesPerSec && sectionFilled[0] == 1)
        base = playCursor - format.wf.nAvgBytesPerSec * 3;
    if (playCursor >= format.wf.nAvgBytesPerSec && playCursor < format.wf.nAvgBytesPerSec * 2 && sectionFilled[1] == 0)
        base = playCursor - format.wf.nAvgBytesPerSec * 3;
    if (playCursor >= format.wf.nAvgBytesPerSec && playCursor < format.wf.nAvgBytesPerSec * 2 && sectionFilled[1] == 1)
        base = playCursor - format.wf.nAvgBytesPerSec * 4;
    if (playCursor >= format.wf.nAvgBytesPerSec * 2 && playCursor < format.wf.nAvgBytesPerSec * 3 && sectionFilled[2] == 0)
        base = playCursor - format.wf.nAvgBytesPerSec * 4;
    if (playCursor >= format.wf.nAvgBytesPerSec * 2 && playCursor < format.wf.nAvgBytesPerSec * 3 && sectionFilled[2] == 1)
        base = playCursor - format.wf.nAvgBytesPerSec * 5;
    unsigned int streamed = filePosition;
    if (streamed < format.wf.nAvgBytesPerSec * 2)
        streamed += endOffset;
    return (int)(streamed + base) / format.wf.nChannels * BITS_PER_BYTE / fileHeader.bitsPerSample;
}

// Stops the buffer and closes the stream if the channel is audible. Called by the API, UpdateFade (fade to
// silence) and ReleaseBuffers. Returns 1 if it stopped something.
int SoundChannel::Stop()
{
    if (directSound != 0 && primaryBuffer != 0 && buffer != 0) {
        if (IsPlaying() != 0) {
            buffer->Stop();
            if (isPlaying != 0)
                Close();
            isPlaying = 0;
            m_01c8 = 0;
            return 1;
        }
    }
    return 0;
}

// Sets the buffer volume. The 0..10000 game volume (clamped) is cubed into DirectSound's 0 .. -10000 hundredths
// of a decibel so that it feels linear. Does nothing unless the channel is audible, or force is set.
// Called by SetVolume, StartFade and Update3DPan.
int SoundChannel::ApplyVolume(int newVolume, int force)
{
    if (directSound == 0)
        return 0;
    if (primaryBuffer == 0)
        return 0;
    if (buffer == 0)
        return 0;
    if (newVolume < 0)
        newVolume = 0;
    else if (newVolume > VOLUME_SCALE_MAX)
        newVolume = VOLUME_SCALE_MAX;
    if (force == 0 && IsPlaying() == 0)
        return 0;
    float level = (float)newVolume;
    level = level / (float)VOLUME_SCALE_MAX;
    level = level * level * level * (float)VOLUME_SCALE_MAX;
    buffer->SetVolume((LONG)-level);
    return 1;
}

// Forgets a running fade: the target becomes the current volume. Called by the API when a volume is set directly.
void SoundChannel::CancelFade()
{
    fadeTarget = volume;
    stopWhenFaded = 0;
    fadeRemainingMs = 0;
}

// Remembers the volume and applies it right away if the channel is audible. Called by the API and UpdateFade.
// Returns 1.
int SoundChannel::SetVolume(int newVolume)
{
    volume = newVolume;
    if (IsPlaying() != 0)
        ApplyVolume(newVolume, 0);
    return 1;
}

// Starts a linear fade of the volume to target over durationMs (0 = jump there now). UpdateFade runs it.
// Called by IVolFade and the play commands. Returns 1.
int SoundChannel::StartFade(int target, int durationMs)
{
    stopWhenFaded = 0;
    fadeTarget = target;
    fadeRemainingMs = durationMs;
    fadeTick = GetTickCount();
    if (durationMs == 0) {
        volume = target;
        if (IsPlaying() != 0)
            ApplyVolume(target, 0);
    }
    return 1;
}

// Locks bytes of the DirectSound buffer at offset (a section boundary) and fills it with the next PCM data from
// the stream. The reserved argument is unused. Past the end of the data the buffer is padded with silence (a
// one-shot sound) or refilled from the loop start (a looping sound); when a one-shot has no data left at all the
// channel is stopped. The buffer may wrap, so DirectSound returns two regions. Called by UpdateFade (section
// refill) and Open (first two sections). Returns 1 when it filled the buffer, 0 when it could not lock it.
int SoundChannel::FillSection(DWORD bytes, int reserved, DWORD offset)
{
    BYTE *region1;
    BYTE *region2;
    DWORD size1;
    DWORD size2;
    buffer->Restore();
    if (buffer->Lock(offset, bytes, (void **)&region1, &size1, (void **)&region2, &size2, 0) != 0)
        return 0;
    if (isLooping == 0) {
        int readPos = Tell();
        if (readPos + size1 <= (DWORD)endOffset) {
            Read(region1, size1);
        } else {
            if (endOffset - readPos != 0)
                Read(region1, endOffset - readPos);
            if (endOffset - readPos <= 0) {
                buffer->Unlock(region1, size1, region2, size2);
                Close();
                buffer->Stop();
                isPlaying = 0;
                return 1;
            }
            memset(region1 + (endOffset - readPos), 0, readPos + size1 - endOffset);
        }
        if (region2 != 0) {
            readPos = Tell();
            if (readPos + size2 <= (DWORD)endOffset) {
                Read(region2, size2);
            } else {
                if (endOffset - readPos != 0)
                    Read(region2, endOffset - readPos);
                memset(region2 + (endOffset - readPos), 0, readPos + size2 - endOffset);
            }
        }
    } else {
        int readPos = Tell();
        if (readPos + size1 <= (DWORD)endOffset) {
            Read(region1, size1);
        } else {
            Read(region1, endOffset - readPos);
            if (loopStartBytes != -1)
                Seek(loopStartBytes + WAV_HEADER_BYTES);
            else
                Seek(WAV_HEADER_BYTES);
            Read(region1 + (endOffset - readPos), readPos + size1 - endOffset);
        }
        if (region2 != 0) {
            readPos = Tell();
            if (readPos + size2 <= (DWORD)endOffset) {
                Read(region2, size2);
            } else {
                Read(region2, endOffset - readPos);
                Seek(WAV_HEADER_BYTES);
                Read(region2 + (endOffset - readPos), readPos + size2 - endOffset);
            }
        }
    }
    buffer->Unlock(region1, size1, region2, size2);
    return 1;
}

// Per-tick update of a running channel (called for every channel by the 10 ms timer): steps the volume fade, then
// refills the section of the three-section buffer that the play cursor just left. Returns 0 when the channel is
// not running, 1 otherwise (also after a fade stopped it).
int SoundChannel::UpdateFade()
{
    if (directSound == 0 || primaryBuffer == 0 || buffer == 0 || isPlaying == 0)
        return 0;
    if (fadeRemainingMs != 0) {
        DWORD now = GetTickCount();
        int elapsed = now - fadeTick;
        int remaining = fadeRemainingMs - elapsed;
        fadeTick = now;
        if (remaining <= 0) {
            fadeRemainingMs = 0;
            if (stopWhenFaded != 0) {
                Stop();
                return 1;
            }
            SetVolume(fadeTarget);
        } else {
            int stepVolume = (fadeTarget - volume) * elapsed / fadeRemainingMs + volume;
            fadeRemainingMs = remaining;
            SetVolume(stepVolume);
        }
    }
    DWORD cursor;
    DWORD writeCursor;
    buffer->GetCurrentPosition(&cursor, &writeCursor);
    playCursor = cursor;
    if (cursor < format.wf.nAvgBytesPerSec && sectionFilled[0] == 0) {
        FillSection(format.wf.nAvgBytesPerSec, 0, format.wf.nAvgBytesPerSec * 2);
        sectionFilled[0] = 1;
        sectionFilled[1] = 0;
        sectionFilled[2] = 1;
    }
    if (cursor >= format.wf.nAvgBytesPerSec && cursor < format.wf.nAvgBytesPerSec * 2 && sectionFilled[1] == 0) {
        FillSection(format.wf.nAvgBytesPerSec, 0, 0);
        sectionFilled[0] = 1;
        sectionFilled[1] = 1;
        sectionFilled[2] = 0;
    }
    if (cursor >= format.wf.nAvgBytesPerSec * 2 && cursor < format.wf.nAvgBytesPerSec * 3 && sectionFilled[2] == 0) {
        FillSection(format.wf.nAvgBytesPerSec, 0, format.wf.nAvgBytesPerSec);
        sectionFilled[0] = 0;
        sectionFilled[1] = 1;
        sectionFilled[2] = 1;
    }
    return 1;
}

// Releases every DirectSound object of the channel (IRelease and the channel table reset): the 3D listener, the
// stream buffer and its 3D interface, the primary buffer and the device, unless another channel owns them.
// Logs the device's reference count to the debugger. Called by IRelease and IInit's failure path. Returns 0.
int SoundChannel::ReleaseBuffers()
{
    char message[DEBUG_MESSAGE_SIZE];
    if (listener != 0)
        listener->Release();
    listener = 0;
    Stop();
    if (prepared != 0)
        FreeBuffers();
    if (sharesPrimaryBuffer != 0)
        primaryBuffer = 0;
    if (buffer != 0)
        buffer->Release();
    buffer = 0;
    if (buffer3d != 0)
        buffer3d->Release();
    buffer3d = 0;
    if (primaryBuffer != 0)
        primaryBuffer->Stop();
    if (primaryBuffer != 0)
        primaryBuffer->Release();
    primaryBuffer = 0;
    if (sharesDirectSound != 0)
        directSound = 0;
    if (directSound != 0) {
        ULONG references = directSound->Release();
        sprintf(message, "%d", references);
        OutputDebugStringA(message);
    }
    directSound = 0;
    return 0;
}

// Software 3D positioning of a sound effect that has no DirectSound 3D buffer: sets its volume from the distance to
// the listener and its pan from the angle between the listener's front direction and the sound. Volume falls
// linearly to silence at SOFT3D_FAR_DISTANCE (scaled by distanceScale). Pan is sin(angle)^2 with the sign of
// sin(angle), scaled to DirectSound's -10000..10000. Called by UpdateSE and SetPosition. Always returns 0.
int SoundChannel::Update3DPan()
{
    if (buffer != 0 && usesSoftware3D != 0) {
        float distance = (float)sqrt(LengthSquared(worldPosition[0] - g_listenerPosition[0],
                                                    worldPosition[1] - g_listenerPosition[1],
                                                    worldPosition[2] - g_listenerPosition[2]));
        float attenuation = (SOFT3D_FAR_DISTANCE - distance * distanceScale) / SOFT3D_VOLUME_DIVISOR;
        float relativeX = worldPosition[0] - g_listenerPosition[0];
        float relativeY = worldPosition[1] - g_listenerPosition[1];
        float soundAngle = (float)atan2(relativeX, relativeY);
        float frontAngle = (float)atan2(g_listenerFront[0], g_listenerFront[1]);
        while (frontAngle > TWO_PI_FLOAT)
            frontAngle -= TWO_PI_FLOAT;
        while (frontAngle < -TWO_PI_FLOAT)
            frontAngle += TWO_PI_FLOAT;
        while (soundAngle > TWO_PI_FLOAT)
            soundAngle -= TWO_PI_FLOAT;
        while (soundAngle < -TWO_PI_FLOAT)
            soundAngle += TWO_PI_FLOAT;
        float pan = soundAngle - frontAngle;
        if (sin(pan) < 0.0)
            pan = (float)-(sin(pan) * sin(pan) * SOFT3D_PAN_GAIN);
        else
            pan = (float)(sin(pan) * sin(pan) * SOFT3D_PAN_GAIN);
        float level = VOLUME_SCALE_MAX - attenuation * SOFT3D_VOLUME_STEP;
        level = 0.0f <= level ? level : 0.0f;
        ApplyVolume((int)level, 1);
        pan = pan * SOFT3D_PAN_SCALE;
        pan = -PAN_FULL > pan ? -PAN_FULL : (PAN_FULL < pan ? PAN_FULL : pan);
        buffer->SetPan((LONG)pan);
    }
    return 0;
}

// Per-tick update of a positioned sound effect (called by the timer for every effect channel): moves the DirectSound
// 3D buffer to the sound's position, pulled towards or away from the listener by distanceScale, with the stored
// velocity, an omnidirectional cone and the effect distance range; then runs the software pan for channels without
// a 3D buffer. Returns 0 while no buffer is attached, 1 otherwise.
int SoundChannel::UpdateSE()
{
    if (buffer == 0)
        return 0;
    if (buffer3d != 0) {
        DS3DBUFFER params;
        params.dwSize = sizeof(DS3DBUFFER);
        buffer3d->GetAllParameters(&params);
        params.dwInsideConeAngle = CONE_ANGLE_OMNI;
        params.dwOutsideConeAngle = CONE_ANGLE_OMNI;
        params.dwMode = DS3DMODE_NORMAL;
        params.flMinDistance = SE_MIN_DISTANCE;
        params.flMaxDistance = SE_MAX_DISTANCE;
        params.vVelocity.x = velocity[0];
        params.vVelocity.y = velocity[1];
        params.vVelocity.z = velocity[2];
        params.vPosition.x = (worldPosition[0] - g_listenerPosition[0]) * distanceScale + g_listenerPosition[0];
        params.vPosition.y = (worldPosition[1] - g_listenerPosition[1]) * distanceScale + g_listenerPosition[1];
        params.vPosition.z = (worldPosition[2] - g_listenerPosition[2]) * distanceScale + g_listenerPosition[2];
        buffer3d->SetAllParameters(&params, DS3D_IMMEDIATE);
    }
    Update3DPan();
    return 1;
}

// Moves a sound effect (ISetSEPos): stores the position (and, for a DirectSound 3D buffer, the velocity; a null
// vx means no velocity) and pushes it to the 3D buffer, or runs the software pan when the channel has no 3D buffer.
// Called by ISetSEPos. Always returns 1.
int SoundChannel::SetPosition(float *x, float *y, float *z, float *vx, float *vy, float *vz)
{
    if (buffer3d != 0) {
        DS3DBUFFER params;
        params.dwSize = sizeof(DS3DBUFFER);
        buffer3d->GetAllParameters(&params);
        params.dwInsideConeAngle = CONE_ANGLE_OMNI;
        params.dwOutsideConeAngle = CONE_ANGLE_OMNI;
        params.dwMode = DS3DMODE_NORMAL;
        params.flMinDistance = SE_MIN_DISTANCE;
        params.flMaxDistance = SE_MAX_DISTANCE;
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
        buffer3d->SetAllParameters(&params, DS3D_IMMEDIATE);
        return 1;
    }
    if (usesSoftware3D != 0) {
        worldPosition[0] = *x;
        worldPosition[1] = *y;
        worldPosition[2] = *z;
        Update3DPan();
    }
    return 1;
}

// Starts a file on the channel (the play commands): opens it, reads its 0x2c-byte WAV header, creates the
// three-second DirectSound buffer for its format, positions the stream at startSamples, primes two sections and
// starts the buffer looping. loopStartSamples/loopEndSamples bound the loop (the end is clamped to the data size);
// loop selects whether the stream wraps. Returns 1 when playing, 0 on failure (no device, nothing to play, file
// error, buffer creation error). Called by the play commands.
int SoundChannel::Open(const char *file, int loop, int startSamples, int loopStartSamples,
                       unsigned int loopEndSamples)
{
    IDirectSoundBuffer *created;
    if (directSound == 0)
        return 0;
    if (primaryBuffer == 0)
        return 0;
    if (IsPlaying() != 0)
        Stop();
    if (loopEndSamples <= 0)
        return 0;
    if (prepared == 0)
        PrepareBuffers(0);
    if (WaveStream::Open(file) == 0)
        return 0;
    if (Read((BYTE *)&fileHeader, WAV_HEADER_BYTES) == 0) {
        Close();
        return 0;
    }
    int stereo = 1 < fileHeader.channels;
    memset(&format, 0, sizeof(format));
    format.wf.nChannels = (stereo != 0) + 1;
    format.wf.nBlockAlign = fileHeader.blockAlign;
    format.wf.nSamplesPerSec = fileHeader.samplesPerSecond;
    format.wBitsPerSample = fileHeader.bitsPerSample;
    format.wf.wFormatTag = WAVE_FORMAT_PCM;
    format.wf.nAvgBytesPerSec = fileHeader.blockAlign * fileHeader.samplesPerSecond;
    DSBUFFERDESC description;
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(DSBUFFERDESC);
    if (g_memoryModeA == 2)
        description.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_LOCHARDWARE;
    else
        description.dwFlags = (g_memoryModeA == 1) * DSBCAPS_LOCSOFTWARE + (DSBCAPS_GETCURRENTPOSITION2 |
                                                                            DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN);
    description.dwBufferBytes = format.wf.nAvgBytesPerSec * CHANNEL_SECTION_COUNT;
    description.lpwfxFormat = (WAVEFORMATEX *)&format;
    if (buffer == 0) {
        directSound->CreateSoundBuffer(&description, &created, 0);
        if (created == 0)
            return 0;
        created->QueryInterface(IID_IDirectSoundBuffer8, (void **)&buffer);
        created->Release();
        if (buffer == 0)
            return 0;
    }
    int startBytes = fileHeader.bitsPerSample * startSamples / BITS_PER_BYTE * format.wf.nChannels;
    loopStartBytes = fileHeader.bitsPerSample * loopStartSamples / BITS_PER_BYTE * format.wf.nChannels;
    loopEndBytes = fileHeader.bitsPerSample * (int)loopEndSamples / BITS_PER_BYTE * format.wf.nChannels;
    if (loopEndBytes > dataSize)
        loopEndBytes = dataSize;
    endOffset = loopEndBytes + WAV_HEADER_BYTES;
    if (endOffset == WAV_HEADER_BYTES)
        *(char *)0 = 1;
    Seek(startBytes + WAV_HEADER_BYTES);
    isLooping = loop;
    buffer->SetCurrentPosition(0);
    FillSection(format.wf.nAvgBytesPerSec * 2, 0, 0);
    startTick = GetTickCount();
    sectionFilled[0] = 0;
    sectionFilled[1] = 1;
    sectionFilled[2] = 1;
    isPlaying = 1;
    buffer->Restore();
    primaryBuffer->Restore();
    buffer->Play(0, 0, DSBPLAY_LOOPING);
    return 1;
}
