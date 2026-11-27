"""gimg.py : the module's original DLL image (module.json names it) and its function inventory."""
import json, os, struct
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MODULE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
MODULE = json.load(open(os.path.join(MODULE_DIR, "module.json")))
DLL = os.path.join(ROOT, "re", "bin", MODULE["dll"])
FUNCS = os.path.join(ROOT, "re", "export", MODULE["dll"], "functions.tsv")
DECOMPILED = os.path.join(ROOT, "re", "export", MODULE["dll"], "decompiled.c")
EXTRA = os.path.join(ROOT, "re", "export", MODULE["dll"], "extra.c")
GAME_START = int(MODULE["game_start"], 16)
GAME_END = int(MODULE["game_end"], 16)


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


TRIAGE = os.path.join(MODULE_DIR, "triage.tsv")


def inventory():
    inv = {}
    for line in open(FUNCS, encoding="utf-8", errors="replace").read().splitlines()[1:]:
        c = line.split("\t")
        try:
            inv[int(c[0], 16)] = (c[1], int(c[2]))
        except (ValueError, IndexError):
            pass
    for text, name in MODULE.get("extra_functions", {}).items():
        inv[int(text, 16)] = (name, 0)  # exports Ghidra decompiled only into extra.c
    # Ghidra's sizes leave out epilogues and switch tables: a game function runs to the next function start,
    # minus the alignment padding (nop or int3) in front of it
    starts = sorted(inv)
    image = Image()
    for a, b in zip(starts, starts[1:] + [GAME_END]):
        if GAME_START <= a < GAME_END:
            code = image.read(a, min(b, GAME_END) - a)
            inv[a] = (inv[a][0], len(code.rstrip(bytes([0x90, 0xCC]))))
    if os.path.exists(TRIAGE):
        for line in open(TRIAGE).read().splitlines()[1:]:
            c = line.split("\t")
            a = int(c[0], 16)
            if a not in inv:
                inv[a] = ("FUN_%08x" % a, int(c[1]))
    return inv
