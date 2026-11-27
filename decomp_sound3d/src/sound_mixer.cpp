// sound_mixer.cpp : the Windows mixer part of sound3d.dll (MixOpen, MixClose, GetParam, SetParam).
//
// SoundArc exports these through its API table; the game uses them to read and set the system volume of the
// auxiliary, CD, telephone and master lines. Control ids are looked up once by MixOpen; GetParam caches the values it
// has read (g_paramValue / g_paramCached, 64 slots per line).
#include <stdlib.h>
#include <string.h>
#include "sound_engine.h"

const DWORD LINE_STRUCT_SIZE = 0xa8;                 // sizeof(MIXERLINEA)
const DWORD CONTROLS_STRUCT_SIZE = 0x18;             // sizeof(MIXERLINECONTROLSA)
const DWORD CONTROL_STRUCT_SIZE = 0x94;              // sizeof(MIXERCONTROLA)
const DWORD CONTROL_TYPE_VOLUME = 0x50030001;
const DWORD CONTROL_TYPE_PAN = 0x40020001;
const DWORD CONTROL_TYPE_PEAKMETER = 0x10020001;
const DWORD LINE_AUXILIARY = 0x1008;
const DWORD LINE_COMPACTDISC = 0x1004;
const DWORD LINE_TELEPHONE = 0x1005;
const int PARAM_INDEX_VOLUME = 5;                    // GetParam/SetParam index of the volume control
const int PARAM_INDEX_PEAK = 6;                      // index of the master peak meter
const int PARAM_INDEX_SPARE = 4;                     // index of the "spare" control of a line
const int KIND_AUX = 0;
const int KIND_CD = 1;
const int KIND_TELEPHONE = 2;
const int KIND_MASTER = 3;

HMIXER MixerControls::handle;
DWORD MixerControls::peakMeter;
DWORD MixerControls::masterVolume;
DWORD MixerControls::masterSpare;
DWORD MixerControls::masterPan;
DWORD MixerControls::auxVolume;
DWORD MixerControls::auxPan;
DWORD MixerControls::auxSpare;
DWORD MixerControls::phoneVolume;
DWORD MixerControls::phonePan;
DWORD MixerControls::phoneSpare;
DWORD MixerControls::cdVolume;
DWORD MixerControls::cdPan;
DWORD MixerControls::cdPanAgain;

// MixerControls::MixerControls: marks every control as missing (-1) and the mixer as not open. Run by the static
// initialisation of the DLL.
MixerControls::MixerControls()
{
    handle = 0;
    peakMeter = MIXER_NO_CONTROL;
    masterVolume = MIXER_NO_CONTROL;
    masterSpare = MIXER_NO_CONTROL;
    masterPan = MIXER_NO_CONTROL;
    auxVolume = MIXER_NO_CONTROL;
    auxPan = MIXER_NO_CONTROL;
    auxSpare = MIXER_NO_CONTROL;
    phoneVolume = MIXER_NO_CONTROL;
    phonePan = MIXER_NO_CONTROL;
    phoneSpare = MIXER_NO_CONTROL;
    cdVolume = MIXER_NO_CONTROL;
    cdPan = MIXER_NO_CONTROL;
    cdPanAgain = MIXER_NO_CONTROL;
}

