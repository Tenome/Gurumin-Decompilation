// sound_window_holder.cpp : the two tiny methods at 0x10009400 and 0x10009410 that nothing in sound3d.dll calls.
//
// Both sit between the exported API functions. They take no data of their own (the object is empty): the first
// clears the engine's window handle and returns its object, the second does nothing. They look like the constructor
// and destructor of a small helper object whose uses the optimiser removed; the names below describe what the bytes
// do, not a known original class.
#include "sound_engine.h"
#include "window_holder.h"


// Forgets the game window the engine was given; returns the holder itself.
WindowHolder *WindowHolder::ClearWindow()
{
    g_window = 0;
    return this;
}

// Nothing to release.
void WindowHolder::Release()
{
}
