import os, struct
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DLL = os.path.join(ROOT, "re", "bin", "APP.dll")
FUNCS = os.path.join(ROOT, "re", "export", "APP.dll", "functions.tsv")
DECOMPILED = os.path.join(ROOT, "re", "export", "APP.dll", "decompiled.c")

class Image:
    def __init__(self, path=DLL):
        d = self.data = open(path, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec, = struct.unpack_from("<H", d, pe + 6)
        opt, = struct.unpack_from("<H", d, pe + 20)
        self.base, = struct.unpack_from("<I", d, pe + 24 + 28)
        self.sections = {}
        off = pe + 24 + opt
        for _ in range(nsec):
            name = d[off:off + 8].rstrip(b"\0").decode()
            vsize, va, rsize, raw = struct.unpack_from("<IIII", d, off + 8)
            self.sections[name] = (self.base + va, vsize, raw, rsize)
            off += 40

    def read(self, va, size):
        for start, vsize, raw, rsize in self.sections.values():
            if start <= va < start + rsize:
                o = raw + va - start
                return self.data[o:o + min(size, start + rsize - va)]
        return b""

TRIAGE = os.path.join(ROOT, "decomp", "triage.tsv")
FIXES = os.path.join(ROOT, "decomp", "inventory_fixes.tsv")
GAME_START, GAME_END = 0x10001000, 0x101b2000  # game code of APP.dll; the CRT follows

def load_fixes():
    """inventory_fixes.tsv rows: (address, kind, note); kind 'add' = function start Ghidra missed,
    'merge' = Ghidra start that lies inside the previous function (jump target, not a function)."""
    rows = []
    if os.path.exists(FIXES):
        for line in open(FIXES, encoding="utf-8").read().splitlines()[1:]:
            c = line.split("\t")
            if len(c) >= 2 and c[0]:
                rows.append((int(c[0], 16), c[1], c[2] if len(c) > 2 else ""))
    return rows

def inventory():
    inv = {}
    for line in open(FUNCS, encoding="utf-8", errors="replace").read().splitlines()[1:]:
        c = line.split("\t")
        try:
            inv[int(c[0], 16)] = (c[1], int(c[2]))
        except (ValueError, IndexError):
            pass
    if os.path.exists(TRIAGE):
        for line in open(TRIAGE).read().splitlines()[1:]:
            c = line.split("\t")
            a = int(c[0], 16)
            if a not in inv:
                inv[a] = ("FUN_%08x" % a, int(c[1]))
    for a, kind, _ in load_fixes():
        if kind == "add":
            inv[a] = ("FUN_%08x" % a, 0)
        elif kind == "merge":
            inv.pop(a, None)
    # Ghidra's sizes leave out epilogues and switch tables: a game function runs to the next function start,
    # minus the alignment padding (nop or int3) in front of it
    image = Image()
    starts = sorted(inv)
    for i, a in enumerate(starts):
        if GAME_START <= a < GAME_END:
            end = starts[i + 1] if i + 1 < len(starts) else a + inv[a][1]
            code = image.read(a, end - a).rstrip(bytes([0x90, 0xCC]))
            inv[a] = (inv[a][0], len(code))
    return inv
