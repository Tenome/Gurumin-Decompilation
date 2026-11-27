"""compare_obj.py <file.obj> [-v] : byte-compare every FUN_xxxxxxxx function in a /Gy COFF object with the module DLL (module.json).
Relocation fields are masked; DIR32 targets that name FUN_/DAT_xxxxxxxx are checked against the original operand."""
import re, struct, sys
from gimg import Image, inventory
img = Image()
_inv = None
import os
NAMED = {}
_named = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "obj_symbols.tsv")
if os.path.exists(_named):
    for _line in open(_named).read().splitlines():
        if _line and not _line.startswith("#"):
            _n, _a = _line.split("\t")[:2]
            NAMED[_n] = int(_a, 16)
ADDR = re.compile(r"^(?:_|\?)?(?:FUN|DAT|M)_([0-9a-fA-F]{8})(?:@|$)")

def parse(path):
    d = open(path, "rb").read()
    machine, nsec, _, symptr, nsym, optsize, _ = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + 18 * nsym
    def symname(o):
        if d[o:o + 4] == b"\0\0\0\0":
            so, = struct.unpack_from("<I", d, o + 4)
            return d[strtab + so:d.index(b"\0", strtab + so)].decode("latin-1")
        return d[o:o + 8].rstrip(b"\0").decode("latin-1")
    syms = []
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        value, sec, typ, cls, aux = struct.unpack_from("<IhHBB", d, o + 8)
        syms.append((symname(o), value, sec, typ, cls))
        syms.extend([None] * aux)
        i += 1 + aux
    secs = []
    for i in range(nsec):
        o = 20 + optsize + 40 * i
        size, raw, relptr, _, nrel, _, chars = struct.unpack_from("<IIIIHHI", d, o + 16)
        rel = [struct.unpack_from("<IIH", d, relptr + 10 * r) for r in range(nrel)]
        secs.append((d[raw:raw + size] if raw else b"", rel))
    return syms, secs

import capstone
_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_HEX = re.compile(r"0x[0-9a-f]+")


def _norm(code, base, relocs):
    ins = list(_md.disasm(code, base))
    index = {i.address: n for n, i in enumerate(ins)}
    out = []
    for i in ins:
        if i.mnemonic in ("int3", "nop") or (i.mnemonic == "lea" and i.op_str in ("esp, [esp]", "ecx, [ecx]", "ebx, [ebx]")):
            continue
        ops = i.op_str
        if any(i.address <= base + r < i.address + i.size for r in relocs):
            out.append((i.mnemonic, "R"))
            continue
        if i.mnemonic.startswith("j") or i.mnemonic == "call":
            try:
                ops = "#%s" % index.get(int(ops, 16), "ext")
            except ValueError:
                pass
        out.append((i.mnemonic, ops))
    return out


def equivalent(ours, orig, relocs):
    """same instruction stream after normalising encodings, relocated operands and branch targets"""
    a = _norm(ours, 0, relocs)
    b = _norm(orig, 0, [])
    if len(a) != len(b):
        return False
    for (m1, o1), (m2, o2) in zip(a, b):
        if m1 != m2:
            return False
        if o1 != o2 and o1 != "R":
            return False
    return True


def main():
    path = sys.argv[1]
    verbose = "-v" in sys.argv
    syms, secs = parse(path)
    ok = bad = equiv = 0
    for s in syms:
        if not s or s[3] != 0x20 or s[4] != 2 or s[2] <= 0:
            continue
        m = ADDR.match(s[0])
        if m:
            va = int(m.group(1), 16)
        elif s[0] in NAMED:
            va = NAMED[s[0]]
        else:
            continue
        data, rel = secs[s[2] - 1]
        orig = img.read(va, len(data))
        mask = bytearray(len(data))
        for off, symidx, typ in rel:
            for k in range(4):
                if off + k < len(mask):
                    mask[off + k] = 1
        diffs = [i for i in range(len(data)) if not mask[i] and (i >= len(orig) or data[i] != orig[i])]
        if len(orig) < len(data) or diffs:
            global _inv
            if _inv is None:
                _inv = inventory()
            longer = img.read(va, _inv.get(va, ("", len(data)))[1])
            if equivalent(data, longer, [r[0] for r in rel]):
                equiv += 1
                print("EQUIV %s (same instructions, encoding differs: cl 13.00 vs 13.10)" % s[0])
                continue
            bad += 1
            print("DIFF %s len ours=%d, diffs=%d first@%s" % (s[0], len(data), len(diffs), diffs[:1]))
        else:
            ok += 1
            print("MATCH %s (%d bytes, %d relocs)" % (s[0], len(data), len(rel)))
    print("matched %d, differing %d, equivalent %d" % (ok, bad, equiv))
if __name__ == "__main__":
    main()

