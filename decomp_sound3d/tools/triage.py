"""triage.py : write triage.tsv for the module (module.json game_start..game_end): (address, size, instructions, x87/SSE ops, calls, loops) for the game-code range."""
import os
import capstone
from gimg import Image, inventory, GAME_START, GAME_END
img, inv = Image(), inventory()
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
out = os.path.join(os.path.dirname(__file__), "..", "triage.tsv")
rows = []
for a, (name, size) in sorted(inv.items()):
    if not GAME_START <= a < GAME_END:
        continue
    n = fp = calls = back = 0
    for i in md.disasm(img.read(a, size), a):
        n += 1
        if i.mnemonic[0] == "f" and i.mnemonic not in ("fs",):
            fp += 1
        elif i.mnemonic.startswith("call"):
            calls += 1
        elif i.mnemonic[0] == "j" and i.op_str.startswith("0x") and int(i.op_str, 16) <= i.address:
            back += 1
    rows.append((a, size, n, fp, calls, back))
with open(out, "w") as f:
    f.write("address\tsize\tinsns\tfp\tcalls\tloops\n")
    for r in rows:
        f.write("%08x\t%d\t%d\t%d\t%d\t%d\n" % r)
small = [r for r in rows if r[3] == 0 and r[1] <= 120]
print("functions", len(rows), "no-fp <=120 bytes", len(small), "with fp", sum(1 for r in rows if r[3]))
