# sound3d.dll queue

Four lanes by address range (the linker keeps each object file's functions together).

- Lane 1: 0x10001000-0x10003560, 32 functions, 9.4 KB
- Lane 2: 0x10003560-0x10005490, 17 functions, 7.9 KB (AllocateHeap 10003610)
- Lane 3: 0x10005490-0x10008090, 31 functions, 10.9 KB (AllocateBytes 100054d0; 10006810 4140 bytes, 148 x87 ops)
- Lane 4: 0x10008090-0x1000a800, 36 functions, 9.8 KB (MixOpen, MixClose, every I... export, IInit)
