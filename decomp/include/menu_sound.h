// Events the menu windows report to the sound callback (Game::m_7660, set to FUN_1003b160 at start-up).
// This is the one declaration of them: include this header, never redeclare them.
#ifndef GURUMIN_MENU_SOUND_H
#define GURUMIN_MENU_SOUND_H

enum MenuSoundEvent
{
    MENU_SOUND_WINDOW_OPENED = 0,    // OpenWindow: the callback stays silent
    MENU_SOUND_CURSOR_MOVED = 1,     // the cursor moved to another entry of a window
    MENU_SOUND_WINDOW_CLOSED = 2     // CloseWindow: the callback stays silent
};

// 0x1003b160: the sound callback of the menu windows (sound_effects.cpp).
extern "C" int FUN_1003b160(int event);

#endif
