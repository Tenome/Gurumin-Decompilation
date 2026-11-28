// DirectInput 8 (this compiler's SDK has no dinput.h): the interfaces and structures the game uses, with their slots
// in the real API's order, and the game's Input object (input_poll.cpp creates and polls the devices, input_font.cpp
// shuts them down and drives force feedback). Unused slots keep the API's names with empty parameter lists.
// This is the one declaration of these types: include this header, never redeclare them.
#ifndef GURUMIN_DINPUT_MIN_H
#define GURUMIN_DINPUT_MIN_H

#include <windows.h>

// IDirectInputEffect (the joystick's force-feedback effect).
struct InputEffect
{
    virtual long __stdcall QueryInterface() = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall Initialize() = 0;
    virtual long __stdcall GetEffectGuid() = 0;
    virtual long __stdcall GetParameters() = 0;
    virtual long __stdcall SetParameters(void *effect, unsigned long flags) = 0;
    virtual long __stdcall Start(unsigned long iterations, unsigned long flags) = 0;
};


// IDirectInputDevice8A (the slots used here).
struct InputDevice
{
    virtual long __stdcall QueryInterface() = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall GetCapabilities() = 0;
    virtual long __stdcall EnumObjects(int (__stdcall *callback)(const void *, void *), void *context,
                                       unsigned long flags) = 0;
    virtual long __stdcall GetProperty() = 0;
    virtual long __stdcall SetProperty(const GUID &property, const void *header) = 0;
    virtual long __stdcall Acquire() = 0;
    virtual long __stdcall Unacquire() = 0;
    virtual long __stdcall GetDeviceState(unsigned long size, void *state) = 0;
    virtual long __stdcall GetDeviceData() = 0;
    virtual long __stdcall SetDataFormat(const void *format) = 0;
    virtual long __stdcall SetEventNotification() = 0;
    virtual long __stdcall SetCooperativeLevel(HWND window, unsigned long flags) = 0;
    virtual long __stdcall GetObjectInfo() = 0;
    virtual long __stdcall GetDeviceInfo() = 0;
    virtual long __stdcall RunControlPanel() = 0;
    virtual long __stdcall Initialize() = 0;
    virtual long __stdcall CreateEffect(const GUID &effect, const void *description, InputEffect **created,
                                        IUnknown *outer) = 0;
    virtual long __stdcall EnumEffects(int (__stdcall *callback)(const void *, void *), void *context,
                                       unsigned long type) = 0;
    virtual long __stdcall GetEffectInfo() = 0;
    virtual long __stdcall GetForceFeedbackState() = 0;
    virtual long __stdcall SendForceFeedbackCommand() = 0;
    virtual long __stdcall EnumCreatedEffectObjects() = 0;
    virtual long __stdcall Escape() = 0;
    virtual long __stdcall Poll() = 0;
};

// IDirectInput8A (the slots used here).
struct DirectInput
{
    virtual long __stdcall QueryInterface() = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall CreateDevice(const GUID &device, InputDevice **created, IUnknown *outer) = 0;
    virtual long __stdcall EnumDevices(unsigned long type, int (__stdcall *callback)(const void *, void *),
                                       void *context, unsigned long flags) = 0;
};

// DIEFFECT.
struct EffectDesc
{
    unsigned long size;
    unsigned long flags;
    unsigned long duration;          // microseconds
    unsigned long samplePeriod;
    unsigned long gain;
    unsigned long triggerButton;
    unsigned long triggerRepeatInterval;
    unsigned long axisCount;
    unsigned long *axes;
    long *directions;
    void *envelope;
    unsigned long typeSpecificSize;
    void *typeSpecific;
    unsigned long startDelay;
};

// DIPERIODIC.
struct PeriodicForce
{
    unsigned long magnitude;
    long offset;
    unsigned long phase;
    unsigned long period;            // microseconds
};

// DIPROPDWORD.
struct PropertyDword
{
    unsigned long size;
    unsigned long headerSize;
    unsigned long object;
    unsigned long how;
    unsigned long data;
};

// DIDEVICEINSTANCEA (the start; the rest is names and usage).
struct DeviceInstance
{
    unsigned long size;
    GUID instanceGuid;
    GUID productGuid;
};

// DIDEVICEOBJECTINSTANCEA (the start).
struct DeviceObjectInstance
{
    unsigned long size;
    GUID typeGuid;
    unsigned long offset;
    unsigned long type;
    unsigned long flags;             // DIDOI_FFACTUATOR (1): the object takes force-feedback output
};

// DIEFFECTINFOA (the start).
struct EffectInfo
{
    unsigned long size;
    GUID guid;
};

// DIMOUSESTATE.
struct MouseState
{
    long x;
    long y;
    long z;                          // the wheel
    unsigned char buttons[4];
};

struct JoyState;

// The input devices (one global object).
struct Input
{
    DirectInput *directInput;
    InputDevice *joystick;
    InputDevice *keyboard;
    InputDevice *mouse;
    GUID effectGuid;                 // 0x10: the periodic effect found on the joystick
    int effectFound;                 // 0x20
    int ready;                       // 0x24: devices set up
    int enumerated;                  // 0x28: DirectInput created and joysticks enumerated
    int forceAxes;                   // 0x2c: force-feedback axes (at most 2 used)
    EffectDesc effect;               // 0x30
    PeriodicForce periodic;          // 0x68: the effect's type-specific data (magnitude 0..10000)

    Input();                                         // 0x10142ae0 (input_poll.cpp): no devices yet, clears the button maps
    int Shutdown();                                  // 0x101437f0 (input_font.cpp): releases the devices
    int Rumble(float divisor, int milliseconds);     // 0x10142700 (input_font.cpp): starts the rumble effect
    int Update();                                    // 0x10142790 (input_poll.cpp): creates or polls the devices
    int ReadControls(int control);                   // 0x10143180 (input_controls.cpp): stick axes and button mask
    int IsJoystickButtonDown(int button, const JoyState *state);       // 0x10142b20 (joystick_buttons.cpp)
    int IsKeyboardButtonDown(int button, const unsigned char *keys);   // 0x10142dd0 (joystick_buttons.cpp)
};

// The game's input devices.
extern "C" Input DAT_10f08008;

#endif