// Asks the open mixer for one control of the line in g_mixerLine; returns its control id or -1.
static __forceinline DWORD QueryControlId(DWORD controlType)
{
    memset(&g_lineControls, 0, sizeof(g_lineControls));
    g_lineControls.dwLineID = g_mixerLine.dwLineID;
    memset(&g_mixerControl, 0, sizeof(g_mixerControl));
    g_lineControls.cbStruct = CONTROLS_STRUCT_SIZE;
    g_lineControls.dwControlType = controlType;
    g_lineControls.cControls = 1;
    g_lineControls.cbmxctrl = CONTROL_STRUCT_SIZE;
    g_lineControls.pamxctrl = &g_mixerControl;
    g_mixerControl.cbStruct = CONTROL_STRUCT_SIZE;
    if (mixerGetLineControls((HMIXEROBJ)MixerControls::handle, &g_lineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
        return g_mixerControl.dwControlID;
    return MIXER_NO_CONTROL;
}

// Fills g_mixerLine with the mixer line of a component type.
static __forceinline void SelectLine(DWORD componentType)
{
    memset(&g_mixerLine, 0, sizeof(g_mixerLine));
    g_mixerLine.cbStruct = LINE_STRUCT_SIZE;
    g_mixerLine.dwComponentType = componentType;
    g_mixerLine.dwDestination = 0;
    g_mixerLine.dwSource = 0;
    mixerGetLineInfo((HMIXEROBJ)MixerControls::handle, &g_mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
}

// MixOpen: opens the default mixer and looks up the volume, pan and peak meter controls of the auxiliary, CD and
// telephone lines and of the first destination (master) line. Clears the GetParam cache. Always returns 1.
extern "C" int MixOpen(void)
{
    mixerOpen(&MixerControls::handle, 0, 0, 0, 0);
    SelectLine(LINE_AUXILIARY);
    MixerControls::auxVolume = QueryControlId(CONTROL_TYPE_VOLUME);
    MixerControls::auxPan = QueryControlId(CONTROL_TYPE_PAN);
    MixerControls::auxSpare = MIXER_NO_CONTROL;
    MixerControls::peakMeter = QueryControlId(CONTROL_TYPE_PEAKMETER);
    SelectLine(LINE_COMPACTDISC);
    MixerControls::cdVolume = QueryControlId(CONTROL_TYPE_VOLUME);
    MixerControls::cdPan = QueryControlId(CONTROL_TYPE_PAN);
    MixerControls::cdPanAgain = QueryControlId(CONTROL_TYPE_PAN);
    SelectLine(LINE_TELEPHONE);
    MixerControls::phoneVolume = QueryControlId(CONTROL_TYPE_VOLUME);
    MixerControls::phonePan = QueryControlId(CONTROL_TYPE_PAN);
    MixerControls::phoneSpare = MIXER_NO_CONTROL;
    memset(&g_mixerLine, 0, sizeof(g_mixerLine));
    g_mixerLine.cbStruct = LINE_STRUCT_SIZE;
    g_mixerLine.dwDestination = 0;
    g_mixerLine.dwSource = 0;
    mixerGetLineInfo((HMIXEROBJ)MixerControls::handle, &g_mixerLine, MIXER_GETLINEINFOF_DESTINATION);
    if (MixerControls::peakMeter == 0)
        MixerControls::peakMeter = QueryControlId(CONTROL_TYPE_PEAKMETER);
    MixerControls::masterPan = QueryControlId(CONTROL_TYPE_PAN);
    MixerControls::masterSpare = MIXER_NO_CONTROL;
    MixerControls::masterVolume = QueryControlId(CONTROL_TYPE_VOLUME);
    memset(g_paramValue, 0, sizeof(g_paramValue));
    memset(g_paramCached, 0, sizeof(g_paramCached));
    return 1;
}

// MixClose: closes the mixer. Returns 0.
extern "C" int MixClose(void)
{
    if (MixerControls::handle != 0) {
        mixerClose(MixerControls::handle);
        MixerControls::handle = 0;
    }
    return 0;
}

// Returns the control id GetParam/SetParam use for a line and index, or -1 when there is none.
// kind 0 auxiliary, 1 CD, 2 telephone, 3 master; index 5 = volume, 6 = master peak meter, 4 = spare control.
// Cases are listed in the order the original compiled them.
// GetParam: current volume of a mixer line. The value is read once per (kind, index), cached, and returned as a
// positive number. Returns 0 when the mixer is not open, the control does not exist or the call fails.
extern "C" int GetParam(int kind, int index)
{
    int slot = kind * MIXER_PARAM_STRIDE + index;
    if (g_paramCached[slot] != 0)
        return g_paramValue[slot];
    int value;
    g_paramCached[slot] = 1;
    if (MixerControls::handle == 0)
        return 0;
    MIXERCAPS caps;
    memset(&caps, 0, sizeof(caps));
    MMRESULT result = mixerGetDevCaps((UINT)MixerControls::handle, &caps, sizeof(caps));
    if (result == MMSYSERR_INVALHANDLE) {
        if (mixerOpen(&MixerControls::handle, 0, 0, 0, 0) == MMSYSERR_NOERROR)
            return g_paramValue[slot];
        return 0;
    }
    if (result != MMSYSERR_NOERROR)
        return 0;
    MIXERCONTROLDETAILS details;
    memset(&details, 0, sizeof(details));
    details.dwControlID = MIXER_NO_CONTROL;
    DWORD controlId;
    switch (kind) {
    case KIND_TELEPHONE:
        if (index != PARAM_INDEX_VOLUME)
            return 0;
        controlId = MixerControls::phoneVolume;
        break;
    case KIND_CD:
        if (index != PARAM_INDEX_VOLUME)
            return 0;
        controlId = MixerControls::cdVolume;
        break;
    case KIND_AUX:
        if (index != PARAM_INDEX_VOLUME)
            return 0;
        controlId = MixerControls::auxVolume;
        break;
    case KIND_MASTER:
        if (index == PARAM_INDEX_VOLUME)
            controlId = MixerControls::masterVolume;
        else if (index == PARAM_INDEX_PEAK)
            controlId = MixerControls::peakMeter;
        else
            return 0;
        break;
    default:
        return 0;
    }
    details.dwControlID = controlId;
    if (controlId == MIXER_NO_CONTROL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    details.cbStruct = sizeof(details);
    details.cbDetails = sizeof(int);
    details.paDetails = &value;
    details.cChannels = 1;
    if (mixerGetControlDetails((HMIXEROBJ)MixerControls::handle, &details, MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR) {
        if (value < 0)
            value = -value;
    } else {
        value = 0;
    }
    LeaveCriticalSection(&g_audioLock);
    g_paramValue[slot] = value;
    return value;
}

// SetParam: sets the volume (or spare control) of a mixer line and updates the GetParam cache. Returns 1 when the
// value was sent, 0 when it equals the cached value, the mixer is not open or there is no such control.
extern "C" int SetParam(int kind, int index, int value)
{
    int *cached = &g_paramValue[kind * MIXER_PARAM_STRIDE + index];
    if (*cached == value || MixerControls::handle == 0)
        return 0;
    MIXERCAPS caps;
    memset(&caps, 0, sizeof(caps));
    MMRESULT result = mixerGetDevCaps((UINT)MixerControls::handle, &caps, sizeof(caps));
    if (result == MMSYSERR_INVALHANDLE) {
        mixerOpen(&MixerControls::handle, 0, 0, 0, 0);
        return 0;
    }
    if (result != MMSYSERR_NOERROR)
        return 0;
    MIXERCONTROLDETAILS details;
    memset(&details, 0, sizeof(details));
    details.dwControlID = MIXER_NO_CONTROL;
    DWORD controlId;
    switch (kind) {
    case KIND_TELEPHONE:
        switch (index) {
        case PARAM_INDEX_SPARE:
            controlId = MixerControls::phoneSpare;
            break;
        case PARAM_INDEX_VOLUME:
            controlId = MixerControls::phoneVolume;
            break;
        default:
            return 0;
        }
        break;
    case KIND_CD:
        switch (index) {
        case PARAM_INDEX_SPARE:
            controlId = MixerControls::cdPanAgain;
            break;
        case PARAM_INDEX_VOLUME:
            controlId = MixerControls::cdVolume;
            break;
        default:
            return 0;
        }
        break;
    case KIND_AUX:
        switch (index) {
        case PARAM_INDEX_SPARE:
            controlId = MixerControls::auxSpare;
            break;
        case PARAM_INDEX_VOLUME:
            controlId = MixerControls::auxVolume;
            break;
        default:
            return 0;
        }
        break;
    case KIND_MASTER:
        switch (index) {
        case PARAM_INDEX_SPARE:
            controlId = MixerControls::masterSpare;
            break;
        case PARAM_INDEX_VOLUME:
            controlId = MixerControls::masterVolume;
            break;
        case PARAM_INDEX_PEAK:
            controlId = MixerControls::peakMeter;
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    details.dwControlID = controlId;
    if (controlId == MIXER_NO_CONTROL)
        return 0;
    EnterCriticalSection(&g_audioLock);
    details.cbStruct = sizeof(details);
    details.cbDetails = sizeof(int);
    details.paDetails = &value;
    details.cChannels = 1;
    mixerSetControlDetails((HMIXEROBJ)MixerControls::handle, &details, MIXER_SETCONTROLDETAILSF_VALUE);
    LeaveCriticalSection(&g_audioLock);
    *cached = value;
    return 1;
}

// CloseMixerHandle: closes the mixer if it is open (void twin of MixClose).
void CloseMixerHandle(void)
{
    if (MixerControls::handle != 0) {
        mixerClose(MixerControls::handle);
        MixerControls::handle = 0;
    }
}

// ClearWaveCache: forgets all cached wave buffers without freeing them.
void ClearWaveCache(void)
{
    for (int i = 0; i < 0x30; i++)
        g_waveCache[i] = 0;
}

// FreeWaveCache: frees every cached wave buffer. Returns 1.
int FreeWaveCache(void)
{
    for (int i = 0; i < 0x30; i++) {
        if (g_waveCache[i] != 0) {
            free(g_waveCache[i]);
            g_waveCache[i] = 0;
        }
    }
    return 1;
}
