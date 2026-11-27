"""build_all.py : compile every src/*.cpp with build.bat and print per-file and total matches.

usage: python tools/build_all.py [--report progress/jp/report.json] [--version jp]

--report writes an objdiff-format progress report (function names, addresses, sizes and match percentages only)
for ByteMatch / decomp.dev, plus progress.svg (README meter) next to it. Functions not yet decompiled are listed in the unit "app/unmatched" at 0 %.
With --version jp the report covers the whole JP game: the helper-DLL trees in MODULES (Packs.dll, SoundArc.dll) are
built with their own tools/build_all.py and their units ("packs/...", "soundarc/...") are added; the measures are the
sums over APP.dll and the modules.
"Matched under the rules" counts the matched functions whose file declares no struct/class/union and no extern
global of its own (AGENTS.md rule 2; tools/rule_check.py decides): printed as RULES and added to the measures.
Percentages: MATCH = 100, EQUIV (cl 13.00 encoding quirks only) = 99, DIFF = share of identical bytes, capped at 98.
"""
import csv, glob, json, os, re, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rule_check import file_violations

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, "..")
MODULES = ["decomp_packs", "decomp_soundarc"]  # helper-DLL trees (module.json) merged into the JP report


def load_game_functions():
    rows = [r for r in csv.DictReader(open(os.path.join(root, "triage.tsv")), delimiter="\t") if r["address"] < "101b2000"]
    return {r["address"]: int(r["size"]) for r in rows}


def load_symbol_addresses():
    names = {}
    for line in open(os.path.join(root, "obj_symbols.tsv"), encoding="utf-8"):
        parts = line.rstrip("\n").split("\t")
        if len(parts) > 1 and parts[1]:
            names[parts[0]] = parts[1].lower()
    return names


def address_of(name, symbols):
    if name in symbols:
        return symbols[name]
    m = re.search(r"(?:FUN_|M_)(10[0-9a-fA-F]{6})", name)
    return m.group(1).lower() if m else None


def measures(functions):
    total_code = sum(f["size"] for f in functions)
    matched = [f for f in functions if f["percent"] >= 100.0]
    matched_code = sum(f["size"] for f in matched)
    fuzzy = sum(f["size"] * f["percent"] for f in functions) / total_code if total_code else 0.0
    complete = bool(functions) and len(matched) == len(functions)
    # matched under the rules: byte-identical and defined in a file without per-file types or globals
    ruled = [f for f in matched if f.get("rules_clean")]
    ruled_code = sum(f["size"] for f in ruled)
    return {
        "fuzzy_match_percent": round(fuzzy, 4),
        "total_code": str(total_code),
        "matched_code": str(matched_code),
        "matched_code_percent": round(100.0 * matched_code / total_code, 4) if total_code else 0.0,
        "total_data": "0",
        "matched_data": "0",
        "matched_data_percent": 0.0,
        "total_functions": len(functions),
        "matched_functions": len(matched),
        "matched_functions_percent": round(100.0 * len(matched) / len(functions), 4) if functions else 0.0,
        "complete_code": str(total_code if complete else 0),
        "complete_code_percent": 100.0 if complete else 0.0,
        "complete_data": "0",
        "complete_data_percent": 0.0,
        "total_units": 1,
        "complete_units": 1 if complete else 0,
        "matched_rules_code": str(ruled_code),
        "matched_rules_code_percent": round(100.0 * ruled_code / total_code, 4) if total_code else 0.0,
        "matched_rules_functions": len(ruled),
        "matched_rules_functions_percent": round(100.0 * len(ruled) / len(functions), 4) if functions else 0.0,
    }


def unit_entry(name, source, functions):
    m = measures(functions)
    return {
        "name": name,
        "measures": m,
        "sections": [{"name": ".text", "size": m["total_code"], "fuzzy_match_percent": m["fuzzy_match_percent"]}],
        "functions": [{"name": f["name"], "size": str(f["size"]), "fuzzy_match_percent": round(f["percent"], 4),
                       "metadata": {"virtual_address": str(int(f["address"], 16))}} for f in functions],
        "metadata": {"complete": m["complete_units"] == 1, "source_path": source},
    }


def build_module_report(module):
    """Run a helper-DLL tree's build_all.py into its build folder; return (units, measures) of its report."""
    module_root = os.path.join(root, "..", module)
    out = os.path.join(module_root, "build", "report.json")
    run = subprocess.run([sys.executable, os.path.join(module_root, "tools", "build_all.py"), "--report", out],
                         capture_output=True, text=True)
    for line in run.stdout.splitlines():
        if line.startswith(("PROGRESS", "RULES")):
            print(module, line)
    if run.returncode != 0:
        print(run.stdout[-2000:], run.stderr[-2000:])
        raise SystemExit("%s: build_all failed" % module)
    report = json.load(open(out, encoding="utf-8"))
    return report["units"], report["measures"]


