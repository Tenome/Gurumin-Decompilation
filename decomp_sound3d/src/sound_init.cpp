// sound_init.cpp : channel initialisation (DirectSound device and 3D listener), the tracked allocator that the
// engine's helper code allocates through, and the file checksum helpers used by the IPlay integrity check.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "sound_engine.h"

// ---- SoundChannel::Init -----------------------------------------------------------------------------------------

// Float constants are macros so that cl emits immediates (a `const float` object is loaded from memory).
#define LISTENER_ROLLOFF 0.2f
#define LISTENER_DOPPLER 1.0f

// SoundChannel::Init: releases what the channel holds, then binds a DirectSound device and primary buffer. With no
// sharedDevice a new device is created for the window (exclusive level) and a primary buffer with a 3D listener is
// made (44.1 kHz, 16 bit stereo, looping, listener front (0,-1,0), top (0,0,1), rolloff 0.2). With a shared device
// or primary buffer the channel only keeps the pointers. Called by IInit for the master object and then for every
// music and effect channel with the master's pointers. Returns 1 on success, 0 on a DirectSound failure.
int SoundChannel::Init(HWND window, IDirectSound8 *sharedDevice, IDirectSoundBuffer *sharedPrimary)
{
    ReleaseBuffers();
    IDirectSound8 **device = &directSound;
    if (sharedDevice == 0) {
        if (DirectSoundCreate8(0, device, 0) != 0)
            return 0;
        if ((*device)->SetCooperativeLevel(window, DSSCL_EXCLUSIVE) != 0) {
            (*device)->Release();
            return 0;
        }
    } else {
        *device = sharedDevice;
        sharesDirectSound = 1;
    }
    if (sharedPrimary == 0) {
        DSBUFFERDESC desc;
        memset(&desc, 0, sizeof(desc));
        IDirectSoundBuffer **primary = &primaryBuffer;
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;
        if ((*device)->CreateSoundBuffer(&desc, primary, 0) != 0) {
            (*device)->Release();
            return 0;
        }
        WAVEFORMATEX format;
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.wBitsPerSample = PCM_OUTPUT_BITS;
        format.nChannels = PCM_OUTPUT_CHANNELS;
        format.nBlockAlign = PCM_OUTPUT_BLOCK_ALIGN;
        format.nSamplesPerSec = PCM_OUTPUT_RATE;
        format.nAvgBytesPerSec = PCM_OUTPUT_BYTES_PER_SECOND;
        format.cbSize = 0;
        (*primary)->SetFormat(&format);
        (*primary)->GetFormat(&format, sizeof(format), 0);
        IDirectSound3DListener **listenerSlot = &listener;
        (*primary)->QueryInterface(IID_IDirectSound3DListener, (void **)listenerSlot);
        if (*listenerSlot != 0) {
            DS3DLISTENER params;
            params.dwSize = sizeof(params);
            (*listenerSlot)->GetAllParameters(&params);
            params.flRolloffFactor = LISTENER_ROLLOFF;
            params.flDopplerFactor = LISTENER_DOPPLER;
            params.vPosition.x = 0;
            params.vPosition.y = 0;
            params.vPosition.z = 0;
            params.vVelocity.x = 0;
            params.vVelocity.y = 0;
            params.vVelocity.z = 0;
            params.vOrientFront.x = 0;
            params.vOrientFront.y = -1.0f;
            params.vOrientFront.z = 0;
            params.vOrientTop.x = 0;
            params.vOrientTop.y = 0;
            params.vOrientTop.z = 1.0f;
            (*listenerSlot)->SetAllParameters(&params, DS3D_IMMEDIATE);
        }
        (*primary)->Restore();
        (*primary)->Play(0, 0, DSBPLAY_LOOPING);
    } else {
        primaryBuffer = sharedPrimary;
        sharesPrimaryBuffer = 1;
    }
    return 1;
}

// ---- tracked allocator --------------------------------------------------------------------------------------------
//
// A table of 4096 block pointers with a 16 byte label each; the engine's helper code (and the libraries linked into
// the DLL) allocate through AllocateNamed and release through TrackedFree, so a leak report can be written.

// AllocatorNoOp: an empty hook slot (never called by this DLL's exports).
void AllocatorNoOp(void)
{
}

