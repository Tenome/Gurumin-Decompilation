"""build_all.py : compile every src/*.cpp of this module with build.bat and print per-file and total matches.

usage: python tools/build_all.py [--report <report.json>]   (default report: module.json "report")

Same scoring as decomp/tools/build_all.py (APP.dll): MATCH = 100, EQUIV = 99, DIFF = share of identical bytes capped
at 98; "matched under the rules" uses decomp/tools/rule_check.py. The game functions are triage.tsv (module.json
game_start..game_end); functions not decompiled yet are listed in the unit "<prefix>unmatched" at 0 %.
"""
import csv, glob, json, os, re, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..")
repo = os.path.join(root, "..")
sys.path.insert(0, os.path.join(repo, "decomp", "tools"))
from rule_check import file_violations
sys.path.insert(0, here)
from gimg import MODULE

import importlib.util
_spec = importlib.util.spec_from_file_location("build_all_app", os.path.join(repo, "decomp", "tools", "build_all.py"))
build_all_app = importlib.util.module_from_spec(_spec)  # decomp/tools/build_all.py: measures() and unit_entry()
_spec.loader.exec_module(build_all_app)


def load_game_functions():
    rows = csv.DictReader(open(os.path.join(root, "triage.tsv")), delimiter="\t")
    return {r["address"]: int(r["size"]) for r in rows}


def load_symbol_addresses():
    names = {}
    for path in sorted(glob.glob(os.path.join(root, "obj_symbols*.tsv"))):  # obj_symbols.tsv plus per-lane files
        for line in open(path, encoding="utf-8"):
            parts = line.rstrip("\n").split("\t")
            if len(parts) > 1 and parts[1] and not parts[0].startswith("#"):
                names[parts[0]] = parts[1].lower()
    return names


def address_of(name, symbols):
    if name in symbols:
        return symbols[name]
    m = re.search(r"(?:FUN_|M_)([0-9a-fA-F]{8})", name)
    return m.group(1).lower() if m else None


def main():
    args = sys.argv[1:]
    report_path = args[args.index("--report") + 1] if "--report" in args else os.path.join(repo, MODULE["report"])
    prefix = MODULE["unit_prefix"]
    game = load_game_functions()
    symbols = load_symbol_addresses()
    total_m = total_d = total_e = 0
    units, all_functions, seen = [], [], set()
    for path in sorted(glob.glob(os.path.join(root, "src", "*.cpp"))):
        name = os.path.basename(path)
        rules_clean = not file_violations(path)
        out = subprocess.run(["cmd", "/c", os.path.join(root, "build.bat"), name], capture_output=True, text=True).stdout
        functions = []
        for line in out.splitlines():
            if line.startswith("DIFF") or "error" in line:
                print(name, line)
            m = re.match(r"MATCH (\S+) \((\d+) bytes", line)
            e = re.match(r"EQUIV (\S+)", line)
            d = re.match(r"DIFF (\S+) len ours=(\d+), diffs=(\d+)", line)
            if m:
                sym, percent = m.group(1), 100.0
            elif e:
                sym, percent = e.group(1), 99.0
            elif d:
                sym = d.group(1)
                percent = min(98.0, max(0.0, 100.0 * (1 - int(d.group(3)) / max(1, int(d.group(2))))))
            else:
                continue
            addr = address_of(sym, symbols)
            if addr is None or addr not in game or addr in seen:
                continue
            seen.add(addr)
            functions.append({"name": sym, "size": game[addr], "percent": percent, "address": addr,
                              "rules_clean": rules_clean})
        m = re.search(r"matched (\d+), differing (\d+), equivalent (\d+)", out)
        if m:
            total_m += int(m.group(1)); total_d += int(m.group(2)); total_e += int(m.group(3))
        else:
            print(name, "BUILD FAILED"); print(out[-2000:])
        all_functions += functions
        if functions:
            units.append(build_all_app.unit_entry(prefix + os.path.splitext(name)[0],
                                                  os.path.relpath(path, repo).replace(os.sep, "/"), functions))
    rest = [{"name": "FUN_" + a, "size": s, "percent": 0.0, "address": a} for a, s in sorted(game.items()) if a not in seen]
    matched = [f for f in all_functions if f["percent"] >= 100.0]
    ruled = [f for f in matched if f["rules_clean"]]
    code = sum(game.values())
    print("TOTAL matched %d, equivalent (13.00 encoding only) %d, differing %d" % (total_m, total_e, total_d))
    print("PROGRESS %d of %d game functions matched (%.1f%%), %d of %d code bytes (%.2f%%)" % (
        len(matched), len(game), 100.0 * len(matched) / len(game), sum(f["size"] for f in matched), code,
        100.0 * sum(f["size"] for f in matched) / code))
    print("RULES %d of %d matched functions are matched under the rules, %d code bytes" % (
        len(ruled), len(matched), sum(f["size"] for f in ruled)))
    units.append(build_all_app.unit_entry(prefix + "unmatched", "", rest))
    total = build_all_app.measures([{"size": f["size"], "percent": round(f["percent"], 4), "rules_clean": f.get("rules_clean")}
                                    for f in all_functions + rest])
    total["total_units"] = len(units)
    total["complete_units"] = sum(1 for u in units if u["metadata"]["complete"])
    os.makedirs(os.path.dirname(os.path.abspath(report_path)), exist_ok=True)
    with open(report_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump({"measures": total, "units": units}, f, indent=1)
    print("report %s: %d of %d functions, %.2f%% of code matched, fuzzy %.2f%%" % (
        os.path.relpath(report_path, repo), total["matched_functions"], total["total_functions"],
        total["matched_code_percent"], total["fuzzy_match_percent"]))


if __name__ == "__main__":
    main()
