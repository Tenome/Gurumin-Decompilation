// window_holder.h : WindowHolder, the empty helper object behind the two uncalled methods at 0x10009400/0x10009410.
// The name describes what the bytes do (clear the engine's window handle), not a known original class.
#ifndef WINDOW_HOLDER_H
#define WINDOW_HOLDER_H

class WindowHolder {
public:
    WindowHolder *ClearWindow();    // 0x10009400
    void Release();                 // 0x10009410
};

#endif
