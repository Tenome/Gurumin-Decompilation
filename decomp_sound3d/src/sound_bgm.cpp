// sound_bgm.cpp : music channel operations of SoundChannel (fade out, start a stream with or without fade-in).
//
// The exported IFadeOut / IFadeOutEx / IPlayB / IPlayFadeB / IPlayFadeBEx (sound_api.cpp) call these under the music
// lock; ProcessCommandQueue calls the Play variants for queued commands.
#include "sound_engine.h"

// SoundChannel::FadeOut: starts a one second fade of this music channel (called by IFadeOut).
int SoundChannel::FadeOut()
{
    stopWhenFaded = 0;
    fadeTarget = VOLUME_SCALE_MAX;
    fadeRemainingMs = FADE_DEFAULT_MS;
    fadeTick = GetTickCount();
    stopWhenFaded = 1;
    return 1;
}

// SoundChannel::FadeOutEx: starts a fade of durationMs (clamped to 1..60000) of this music channel (IFadeOutEx).
int SoundChannel::FadeOutEx(int durationMs)
{
    if (durationMs < FADE_MIN_MS)
        durationMs = FADE_MIN_MS;
    else if (durationMs > FADE_MAX_MS)
        durationMs = FADE_MAX_MS;
    StartFade(VOLUME_SCALE_MAX, durationMs);
    stopWhenFaded = 1;
    return 1;
}

// SoundChannel::PlayB: silences the channel, opens and starts a new music file at full level at once.
// Called by IPlayB and by ProcessCommandQueue for queued IPlay commands. Returns Open's result.
int SoundChannel::PlayB(const char *file, int loop, int startSamples, int loopStartSamples, int loopEndSamples)
{
    fadeTarget = volume;
    stopWhenFaded = 0;
    fadeRemainingMs = 0;
    volume = VOLUME_SCALE_MAX;
    if (IsPlaying() && directSound != 0 && primaryBuffer != 0 && buffer != 0) {
        if (IsPlaying())
            buffer->SetVolume(VOLUME_SILENT);
    }
    int result = Open(file, loop, startSamples, loopStartSamples, loopEndSamples);
    int restoredVolume = g_bgmVolumeCopy;
    stopWhenFaded = 0;
    fadeTarget = restoredVolume;
    fadeRemainingMs = 0;
    fadeTick = GetTickCount();
    volume = restoredVolume;
    if (IsPlaying())
        ApplyVolume(restoredVolume, 0);
    return result;
}

// SoundChannel::PlayFadeB: like PlayB but leaves a one second fade-in pending (IPlayFadeB, queued IPlayFade).
int SoundChannel::PlayFadeB(const char *file, int loop, int startSamples, int loopStartSamples, int loopEndSamples)
{
    fadeTarget = volume;
    stopWhenFaded = 0;
    fadeRemainingMs = 0;
    volume = VOLUME_SCALE_MAX;
    if (IsPlaying() && directSound != 0 && primaryBuffer != 0 && buffer != 0) {
        if (IsPlaying())
            buffer->SetVolume(VOLUME_SILENT);
    }
    int result = Open(file, loop, startSamples, loopStartSamples, loopEndSamples);
    int restoredVolume = g_bgmVolumeCopy;
    stopWhenFaded = 0;
    fadeTarget = restoredVolume;
    fadeRemainingMs = FADE_DEFAULT_MS;
    fadeTick = GetTickCount();
    return result;
}

// SoundChannel::PlayFadeBEx: like PlayB but starts a fade-in over fadeMs (IPlayFadeBEx).
int SoundChannel::PlayFadeBEx(const char *file, int loop, int startSamples, int loopStartSamples, int loopEndSamples,
                              int fadeMs)
{
    fadeTarget = volume;
    stopWhenFaded = 0;
    fadeRemainingMs = 0;
    volume = VOLUME_SCALE_MAX;
    if (IsPlaying() && directSound != 0 && primaryBuffer != 0 && buffer != 0) {
        if (IsPlaying())
            buffer->SetVolume(VOLUME_SILENT);
    }
    int result = Open(file, loop, startSamples, loopStartSamples, loopEndSamples);
    StartFade(g_bgmVolumeCopy, fadeMs);
    return result;
}