def add_measures(a, b):
    """Measures of two reports taken together (sums, percentages recomputed)."""
    def total(key):
        return int(a[key]) + int(b[key])
    code, functions = total("total_code"), a["total_functions"] + b["total_functions"]
    matched_code, ruled_code = total("matched_code"), total("matched_rules_code")
    matched_functions = a["matched_functions"] + b["matched_functions"]
    ruled_functions = a["matched_rules_functions"] + b["matched_rules_functions"]
    complete_code = total("complete_code")
    fuzzy = (a["fuzzy_match_percent"] * int(a["total_code"]) + b["fuzzy_match_percent"] * int(b["total_code"])) / code
    m = dict(a)
    m.update({
        "fuzzy_match_percent": round(fuzzy, 4),
        "total_code": str(code),
        "matched_code": str(matched_code),
        "matched_code_percent": round(100.0 * matched_code / code, 4),
        "total_functions": functions,
        "matched_functions": matched_functions,
        "matched_functions_percent": round(100.0 * matched_functions / functions, 4),
        "complete_code": str(complete_code),
        "complete_code_percent": round(100.0 * complete_code / code, 4),
        "matched_rules_code": str(ruled_code),
        "matched_rules_code_percent": round(100.0 * ruled_code / code, 4),
        "matched_rules_functions": ruled_functions,
        "matched_rules_functions_percent": round(100.0 * ruled_functions / functions, 4),
    })
    return m


def main():
    args = sys.argv[1:]
    report_path = args[args.index("--report") + 1] if "--report" in args else None
    version = args[args.index("--version") + 1] if "--version" in args else "jp"
    game = load_game_functions()
    symbols = load_symbol_addresses()
    total_m = total_d = total_e = 0
    matched_bytes = 0
    ruled_matched = ruled_bytes = 0  # the matched functions whose file passes rule_check
    unique_matched = 0  # game functions matched, each address counted once (some files repeat a callee)
    units = []
    all_functions = []  # every compared function with its rules_clean flag, for the report's total measures
    seen = set()
    sources = [(p, os.path.basename(p), []) for p in sorted(glob.glob(os.path.join(root, "src", "*.cpp")))]
    sources += [(p, os.path.join("..", "src_od", os.path.basename(p)), ["/Od", "/Oi", "/Op-"])  # the /Od units were built without /Op (fcomp compares)
                for p in sorted(glob.glob(os.path.join(root, "src_od", "*.cpp")))]
    for path, build_name, flags in sources:
        name = os.path.basename(path)
        rules_clean = not file_violations(path)
        out = subprocess.run(["cmd", "/c", os.path.join(root, "build.bat"), build_name] + flags,
                             capture_output=True, text=True).stdout
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
            if percent >= 100.0:
                unique_matched += 1
                matched_bytes += game[addr]
                if rules_clean:
                    ruled_matched += 1
                    ruled_bytes += game[addr]
            functions.append({"name": sym, "size": game[addr], "percent": percent, "address": addr,
                              "rules_clean": rules_clean})
        m = re.search(r"matched (\d+), differing (\d+), equivalent (\d+)", out)
        if m:
            total_m += int(m.group(1)); total_d += int(m.group(2)); total_e += int(m.group(3))
        else:
            print(name, "BUILD FAILED"); print(out[-2000:])
        all_functions += functions
        if functions:
            units.append(unit_entry("app/" + os.path.splitext(name)[0], os.path.relpath(path, os.path.join(root, "..")).replace(os.sep, "/"), functions))
    print("TOTAL matched %d, equivalent (13.00 encoding only) %d, differing %d" % (total_m, total_e, total_d))
    code = sum(game.values())
    print("PROGRESS %d of %d game functions matched (%.1f%%), %d of %d code bytes (%.2f%%)" % (
        unique_matched, len(game), 100.0 * unique_matched / len(game), matched_bytes, code, 100.0 * matched_bytes / code))
    print("RULES %d of %d matched functions are matched under the rules (no per-file types or globals), %d code bytes (%.2f%%)" % (
        ruled_matched, unique_matched, ruled_bytes, 100.0 * ruled_bytes / code))
    if report_path:
        rest = [{"name": "FUN_" + a, "size": s, "percent": 0.0, "address": a} for a, s in sorted(game.items()) if a not in seen]
        units.append(unit_entry("app/unmatched", "", rest))
        everything = [{"size": f["size"], "percent": round(f["percent"], 4), "rules_clean": f.get("rules_clean")}
                      for f in all_functions + rest]
        total = measures(everything)
        if version == "jp":
            for module in MODULES:
                module_units, module_total = build_module_report(module)
                units += module_units
                total = add_measures(total, module_total)
        total["total_units"] = len(units)
        total["complete_units"] = sum(1 for u in units if u["metadata"]["complete"])
        report = {"measures": total, "units": units}
        os.makedirs(os.path.dirname(os.path.abspath(report_path)), exist_ok=True)
        with open(report_path, "w", encoding="utf-8", newline="\n") as f:
            json.dump(report, f, indent=1)
        subprocess.run([sys.executable, os.path.join(here, "progress_svg.py"), report_path,
                        os.path.join(os.path.dirname(os.path.abspath(report_path)), "progress.svg")], check=True)
        print("report (%s): %d of %d functions, %.2f%% of code matched (%d functions, %.2f%% under the rules), fuzzy %.2f%%" % (
            version, total["matched_functions"], total["total_functions"], total["matched_code_percent"],
            total["matched_rules_functions"], total["matched_rules_code_percent"], total["fuzzy_match_percent"]))


if __name__ == "__main__":
    main()