// AllocateNamed: operator new for up to 100000000 bytes; remembers the block under the first free table slot with
// the first 15 characters of label. Returns null for 0 or more than 100000000 bytes. The last slot is reused when
// the table is full.
void *AllocateNamed(unsigned size, const char *label)
{
    if (size > ALLOC_MAX_BYTES)
        return 0;
    if (size == 0)
        return 0;
    int slot;
    for (slot = 0; slot < ALLOC_TABLE_SIZE; slot++) {
        if (g_allocTable[slot] == 0)
            break;
    }
    if (slot == ALLOC_TABLE_SIZE)
        slot = ALLOC_TABLE_SIZE - 1;
    strncpy(g_allocNames[slot], label, ALLOC_NAME_SIZE - 1);
    void *block = operator new(size);
    g_allocTable[slot] = block;
    return block;
}

// AllocatorReturnZero: second empty hook slot, returns 0.
int AllocatorReturnZero(void)
{
    return 0;
}

// DumpTrackedAllocations: writes every live block as "<slot> <label> <address>" to a text file; nothing is written
// when the table is empty.
void DumpTrackedAllocations(const char *path)
{
    for (int first = 0; first < ALLOC_TABLE_SIZE; first++) {
        if (g_allocTable[first] != 0) {
            FILE *file = fopen(path, "wt");
            if (file == 0)
                return;
            for (int slot = 0; slot < ALLOC_TABLE_SIZE; slot++) {
                if (g_allocTable[slot] != 0) {
                    fprintf(file, "%05d %s %x", slot, g_allocNames[slot], g_allocTable[slot]);
                    fputs("\n", file);
                }
            }
            fclose(file);
            return;
        }
    }
}

// AllocatorReturnArgument: third hook slot, returns its argument.
int AllocatorReturnArgument(int value)
{
    return value;
}

// InitTrackedAllocator: clears the table and the labels and installs the hook pointers. Called by IInit, which ignores
// the result (always 1). The original also carries a /GS cookie here although the function has no visible buffer.
int InitTrackedAllocator(void)
{
    for (int slot = 0; slot < ALLOC_TABLE_SIZE; slot++)
        g_allocTable[slot] = 0;
    for (int slot = 0; slot < ALLOC_TABLE_SIZE; slot++)
        g_allocNames[slot][0] = 0;
    strcpy(g_allocUnknownLabel, "Unknown");
    for (int i = 0; i < 5; i++)
        g_allocCounters[i] = 0;
    g_allocFlag = 0;
    g_memcopyHook = memcpy;
    g_allocateHook = AllocateNamed;
    g_freeHook = TrackedFree;
    return 1;
}

// TrackedFree: forgets the block in the table (if present) and frees it.
void TrackedFree(void *block)
{
    int slot;
    for (slot = 0; slot < ALLOC_TABLE_SIZE; slot++) {
        if (g_allocTable[slot] == block)
            break;
    }
    if (slot != ALLOC_TABLE_SIZE)
        g_allocTable[slot] = 0;
    free(block);
}

// ---- file checksums (IPlay integrity check) ------------------------------------------------------------------------

const unsigned CHECKSUM_XOR_SEED = 0xcc82;

// ComputeFileChecksum: reads a file as little-endian 32 bit words and returns the sum and the xor (seeded with
// 0xcc82) of all of them. A short last word keeps the stale high bytes of the previous word. Returns 0 when the file
// cannot be opened.
int ComputeFileChecksum(const char *path, int *sumOut, unsigned *xorOut)
{
    FILE *file = fopen(path, "rb");
    if (file == 0)
        return 0;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    int sum = 0;
    unsigned mask = CHECKSUM_XOR_SEED;
    unsigned word;
    if (size > 0) {
        int count = ((size - 1U) >> 2) + 1;
        do {
            fread(&word, 1, 4, file);
            sum += word;
            mask ^= word;
            count--;
        } while (count != 0);
    }
    fclose(file);
    *sumOut = sum;
    *xorOut = mask;
    return 1;
}

// ReadFileWords: reads the two 32 bit words stored at offset + index * 8 of a file. Returns 0 when the file cannot be
// opened.
int ReadFileWords(const char *path, int offset, int index, void *first, void *second)
{
    FILE *file = fopen(path, "rb");
    if (file == 0)
        return 0;
    fseek(file, offset + index * 8, SEEK_SET);
    fread(first, 1, 4, file);
    fread(second, 1, 4, file);
    fclose(file);
    return 1;
}
