"""check_matches.py [--against <git rev>] : compare this module's last build (build/report.json, from tools/build_all.py)
with its units in the committed progress/jp/report.json (default HEAD), by address. Lists lost and new matches;
exits 1 if any match was lost."""
import json, os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gimg import MODULE, MODULE_DIR, ROOT

PREFIX = MODULE["unit_prefix"]


def matched(report):
    """matched functions of this module by address -> name"""
    return {f["metadata"]["virtual_address"]: f["name"] for u in report["units"] if u["name"].startswith(PREFIX)
            for f in u.get("functions", []) if f.get("fuzzy_match_percent") == 100}


rev = sys.argv[sys.argv.index("--against") + 1] if "--against" in sys.argv else "HEAD"
shown = subprocess.run(["git", "show", "%s:progress/jp/report.json" % rev], cwd=ROOT, capture_output=True, text=True)
old = json.loads(shown.stdout) if shown.returncode == 0 else {"units": []}
new = json.load(open(os.path.join(MODULE_DIR, "build", "report.json")))
before, after = matched(old), matched(new)
lost = sorted(before[a] for a in set(before) - set(after))
gained = sorted(after[a] for a in set(after) - set(before))
print("matched %d -> %d (%+d)" % (len(before), len(after), len(after) - len(before)))
for name in gained:
    print("  new   " + name)
for name in lost:
    print("  LOST  " + name)
sys.exit(1 if lost else 0)
