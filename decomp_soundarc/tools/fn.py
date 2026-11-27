"""fn.py <hex addr> : Ghidra's decompilation of one function"""
import sys
import os
from gimg import DECOMPILED, EXTRA
pat = " @ %s" % sys.argv[1].lower()
on = False
lines = list(open(DECOMPILED, encoding="utf-8", errors="replace"))
if os.path.exists(EXTRA):
    lines += list(open(EXTRA, encoding="utf-8", errors="replace"))
for line in lines:
    if not on and line.startswith("// ====") and pat in line:
        on = True
    elif on and line.startswith("// ===="):
        break
    if on:
        print(line, end="")


