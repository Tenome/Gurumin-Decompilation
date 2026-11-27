# SoundArc.dll queue (one lane: src/sound_server.cpp)

- [ ] 00401000 630 WinMain: mutex, window class, tray icon, c:\sound.log, message loop with a 100 ms timer
- [ ] 00401280 66 pump one message (PeekMessage/Translate/Dispatch/Sleep(1))
- [ ] 004012d0 134 register the window class, 00401360 147 create the window
- [ ] 00401400 78 log helper (opens c:\sound.log "at", fprintf)
- [ ] 00401450 4585 window procedure / command dispatcher (48 x87 ops, 98 calls)
- [ ] 00402640 114, 004026c0 22 (returns its fastcall argument), 004026e0 19 (empty)
- [ ] 00402700 1002 load sound3d.dll and resolve its I... entry points (44 calls)
- [ ] 00402af0 64 release / FreeLibrary
